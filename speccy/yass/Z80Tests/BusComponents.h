/////////////////////////////////////////////////////////////////////////
// BusComponent.H
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
//	This file implements some widely used components:
//	RAM<Base,Size>: implements a RAM block
//	ROM<Base,Size>:	implements a ROM block that obtains it's contents
//									from a file on disk.

#pragma once
#include "Bus.h"

#pragma unmanaged

template<unsigned int B,unsigned int S> class RAM : public BusComponent<B,S>
{
public:
	RAM()
	{
		ZeroMemory(data,S);
	}
	void Write(unsigned int address,unsigned char value)
	{
		data[address-B] = value;
	}
	unsigned char Read(unsigned int address)
	{
		return(data[address-B]);
	}

protected:
  unsigned char data[S];
};


template<unsigned int B,unsigned int S> class ROM : public BusComponent<B,S>
{
public:
	int Load(char *romName)
  {
    FILE *file = NULL;
    int err = fopen_s(&file,romName,"rb");
    if ((err == 0) && (file != NULL))
    {
      int got = fread(data,1,S,file);
      fclose(file);
    }
		return(err);
  };

  public:
  void Write(unsigned int address,unsigned char value)
  {
		// Do nothing... this is ROM!!! ;)
  }
  unsigned char Read(unsigned int address)
  {
    return(data[address-B]);
  }

protected:
  unsigned char data[S];
};
