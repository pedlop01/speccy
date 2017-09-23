#include <stdio.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"

int main(int argc, char *argv[]) {
// printf("\033[8;5Hhello");  // Move to (8, 5) and output hello
// printf("\033[XA"); // Move up X lines;
// printf("\033[XB"); // Move down X lines;
// printf("\033[XC"); // Move right X column;
// printf("\033[XD"); // Move left X column;
// printf("\033[2J"); // Clear screen

  printf("Z80 cpu test\n");

  // Components
  Z80 cpu;
  RAM<0,1024> ram;
  Bus16 dataBus;

  // Link components to bus
  dataBus.AddBusComponent(&ram);

  // Link bus to cpu
  cpu.DataBus = &dataBus;

  unsigned char testProg[] = {
    0x21, 0x00, 0x02,             // 0x0000 LD HL, #0200
    0x34,                         // 0x0003 INC (HL)
    0xC3, 0x03, 0x00              // 0x0004 JP 0003
  };

  unsigned short ptr = 0;
  for (int dd = 0; dd < sizeof(testProg); dd++)
    cpu.DataBus->Write(ptr++, testProg[dd]);

  cpu.DataBus->Write(0x0200, 0x23);

  cpu.regs.PC = 0x0000;
  char key = 'a';
  do {
    printf("Cycles   PC   AF   HL   Mem(0x200)\n");
    printf("%d    %04X %04X %04X %02X\n",
      cpu.tStates,
      cpu.regs.PC, cpu.regs.AF,
      cpu.regs.HL, ram.Read(0x0200));

    //scanf("%c\n", &key);
    //if (key == 'a') {
    //  printf("Emulate instruction\n");
      cpu.EmulateOne();
    //}
  } while(key == 'a');
}

