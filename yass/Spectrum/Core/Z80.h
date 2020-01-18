/////////////////////////////////////////////////////////////////////////
// Z80.h
//	Implementation of a Z80 processor.
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

#pragma once
#include "Bus.h"

#pragma unmanaged

struct Z80Registers
{
  // AF register pair
  union
  {
    unsigned short AF;
    struct
    {
      union
      {
        unsigned char F;    // LSB
        struct
        {
          unsigned char CF : 1;
          unsigned char NF : 1;
          unsigned char PF : 1;
          unsigned char XF : 1;
          unsigned char HF : 1;
          unsigned char YF : 1;
          unsigned char ZF : 1;
          unsigned char SF : 1;
        };
      };
      unsigned char A;    // MSB
    };
  };

  // BC register pair
  union
  {
    unsigned short BC;
    struct
    {
      unsigned char C;    // LSB
      unsigned char B;    // MSB
    };
  };

  // DE register pair
  union
  {
    unsigned short DE;
    struct
    {
      unsigned char E;    // LSB
      unsigned char D;    // MSB
    };
  };

  // HL register pair
  union
  {
    unsigned short HL;
    struct
    {
      unsigned char L;    // LSB
      unsigned char H;    // MSB
    };
  };
 
   // IR register pair
  union
  {
    unsigned short IR;
    struct
    {
      union
      {
        unsigned char R;    // LSB
        struct
        {
          unsigned char R60 : 7;
          unsigned char R7 : 1;
        };
      };
      unsigned char I;    // MSB
    };
  };

  unsigned short IX;
  unsigned short IY;
  unsigned short SP;
  unsigned short PC;

  unsigned short altAF;
  unsigned short altBC;
  unsigned short altDE;
  unsigned short altHL;

  // XX register pair
  union
  {
    unsigned short XX;
    struct
    {
      unsigned char LX;    // LSB
      unsigned char HX;    // MSB
    };
  };

  unsigned char iff1A;
  unsigned char iff1B;
  unsigned char IM;
};

class Z80
{
public:
  Z80()
  {
    ZeroMemory(&regs,sizeof(regs));
    DataBus = NULL;
		IOBus = NULL;
    tStates = 0;
  }

public:
  void EmulateOne();
  void INT();
  void NMI();

protected:
  void EmulateOneCB();
  void EmulateOneED();
  void EmulateOneXX();
  void EmulateOneXXCB();

protected:
	// ALU
	void ADC_R8(unsigned char v);
	void ADC_R16(unsigned short &r,unsigned short v);
	void ADD_R8(unsigned char v);
	void ADD_R16(unsigned short &r,unsigned short v);
	void INC_R8(unsigned char &r);
	void DEC_R8(unsigned char &r);
	void SUB_R8(unsigned char v);
	void SBC_R8(unsigned char &r,unsigned char v);
	void SBC_R16(unsigned short &r,unsigned short v);
	void AND_R8(unsigned char v);
	void CMP_R8(unsigned char v);
	void CMP_R8_NOFLAGS(unsigned char r,unsigned char v);
	void OR_R8(unsigned char v);
	void XOR_R8(unsigned char v);

  // Rotation
  void RLC8(unsigned char &val);
  void RRC8(unsigned char &val);
  void RL8(unsigned char &val);
  void RR8(unsigned char &val);
  void SLA8(unsigned char &val);
  void SRA8(unsigned char &val);
  void SRL8(unsigned char &val);
  void SLI8(unsigned char &val);    // Undocumented opcode

  // Bit checking, setting and clearing
  void BIT8(unsigned char val,unsigned char bit);

	// IO
	void IN8(unsigned char &val,unsigned short port);

protected:
  unsigned short Read16(unsigned short addr)
  {
    return(DataBus->Read(addr) | DataBus->Read(addr+1) << 8);
  }

  void Write16(unsigned short addr,unsigned short value)
  {
    DataBus->Write(addr,value & 0xFF);
    DataBus->Write(addr+1,value >> 8);
  }

  void Push16(unsigned short value)
  {
    regs.SP -= 2;
    Write16(regs.SP,value);
  }

  unsigned short Pop16()
  {
    unsigned short retVal = Read16(regs.SP);
    regs.SP += 2;
    return(retVal);
  }

public:
  BusComponent<0x0000,0x10000>* DataBus;
  BusComponent<0x0000,0x10000>* IOBus;
  Z80Registers regs;
  unsigned int tStates;
};