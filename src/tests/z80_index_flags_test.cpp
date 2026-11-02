#include <cassert>
#include <cstdio>
#include "../z80.h"
#include "../ram.hpp"

int main() {
  RAM<0, 65536> ram;
  // DD/FD byte INC/DEC must set S,Z,H,P/V,N and preserve carry just like
  // normal 8-bit INC/DEC. Exhaust the operand and both initial flag patterns.
  for (unsigned prefix : {0xDDu, 0xFDu}) {
    for (unsigned op : {0x24u, 0x25u, 0x2Cu, 0x2Du}) {
      for (unsigned value = 0; value < 256; ++value) {
        for (unsigned flags : {0u, 255u}) {
          Z80 cpu;
          cpu.DataBus = &ram;
          cpu.regs.AF = 0x5A00 | flags;
          cpu.regs.IX = 0xA55A; cpu.regs.IY = 0xB469;
          cpu.regs.HL = 0x8765;
          bool high = op < 0x2C;
          bool decrement = op & 1;
          unsigned initial = high ? ((value << 8) | 0x69) : (0xB400 | value);
          if (prefix == 0xDD) cpu.regs.IX = initial;
          else cpu.regs.IY = initial;
          ram.Write(0, prefix); ram.Write(1, op);
          cpu.EmulateOne();
          unsigned result = (value + (decrement ? 255 : 1)) & 255;
          unsigned expected = (flags & 1) | (result & 0x80) | (result == 0 ? 0x40 : 0);
          if (decrement) {
            expected |= 2;
            if (!(value & 15)) expected |= 0x10;
            if (value == 0x80) expected |= 4;
          } else {
            if ((value & 15) == 15) expected |= 0x10;
            if (value == 0x7F) expected |= 4;
          }
          assert((cpu.regs.AF & 0xD7) == expected);
          unsigned index = prefix == 0xDD ? cpu.regs.IX : cpu.regs.IY;
          assert(index == (high ? ((result << 8) | 0x69) : (0xB400 | result)));
          assert((prefix == 0xDD ? cpu.regs.IY : cpu.regs.IX) == (prefix == 0xDD ? 0xB469 : 0xA55A));
          assert(cpu.regs.HL == 0x8765 && cpu.regs.A == 0x5A);
          assert(cpu.regs.PC == 2 && cpu.tStates == 8 && cpu.regs.R60 == 2);
        }
      }
    }
  }
  // Byte-counted memory copy: a stale Z flag must not let the loop overwrite
  // the sentinel after its intended three-byte output (Capitan Trueno's trigger).
  const unsigned char program[] = {
    0xFD,0x2E,3,        // LD IYL,3
    0x21,0x00,0x80,    // LD HL,8000
    0x36,0xA5,         // LD (HL),A5
    0x23,              // INC HL (preserves flags)
    0xFD,0x2D,         // DEC IYL
    0xC2,6,0,          // JP NZ,loop
    0x76
  };
  for (unsigned i = 0; i < sizeof(program); ++i) ram.Write(i, program[i]);
  ram.Write(0x8003, 0x42);
  Z80 cpu; cpu.DataBus = &ram;
  for (unsigned i = 0; cpu.regs.PC != 14 && i < 100; ++i) cpu.EmulateOne();
  assert(cpu.regs.PC == 14 && cpu.regs.HL == 0x8003 && (cpu.regs.IY & 255) == 0);
  assert(ram.Read(0x8000) == 0xA5 && ram.Read(0x8002) == 0xA5 && ram.Read(0x8003) == 0x42);
  puts("Z80 index bytes: exhaustive INC/DEC flags, carry preservation and terminating copy loop passed");
}
