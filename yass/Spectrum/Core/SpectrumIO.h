/////////////////////////////////////////////////////////////////////////
// SpectrumIO.h
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
//
//	This file implements a ZX Spectrum I/O bus component, and handles the
//	I/O operations of the system.
//	Currently, the class supports a bus with an ULA and a Kempston interface
//	attached. This is the class you should update to add for more system
//	devices.

#pragma once

#include "BusComponents.h"
#include "ULA.h"
#include "Kempston.H"

#pragma unmanaged

class SpectrumIOBus : public BusComponent<0x00,0x10000>
{
public:
  SpectrumIOBus()
  {
    ULA = NULL;
    Kempston = NULL;
  }

  unsigned char Read(unsigned int address)
  {
    // Kempston interface is addressed by A5 == 0
    if (Kempston && ((address & 0x20) == 0))
      return(Kempston->Read(address));

    // ULA is addressed by A0 == 0
    if (ULA && ((address & 0x01) == 0))
    return(ULA->Read(address));

    // No device
    return(0xFF);
  }

  void Write(unsigned int address,unsigned char value)
  {
    // ULA is addressed by A0 == 0
    if (ULA && ((address & 0x01) == 0))
      return(ULA->Write(address,value));
  }

public:
  BusComponentBase *ULA;
  BusComponentBase *Kempston;
};
