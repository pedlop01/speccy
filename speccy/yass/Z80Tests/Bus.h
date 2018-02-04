/////////////////////////////////////////////////////////////////////////
// Bus.h
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
//	Base classes to implement hardware busses and components attached
//	to those busses.

#pragma once
#include <vector>

using namespace std;

#pragma unmanaged

/////////////////////////////////////////////////////////////////////////
// BusComponentBase
//	This class defines any object that can be memory/IO addressed on a
//	system.

class BusComponentBase
{
public:
  virtual unsigned int GetStartAddress() = 0;
  virtual unsigned int GetRegionSize() = 0;
  virtual void Write(unsigned int address,unsigned char value) = 0;
  virtual unsigned char Read(unsigned int address) = 0;
};

/////////////////////////////////////////////////////////////////////////
// BusComponent<Base,Size>
//	Templated version of the BusComponentBase class.

template<unsigned int B,unsigned int S> class BusComponent : public BusComponentBase
{
public:
  virtual unsigned int GetStartAddress() { return(B); }
  virtual unsigned int GetRegionSize() { return(S); }
};

/////////////////////////////////////////////////////////////////////////
// Bus<Base,Size>
//	The Bus<> class defines is a BusCoponentBase that allows the
//	attaching of several sub-components, much the same as a real, harware
//	bus.

template<unsigned int B,unsigned int S> class Bus : public BusComponent<B,S>
{
public:
  HRESULT AddBusComponent(BusComponentBase *newComponent)
  {
    for (unsigned int dd=0;dd<m_components.size();dd++)
    {
      if (m_components.at(dd) == newComponent)
        return(ERROR_ALREADY_EXISTS);
    }
    m_components.push_back(newComponent);
    OnComponentsUpdated();
    return(S_OK);
  }

  HRESULT RemoveBusComponent(BusComponentBase *component)
  {
    for (unsigned int dd=0;dd<m_components.size();dd++)
    {
      if (m_components.at(dd) == component)
      {
        m_components.erase(m_components.begin()+dd);
        OnComponentsUpdated();
        return(S_OK);
      }
    }
    return(ERROR_NOT_FOUND);
  }

protected:
  virtual void OnComponentsUpdated()
  {
  };

protected:
  vector<BusComponentBase*> m_components;
};


/////////////////////////////////////////////////////////////////////////
// Bus16
//	Complete 16 bit bus implementation. Several components can be
//	attached to this bus, and any read/write request will be forwarded
//	to the appropriate component.

class Bus16 : public Bus<0x0000,0x10000>
{
public:
  Bus16()
  {
    ZeroMemory(m_pagedComponents,sizeof(m_pagedComponents));
  }

public:
  void Write(unsigned int address,unsigned char value)
  {
    BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF) / 1024];
    if (pComp)
      pComp->Write(address,value);
  }
  unsigned char Read(unsigned int address)
  {
    BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF) / 1024];
    return(pComp ? pComp->Read(address) : 0xFF);
  }

protected:
  virtual void OnComponentsUpdated()
  {
    // Build a quick lookup table por each component. Address
    // space is split in 1 KiloByte segment (64 entries).
    for (unsigned int dd=0;dd<m_components.size();dd++)
    {
      BusComponentBase *pComp = m_components.at(dd);
      if (pComp)
      {
        unsigned int start = pComp->GetStartAddress() / 1024;
        unsigned int end = start + (pComp->GetRegionSize() / 1024);
        for (unsigned int zz = start;zz<end;zz++)
          m_pagedComponents[zz] = pComp;
      }
    }
  };

protected:
    BusComponentBase* m_pagedComponents[64];
};