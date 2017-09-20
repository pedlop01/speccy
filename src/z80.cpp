#include "z80.h"

Z80::Z80 () {
    memset(&regs, 0, sizeof(regs));
    DataBus = nullptr;
    IOBus = nullptr;
    tStates = 0;
}

void Z80 :: EmulateOne() {
    // Fetch next instruction
    unsigned char op = DataBus->Read(regs.PC++);
    // Increment instruction counter register
    regs.R60++;

    switch(op) {
        // nop
        case 0x00:
          tStates += 4;
          break;

        // ld bc, NN
        case 0x01:
          tStates += 10;
          regs.C = DataBus->Read(regs.PC++);
          regs.B = DataBus->Read(regs.PC++);
          break;

        // ld (bc), a
        case 0x02:
          tStates += 7;
          DataBus->Write(regs.BC, regs.A);
          break;

        // inc bc
        case 0x03:
          tStates += 6;
          regs.BC++;
          break;

        default:
          break;
    }
}