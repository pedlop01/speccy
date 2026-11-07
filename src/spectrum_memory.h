#ifndef SPECTRUM_MEMORY_H
#define SPECTRUM_MEMORY_H

#include <array>
#include "rom_loader.h"
#include "ula.h"
#include "disk.h"

// One physical store per RAM bank. The primary screen bank lives in ULAMemory
// so writes through either CPU window also notify the renderer. The ULA
// reads the selected screen bank independently of the CPU memory windows.
class SpectrumMemory : public BusComponent<0, 65536> {
public:
  explicit SpectrumMemory(ULA &screen) : ula(screen), screen(static_cast<ULAMemory &>(screen)) { Reset(); }
  SpectrumMemory(const SpectrumMemory &) = delete;
  SpectrumMemory &operator=(const SpectrumMemory &) = delete;

  bool Configure(const ROMSet &rom, std::string &error) {
    if (!ValidateROMHardware(rom, error)) return false;
    const auto &candidate = HardwareFor(rom.model);
    auto pages = rom.pages;
    romPages.swap(pages);
    configuration = &candidate;
    Reset();
    error.clear();
    return true;
  }

  void Reset() {
    for (auto &bank : banks) bank.fill(0);
    static_cast<ULAMemory &>(screen).Clear();
    paging = pagingPlus3 = 0;
    ula.SelectScreen(nullptr);
  }
  struct State { std::vector<unsigned char> ram; unsigned char paging; unsigned char pagingPlus3; };
  State CaptureState() const {
    State state;
    state.paging = paging;state.pagingPlus3=pagingPlus3;
    state.ram.resize(configuration->memory.RAMBytes());
    for (unsigned bank = 0; bank < configuration->memory.ramBanks; ++bank) {
      const auto *source = bank == configuration->memory.screenBanks[0] ? ula.PrimaryBank() : banks[bank].data();
      std::copy(source, source + SPECTRUM_BANK_BYTES, state.ram.begin() + bank * SPECTRUM_BANK_BYTES);
    }
    return state;
  }
  bool RestoreState(const State &state) {
    if (state.ram.size() != configuration->memory.RAMBytes()) return false;
    for (unsigned bank = 0; bank < configuration->memory.ramBanks; ++bank) {
      const auto *source = state.ram.data() + bank * SPECTRUM_BANK_BYTES;
      if (bank == configuration->memory.screenBanks[0]) ula.RestorePrimaryBank(source);
      else std::copy(source, source + SPECTRUM_BANK_BYTES, banks[bank].begin());
    }
    paging = IsPaged() ? state.paging : 0;pagingPlus3=IsPlus3()?state.pagingPlus3:0;
    ula.SelectScreen(ScreenBank() == configuration->memory.screenBanks[0] ? nullptr : banks[ScreenBank()].data());
    return true;
  }
  bool IsPlus3() const {return configuration->memory.paging==PagingKind::Plus3;}
  unsigned char PagingLatchPlus3() const {return pagingPlus3;}
  bool SpecialPaging() const {return IsPlus3() && (pagingPlus3&1);}
  unsigned char PagingLatch() const { return paging; }
  bool PagingLocked() const { return (paging & 0x20) != 0; }
  unsigned ROMPage() const {
    return IsPaged() ? (((paging >> 4) & 1)|(IsPlus3()?((pagingPlus3>>1)&2):0)) : configuration->memory.bootROMPage;
  }
  unsigned ScreenBank() const { return configuration->memory.screenBanks[(paging >> 3) & 1]; }
  unsigned RAMBank(unsigned slot) const {
    if(SpecialPaging()){static const unsigned maps[4][4]={{0,1,2,3},{4,5,6,7},{4,5,6,3},{4,7,6,3}};return maps[(pagingPlus3>>1)&3][slot+1];}
    return IsPaged() && slot == 2 ? paging & 7 : configuration->memory.bootRAMBanks.at(slot);
  }
  void WritePaging(unsigned port, unsigned char value) {
    if(!IsPaged() || PagingLocked())return;
    unsigned previous=ScreenBank();
    if(IsPlus3()) {
      if((port&0xf002)==0x1000)pagingPlus3=value;
      else if((port&0xc002)==0x4000)paging=value;
    }else if(!(port&0x8002))paging=value;
    if(previous!=ScreenBank())ula.SelectScreen(ScreenBank()==configuration->memory.screenBanks[0]?nullptr:banks[ScreenBank()].data());
  }

