#include <cassert>
#include <cstdio>
#include "../z80.h"
#include "../ram.hpp"

int main() {
  RAM<0, 65536> ram;
  ram.Write(0x100, 0xED);
  // Exhaust both operands, both initial flag patterns and BC terminal/wrap
  // states. P/V reflects the remaining count, not arithmetic overflow.
  for (unsigned op : {0xA1u, 0xA9u, 0xB1u, 0xB9u}) {
    ram.Write(0x101, op);
    for (unsigned a = 0; a < 256; ++a) {
      for (unsigned value = 0; value < 256; ++value) {
        ram.Write(0x8000, value);
        for (unsigned count : {0u, 1u, 2u}) {
          for (unsigned flags : {0u, 255u}) {
            Z80 cpu; cpu.DataBus = &ram;
            cpu.regs.AF = (a << 8) | flags;
            cpu.regs.BC = count; cpu.regs.HL = 0x8000; cpu.regs.PC = 0x100;
            cpu.EmulateOne();
            const unsigned result = (a - value) & 255;
            const unsigned left = (count - 1) & 65535;
            unsigned expected = (flags & 1) | 2 | (result & 0x80);
            if (a == value) expected |= 0x40;
            if ((a & 15) < (value & 15)) expected |= 0x10;
            if (left) expected |= 4;
            assert((cpu.regs.AF & 0xD7) == expected);
            assert(cpu.regs.A == a && cpu.regs.BC == left);
            assert(cpu.regs.HL == ((op & 8) ? 0x7FFF : 0x8001));
            assert(ram.Read(0x8000) == value); // Compare never changes memory.
            const bool repeat = (op & 0x10) && left && a != value;
            assert(cpu.regs.PC == (repeat ? 0x100 : 0x102));
            assert(cpu.tStates == (repeat ? 21u : 16u));
          }
        }
      }
    }
  }
  // Forward/backward repeated searches must stop at a nonzero match, and
  // must not stop on an unrelated zero byte (the Bomb Jack text-code case).
  for (unsigned op : {0xB1u, 0xB9u}) {
    Z80 cpu; cpu.DataBus = &ram;
    ram.Write(0x101, op);
    const int direction = (op & 8) ? -1 : 1;
    ram.Write(0x8000, 0); ram.Write(0x8000 + direction, 0x16);
    ram.Write(0x8000 + direction * 2, 0x42);
    cpu.regs.PC = 0x100; cpu.regs.HL = 0x8000;
    cpu.regs.A = 0x16; cpu.regs.BC = 3; cpu.regs.CF = 1;
    cpu.EmulateOne(); assert(cpu.regs.PC == 0x100 && !cpu.regs.ZF);
    cpu.EmulateOne(); assert(cpu.regs.PC == 0x102 && cpu.regs.ZF && cpu.regs.CF);
    assert(cpu.regs.BC == 1 && cpu.regs.HL == 0x8000 + direction * 2);
  }
  puts("Z80 block compare: exhaustive CPI/CPD/CPIR/CPDR flags, counters, carry and search termination passed");
}
