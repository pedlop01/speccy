#include <cassert>
#include <cstdio>
#include "../emulator.h"

int main() {
  ULA ula;
  SpectrumMemory memory(ula);
  SpectrumIO io(ula, memory);
  std::string error;
  ROMSet rom;
  assert(LoadROMSet(FindROM("128-0.rom"), rom, error));
  assert(memory.Configure(rom, error));
  assert(memory.RAMBank(0) == 5 && memory.RAMBank(1) == 2 && memory.RAMBank(2) == 0);
  assert(memory.ROMPage() == 0 && memory.ScreenBank() == 5 && !memory.PagingLocked());

  // Every byte of every bank must survive switches, including window boundaries.
  for (unsigned bank = 0; bank < 8; ++bank) {
    io.Write(0x7FFD, bank);
    for (unsigned n = 0; n < 16384; ++n) memory.Write(0xC000 + n, (n * 13 + bank * 29) & 255);
  }
  for (unsigned bank = 0; bank < 8; ++bank) {
    io.Write(0x7FFD, bank);
    for (unsigned n = 0; n < 16384; ++n) {
      assert(memory.Read(0xC000 + n) == ((n * 13 + bank * 29) & 255));
      if (bank == 2) assert(memory.Read(0x8000 + n) == memory.Read(0xC000 + n));
      if (bank == 5) assert(memory.Read(0x4000 + n) == memory.Read(0xC000 + n));
    }
  }
  for (unsigned bank : {2u, 5u}) {
    io.Write(0x7FFD, bank);
    unsigned fixed = bank == 2 ? 0x8000 : 0x4000;
    memory.Write(fixed + 13, 0xAB); assert(memory.Read(0xC00D) == 0xAB);
    memory.Write(0xC00D, 0xCD); assert(memory.Read(fixed + 13) == 0xCD);
  }
  // Writes through the bank-5 alias also reach the ULA's canonical video bytes.
  io.Write(0x7FFD, 5);
  memory.Write(0xC000, 0x7B);
  assert(static_cast<BusComponentBase *>(static_cast<ULAMemory *>(&ula))->Read(0x4000) == 0x7B);

  for (unsigned page = 0; page < 2; ++page) {
    io.Write(0x7FFD, page << 4);
    for (unsigned n = 0; n < 16384; ++n) {
      memory.Write(n, 0xA5);
      assert(memory.Read(n) == rom.pages[page][n]);
    }
  }
  // Exhaust all aliases and rejected addresses without resetting the RAM.
  for (unsigned port = 0; port < 65536; ++port) {
    io.Write(0x7FFD, 0);
    io.Write(port, 0x1B);
    assert(memory.PagingLatch() == ((port & 0x8002) ? 0 : 0x1B));
  }
  io.Write(0x7FFC, 0x3F); // Even aliases also update the ULA beeper/border.
  assert(memory.PagingLocked() && memory.RAMBank(2) == 7);
  assert(memory.ROMPage() == 1 && memory.ScreenBank() == 7);
  for (unsigned value = 0; value < 256; ++value) io.Write(0x7FFD, value);
  assert(memory.PagingLatch() == 0x3F);
  assert(io.Read(0x7FFD) == ula.IORead(0x7FFD)); // No latch readback.
  memory.Reset();
  assert(!memory.PagingLocked() && memory.PagingLatch() == 0 && memory.ROMPage() == 0);
  for (unsigned bank = 0; bank < 8; ++bank)
    for (unsigned n = 0; n < 16384; ++n) assert(memory.ReadBank(bank, n) == 0);
  assert(memory.Read(0) == rom.pages[0][0]);

  // Invalid configurations leave ROM, RAM and latch untouched.
  memory.Write(0xC000, 0x91); io.Write(0x7FFD, 0x20);
  ROMSet invalid = rom; invalid.pages.pop_back();
  assert(!memory.Configure(invalid, error));
  assert(memory.PagingLocked() && memory.Read(0xC000) == 0x91);
  assert(LoadROMSet(FindROM("plus3-0.rom"), invalid, error));
  assert(memory.Configure(invalid, error) && !memory.PagingLocked());

  assert(LoadROMSet(FindROM("48.rom"), rom, error));
  assert(memory.Configure(rom, error));
  for (unsigned value = 0; value < 256; ++value) io.Write(0x7FFD, value);
  assert(memory.PagingLatch() == 0 && memory.ScreenBank() == 0);
  memory.Write(0x4000, 1); memory.Write(0x8000, 2); memory.Write(0xC000, 3);
  assert(memory.Read(0x4000) == 1 && memory.Read(0x8000) == 2 && memory.Read(0xC000) == 3);

  // Real ROM boot through the same memory and I/O components used by the machine.
  // Full 128K selection remains gated until video/timing and AY are implemented.
  for (const char *file : {"128-0.rom", "128-spanish-0.rom", "plus2-0.rom"}) {
    assert(LoadROMSet(FindROM(file), rom, error));
    assert(memory.Configure(rom, error));
    ula.Configure(rom.model);
    Z80 cpu;
    cpu.DataBus = &memory; cpu.IOBus = &io;
    for (unsigned frame = 0; frame < 200; ++frame) {
      bool irq = false;
      while (!irq) {
        cpu.tStates = 0; cpu.EmulateOne(); ula.AddCycles(cpu.tStates, irq);
      }
      cpu.tStates = 0; cpu.INT(); bool unused = false; ula.AddCycles(cpu.tStates, unused);
    }
    unsigned nonzero = 0;
    for (unsigned n = 0; n < 6144; ++n) nonzero += memory.ReadBank(5, n) != 0;
    printf("%s: PC=%04X latch=%02X screen bytes=%u\n", file, cpu.regs.PC, memory.PagingLatch(), nonzero);
    assert(nonzero > 100);
    std::string text;
    for (unsigned row = 0; row < 24; ++row) {
      for (unsigned col = 0; col < 32; ++col) {
        char match = '?';
        for (unsigned ch = 32; ch < 127; ++ch) {
          bool same = true;
          for (unsigned line = 0; line < 8; ++line) {
            unsigned y = row * 8 + line;
            unsigned offset = ((y & 0xC0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | col;
            if (memory.ReadBank(5, offset) != rom.pages[1][0x3C00 + ch * 8 + line]) same = false;
          }
          if (same) { match = ch; break; }
        }
        text += match;
      }
      text += '\n';
    }
    if (std::string(file) == "128-spanish-0.rom") {
      // The original Spanish ROM enters BASIC directly, without the UK menu.
      assert(text.find("1985 Sinclair Research Ltd") != std::string::npos);
      assert(text.find("ESPA") != std::string::npos);
    } else {
      assert(text.find("128 BASIC") != std::string::npos);
      assert(text.find("48 BASIC") != std::string::npos);
    }
    assert(!memory.PagingLocked());
  }
  puts("Paging: banks, aliases, ROM protection, port decoding, lock, reset and real ROM boot passed");
}