  unsigned char Read(unsigned address) override {
    address &= 0xFFFF;
    if (address < 0x4000) {
      if(SpecialPaging())return ReadBank((pagingPlus3&6)?4:0,address);
      return romPages.empty() ? 0xFF : romPages[ROMPage()][address];
    }
    const unsigned bank = RAMBank((address >> 14) - 1);
    return ReadBank(bank, address & 0x3FFF);
  }
  void Write(unsigned address, unsigned char value) override {
    address &= 0xFFFF;
    if (address < 0x4000 && !SpecialPaging()) return;
    const unsigned bank = address<0x4000?((pagingPlus3&6)?4:0):RAMBank((address >> 14) - 1);
    const unsigned offset = address & 0x3FFF;
    if (bank == configuration->memory.screenBanks[0]) screen.Write(0x4000 + offset, value);
    else {
      banks[bank][offset] = value;
      if (bank == ScreenBank()) ula.DisplayWrite(offset, value);
    }
  }
  unsigned char ReadBank(unsigned bank, unsigned offset) {
    if (bank >= configuration->memory.ramBanks || offset >= SPECTRUM_BANK_BYTES)
      throw std::out_of_range("Invalid RAM bank or offset");
    return bank == configuration->memory.screenBanks[0] ? screen.Read(0x4000 + offset) : banks[bank][offset];
  }

private:
  // Access via the public bus interface; ULAMemory's overrides are protected.
  ULA &ula;
  BusComponentBase &screen;
  std::array<std::array<unsigned char, SPECTRUM_BANK_BYTES>, 8> banks;
  std::vector<std::vector<unsigned char>> romPages;
  const MachineConfiguration *configuration = &HardwareFor(MachineModel::Spectrum48);
  unsigned char paging = 0,pagingPlus3=0;
  bool IsPaged() const { return configuration->memory.paging != PagingKind::Flat; }
};

// I/O devices may share decoded addresses, so writes are offered to both.
// Paging has no readable register: reads retain keyboard/EAR/floating-bus behavior.
class SpectrumIO : public BusComponent<0, 65536> {
public:
  SpectrumIO(ULA &ula, SpectrumMemory &memory) : ula(ula), memory(memory) {}
  Disks::Controller floppy;
  bool kempstonEnabled=false;
  unsigned char kempstonState=0;
  unsigned char Read(unsigned address) override {
    // Compatible Kempston interfaces partially decode A5, so 0xDF (Navy
    // Moves), 0x5F and 0x9F alias 0x1F; the upper address byte is ignored.
    const bool kempston=kempstonEnabled && !(address&0x20);
    const unsigned char joystick=kempston?kempstonState&31:0xff;
    if(memory.IsPlus3()){
      if((address&0xf002)==0x2000)return floppy.Status()&joystick;
      if((address&0xf002)==0x3000)return floppy.Read()&joystick;
    }
    unsigned char value = ula.IORead(address & 0xFFFF);
    if(kempston)value=(address&1)?joystick:value&joystick;
    if (ula.ay.Enabled() && (address & 0xC002) == 0xC000) {
      const unsigned char ayValue = ula.ay.Read();
      return (address & 1) ? ayValue&joystick : value & ayValue;
    }
    return value;
  }
  void Write(unsigned address, unsigned char value) override {
    ula.IOWrite(address & 0xFFFF, value);
    memory.WritePaging(address & 0xFFFF, value);
    if(memory.IsPlus3()){
      floppy.Motor((memory.PagingLatchPlus3()&8)!=0);
      if((address&0xf002)==0x3000)floppy.Write(value);
    }
    if (ula.ay.Enabled()) {
      if ((address & 0xC002) == 0xC000) ula.ay.Select(value);
      else if ((address & 0xC002) == 0x8000) ula.ay.Write(value);
    }
  }
private:
  ULA &ula;
  SpectrumMemory &memory;
};
#endif
