#ifndef SPECTRUM_MEMORY_H
#define SPECTRUM_MEMORY_H

#include <array>
#include "rom_loader.h"
#include "ula.h"

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
    if (candidate.memory.paging == PagingKind::Plus3) {
      error = "+3 memory paging is not implemented";
      return false;
    }
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
    paging = 0;
    ula.SelectScreen(nullptr);
  }
  unsigned char PagingLatch() const { return paging; }
  bool PagingLocked() const { return (paging & 0x20) != 0; }
  unsigned ROMPage() const {
    return IsPaged() ? ((paging >> 4) & 1) : configuration->memory.bootROMPage;
  }
  unsigned ScreenBank() const { return configuration->memory.screenBanks[(paging >> 3) & 1]; }
  unsigned RAMBank(unsigned slot) const {
    return IsPaged() && slot == 2 ? paging & 7 : configuration->memory.bootRAMBanks.at(slot);
  }
  void WritePaging(unsigned port, unsigned char value) {
    // 128K and grey +2 decode only A15 and A1, including overlapping ULA ports.
    if (IsPaged() && !(port & 0x8002) && !PagingLocked()) {
      const unsigned previous = ScreenBank();
      paging = value;
      if (ScreenBank() != previous)
        ula.SelectScreen(ScreenBank() == configuration->memory.screenBanks[0] ? nullptr : banks[ScreenBank()].data());
    }
  }
  unsigned char Read(unsigned address) override {
    address &= 0xFFFF;
    if (address < 0x4000)
      return romPages.empty() ? 0xFF : romPages[ROMPage()][address];
    const unsigned bank = RAMBank((address >> 14) - 1);
    return ReadBank(bank, address & 0x3FFF);
  }
  void Write(unsigned address, unsigned char value) override {
    address &= 0xFFFF;
    if (address < 0x4000) return;
    const unsigned bank = RAMBank((address >> 14) - 1);
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
  unsigned char paging = 0;
  bool IsPaged() const { return configuration->memory.paging == PagingKind::Spectrum128; }
};

// I/O devices may share decoded addresses, so writes are offered to both.
// Paging has no readable register: reads retain keyboard/EAR/floating-bus behavior.
class SpectrumIO : public BusComponent<0, 65536> {
public:
  SpectrumIO(ULA &ula, SpectrumMemory &memory) : ula(ula), memory(memory) {}
  unsigned char Read(unsigned address) override {
    unsigned char value = ula.IORead(address & 0xFFFF);
    if (ula.ay.Enabled() && (address & 0xC002) == 0xC000) {
      const unsigned char ayValue = ula.ay.Read();
      return (address & 1) ? ayValue : value & ayValue;
    }
    return value;
  }
  void Write(unsigned address, unsigned char value) override {
    ula.IOWrite(address & 0xFFFF, value);
    memory.WritePaging(address & 0xFFFF, value);
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
