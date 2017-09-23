#include <stdio.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

int main(int argc, char *argv[]) {
  
  // Components
  Z80 cpu;
  ULA ula;
  Bus16 bus;
  ROM<0, 16384> rom;
  RAM<32768, 32*1024> ram;

  // Load ROM contents
  if(rom.Load("48.rom")) {
    exit(-1);
  }

  // Populate busses
  bus.AddBusComponent(&rom);
  bus.AddBusComponent(&ram);
  bus.AddBusComponent((ULAMemory*)&ula);

  // And attach busses to cpu core
  cpu.DataBus = &bus;
  cpu.IOBus = (ULAIO*)&ula;

  cpu.regs.PC = 0;

  // Main loop
  do {
    // Emulate instructions
    cpu.EmulateOne();
  } while(true);
}

