#include "bus.h"

template<unsigned int B, unsigned int S>
int Bus<B,S>::AddBusComponent(BusComponentBase *newComponent) {

    for (unsigned int dd = 0; dd < m_components.size(); dd++) {
        if (m_components.at(dd) == newComponent )
          return -1;
    }
    m_components.push_back(newComponent);
    OnComponentsUpdated();
    return 0;
}

template<unsigned int B, unsigned int S>
int Bus<B,S>::RemoveBusComponent(BusComponentBase *component) {

  for (unsigned int dd = 0; dd < m_components.size(); dd++) {
      if (m_components.at(dd) == component) {
          m_components.erase(m_components.begin()+dd);
          OnComponentsUpdated();
          return 0;
      }
  }

  return -1;
}


Bus16::Bus16() {
    memset(m_pagedComponents, 0, sizeof(m_pagedComponents));
}

void Bus16::Write(unsigned int address, unsigned char value) {
    BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF)/1024];
    if (pComp)
        pComp->Write(address, value);
}

unsigned char Bus16::Read(unsigned int address) {
    BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF)/1024];
    return pComp ? pComp->Read(address) : 0xFF;
}

void Bus16::OnComponentsUpdated() {
    // Build a quick lookup table for each component. Address
    // space is split in 1 KB segment (64 entries).
    for (unsigned int dd = 0; dd < m_components.size(); dd++) {
        BusComponentBase *pComp = m_components.at(dd);
        if (pComp) {
            unsigned int start = pComp->GetStartAddress() / 1024;
            unsigned int end = start + (pComp->GetRegionSize() / 1024);
            for (unsigned int zz = start; zz < end; zz++)
                m_pagedComponents[zz] = pComp;
        }
    }
}
