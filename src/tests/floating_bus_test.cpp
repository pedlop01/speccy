#include <cassert>
#include <cstdio>
#include "../ula.h"
#include "../z80.h"

struct TestULA : ULA {
  void At(unsigned int tstate) { dwFrameTStates = tstate; }
};

int main() {
  TestULA ula;
  BusComponentBase *memory = static_cast<ULAMemory *>(&ula);
  // Distinct, non-idle values expose both address and bitmap/attribute mistakes.
  for (unsigned address = 0x4000; address < 0x5B00; ++address)
    memory->Write(address, (address * 13 + (address >> 8)) % 255);
  ula.At(14337);
  assert(ula.IORead(0xFFFF) == 0xFF);
  for (unsigned y = 0; y < 192; ++y) {
    for (unsigned pair = 0; pair < 16; ++pair) {
      unsigned start = 14338 + y * 224 + pair * 8;
      for (unsigned slot = 0; slot < 8; ++slot) {
        ula.At(start + slot);
        unsigned char expected = 0xFF;
        if (slot < 4) {
          unsigned column = pair * 2 + slot / 2;
          unsigned address = slot % 2
              ? 0x5800 + (y / 8) * 32 + column
              : 0x4000 + (y / 64) * 2048 + (y % 8) * 256 +
                  ((y % 64) / 8) * 32 + column;
          expected = memory->Read(address);
        }
        assert(ula.IORead(0xFFFF) == expected);
        assert(ula.IORead(0x00FF) == expected); // High port byte is irrelevant.
      }
    }
    ula.At(14338 + y * 224 + 128);
    assert(ula.IORead(0xFFFF) == 0xFF); // Horizontal blanking.
  }
  ula.At(14338 + 192 * 224);
  assert(ula.IORead(0xFFFF) == 0xFF); // Lower border.
  ula.At(TSTATES_PER_FRAME - 1);
  assert(ula.IORead(0xFFFF) == 0xFF);
  ula.At(0);
  assert(ula.IORead(0xFFFF) == 0xFF);
  ula.PressKey(0, 0, true);
  assert(ula.IORead(0xF7FE) == 0xFE); // Even ports still read the keyboard.
  ula.PressKey(0, 0, false);
  assert(ula.IORead(0xF7FE) == 0xFF);

  // Cobra's exact wait loop at 95FE: LD BC,3F28 / LD A,C /
  // IN A,(FF) / CP B / JR NC,9601. A constant FF never exits this loop.
  ULA gameULA;
  RAM<32768, 32768> ram;
  Bus16 bus;
  bus.AddBusComponent(static_cast<ULAMemory *>(&gameULA));
  bus.AddBusComponent(&ram);
  const unsigned char loop[] = {0x01,0x28,0x3F,0x79,0xDB,0xFF,0xB8,0x30,0xFA};
  for (unsigned i = 0; i < sizeof(loop); ++i) bus.Write(0x95FE + i, loop[i]);
  Z80 cpu;
  cpu.DataBus = &bus;
  cpu.IOBus = static_cast<ULAIO *>(&gameULA);
  cpu.regs.PC = 0x95FE;
  unsigned elapsed = 0;
  while (cpu.regs.PC != 0x9607 && elapsed < TSTATES_PER_FRAME) {
    cpu.tStates = 0;
    cpu.EmulateOne();
    bool irq = false;
    gameULA.AddCycles(cpu.tStates, irq);
    elapsed += cpu.tStates;
  }
  assert(cpu.regs.PC == 0x9607 && cpu.regs.CF);
  assert(elapsed >= 14338 && elapsed < TSTATES_PER_FRAME);
  puts("Floating bus: fetches, blanking, keyboard and Cobra wait loop passed");
}
