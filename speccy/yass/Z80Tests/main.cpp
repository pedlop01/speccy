/////////////////////////////////////////////////////////////////////////
// main.cpp
//	Copyright (c) 2012, Fernando Marcos
//	All rights reserved.
//	
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met: 
//	
//	1. Redistributions of source code must retain the above copyright notice, this
//	   list of conditions and the following disclaimer. 
//	2. Redistributions in binary form must reproduce the above copyright notice,
//	   this list of conditions and the following disclaimer in the documentation
//	   and/or other materials provided with the distribution. 
//	
//	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
//	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

#include "stdafx.h"
#include "BusComponents.h"
#include "Z80.h"
#include <conio.h>

int _tmain(int argc, _TCHAR* argv[])
{
  printf("Z80 cpu tests\n");

  // Components for tests
  Z80 cpu;						// processor
  RAM<0,1024> ram;		// 1KB of RAM
  Bus16 dataBus;			// Main data bus

  // attach components to bus
  dataBus.AddBusComponent(&ram);

  // attach bus to cpu
  cpu.DataBus = &dataBus;

  // Load test program at RAM start
  // This test program simply updates contents of memory location 0x0200
  unsigned short ptr = 0;
  ram.Write(ptr++,0x21);		// 0x0000 21 00 02  LD HL,#0200
  ram.Write(ptr++,0x00);
  ram.Write(ptr++,0x02);
  ram.Write(ptr++,0x34);		// 0x0003 34        INC (HL)
  ram.Write(ptr++,0xC3);		// 0x0004 C3 03 00  JP 0003
  ram.Write(ptr++,0x03);
  ram.Write(ptr++,0x00);

  ram.Write(0x0200,0x23);		// Set initial value

  cpu.regs.PC = 0x0000;

  int key = 0;
  do
  {
    printf("Cycles   PC   AF   HL   Mem(0x200)\n");
    printf("%8u %04X %04X %04X %02X\n",cpu.tStates,cpu.regs.PC,cpu.regs.AF,cpu.regs.HL,ram.Read(0x0200));

    key = _getch();
    if (key == 0x20)
      cpu.EmulateOne();
  } while(key == 0x20);		// repeat while Space key pressed

  return 0;
}

