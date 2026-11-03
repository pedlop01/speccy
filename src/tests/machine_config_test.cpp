#include <cassert>
#include <cstdio>
#include "../emulator.h"

struct TimedULA : ULA {
  void At(unsigned tstate) { dwFrameTStates = tstate; }
};

int main() {
  const auto &flat = HardwareFor(MachineModel::Spectrum48);
  assert(flat.memory.RAMBytes() == 48 * 1024 && flat.memory.ROMBytes() == 16384);
  assert(flat.memory.bootROMPage == 0 && flat.memory.paging == PagingKind::Flat);
  assert((flat.memory.bootRAMBanks == std::array<unsigned, 3>{{0, 1, 2}}));
  assert(!flat.memory.HasShadowScreen() && !flat.devices.HasAY() && !flat.devices.floppyController);
  assert(flat.timing.cpuClockHz == 3500000 && flat.timing.TStatesPerFrame() == 69888);
  assert(flat.timing.firstDisplayTstate == 14336 && flat.timing.interruptPulseTstates == 32);
  assert(RequiredHardwareFeatures(flat) == 0 && MissingHardwareFeatures(flat).empty());
  assert(SupportsHardware(MachineModel::Spectrum48));

  for (auto model : {MachineModel::Spectrum128, MachineModel::Plus2, MachineModel::Plus3}) {
    const auto &hardware = HardwareFor(model);
    assert(hardware.memory.RAMBytes() == 128 * 1024);
    assert((hardware.memory.bootRAMBanks == std::array<unsigned, 3>{{5, 2, 0}}));
    assert((hardware.memory.screenBanks == std::array<unsigned, 2>{{5, 7}}));
    assert(hardware.memory.HasShadowScreen() && hardware.devices.HasAY());
    assert(hardware.devices.ayClockHz == 1773400);
    assert(hardware.timing.cpuClockHz == 3546900);
    assert(hardware.timing.tstatesPerLine == 228 && hardware.timing.linesPerFrame == 311);
    assert(hardware.timing.TStatesPerFrame() == 70908);
    assert(SupportsHardware(model));
    assert(MissingHardwareFeatures(hardware).find("banked RAM") == std::string::npos);
    assert(MissingHardwareFeatures(hardware).find("AY sound") == std::string::npos);
  }
  assert(HardwareFor(MachineModel::Spectrum128).memory.romPages == 2);
  assert(HardwareFor(MachineModel::Plus2).memory.paging == PagingKind::Spectrum128);
  const auto &plus3 = HardwareFor(MachineModel::Plus3);
  assert(plus3.memory.romPages == 4 && plus3.memory.paging == PagingKind::Plus3);
  assert(plus3.devices.floppyController && plus3.timing.floatingBus == FloatingBusKind::Idle);
  assert(plus3.timing.firstDisplayTstate == 14365);
  assert(MissingHardwareFeatures(plus3).empty());
  bool rejected = false;
  try { HardwareFor(static_cast<MachineModel>(99)); } catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected); // Unknown hardware must never silently default to 48K.

  std::string error;
  for (const auto &romProfile : ROMProfiles()) {
    const auto &hardware = HardwareFor(romProfile.model);
    assert(romProfile.files.size() == hardware.memory.romPages);
    assert(hardware.memory.bootROMPage < hardware.memory.romPages);
    for (unsigned bank : hardware.memory.bootRAMBanks) assert(bank < hardware.memory.ramBanks);
    ROMSet set;
    assert(LoadROMSet(FindROM(romProfile.files.front()), set, error));
    assert(ValidateROMHardware(set, error));
    set.pages.pop_back(); assert(!ValidateROMHardware(set, error));
    assert(LoadROMSet(FindROM(romProfile.files.front()), set, error));
    set.pages.front().pop_back(); assert(!ValidateROMHardware(set, error));
  }
  ROMSet english, spanish;
  assert(LoadROMSet(FindROM("128-0.rom"), english, error));
  assert(LoadROMSet(FindROM("128-spanish-0.rom"), spanish, error));
  assert(&HardwareFor(english.model) == &HardwareFor(spanish.model));

  // Exercise actual ULA consumers, not just descriptor values: frame boundaries,
  // residual cycles, floating-bus phase and the +3 idle-bus policy.
  TimedULA ula;
  bool irq = false;
  ula.Configure(MachineModel::Spectrum128);
  ula.AddCycles(70907, irq); assert(!irq);
  ula.AddCycles(1, irq); assert(irq);
  ula.AddCycles(17, irq); assert(!irq);
  ula.AddCycles(70908 - 18, irq); assert(!irq);
  ula.AddCycles(1, irq); assert(irq);
  static_cast<BusComponentBase *>(static_cast<ULAMemory *>(&ula))->Write(0x4000, 0x12);
  ula.At(14363); assert(ula.IORead(0xFFFF) == 0xFF);
  ula.At(14364); assert(ula.IORead(0xFFFF) == 0x12);
  ula.Configure(MachineModel::Plus3);
  static_cast<BusComponentBase *>(static_cast<ULAMemory *>(&ula))->Write(0x4000, 0x12);
  ula.At(14364); assert(ula.IORead(0xFFFF) == 0xFF);
  ula.Configure(MachineModel::Spectrum48);
  assert(ula.Timing().tstatesPerLine == 224);

  SpectrumMachine machine;
  assert(machine.LoadROM(FindROM("48.rom"), error));
  assert(&machine.Configuration() == &flat);
  assert(machine.ula.Timing().TStatesPerFrame() == flat.timing.TStatesPerFrame());
  machine.cpu.regs.PC = 0x9000; machine.bus.Write(0x8000, 0x42);
  assert(machine.LoadROM(FindROM("plus3-0.rom"), error));
  assert(machine.Configuration().model==MachineModel::Plus3 && machine.ula.Timing().tstatesPerLine==228);
  machine.ula.Configure(MachineModel::Spectrum48);machine.Reset();
  assert(machine.ula.Timing().tstatesPerLine==228 && machine.cpu.regs.PC==0);
  for (const char *file : {"128-0.rom", "128-spanish-0.rom", "plus2-0.rom"}) {
    assert(machine.LoadROM(FindROM(file), error));
    assert(machine.ula.ay.Enabled() && machine.ula.Timing().tstatesPerLine == 228);
    machine.io.Write(0x7FFD, 0x3F);
    machine.Reset();
    assert(machine.memory.PagingLatch() == 0 && machine.cpu.regs.PC == 0);
    assert(machine.memory.RAMBank(0) == 5 && machine.memory.RAMBank(1) == 2);
  }
  puts("Hardware profiles: model facts, ROM requirements, capabilities, ULA wiring and reset passed");
}
