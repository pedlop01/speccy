/////////////////////////////////////////////////////////////////////////
// Spectrum.h
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

#define KEMPSTON_RIGHT        0x01
#define KEMPSTON_LEFT         0x02
#define KEMPSTON_DOWN         0x04
#define KEMPSTON_UP           0x08
#define KEMPSTON_FIRE         0x10

class Kempston : public BusComponent<0x1F,1>
{
public:
	Kempston()
	{
		kempstonData = 0;
	}

public:
	void SetKempstonData(unsigned char data)
	{
		kempstonData = data;
	}

public:
	unsigned char Read(unsigned int address)
	{
		return(kempstonData);
	}

	void Write(unsigned int address,unsigned char value)
	{
		// Do nothing
	}

	unsigned char kempstonData;
};
