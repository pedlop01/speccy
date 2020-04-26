#ifndef BUS_H
#define BUS_H

#include <vector>
#include <cstring>

using namespace std;

class BusComponentBase {
public:
    virtual unsigned int  GetStartAddress() = 0;
    virtual unsigned int  GetRegionSize() = 0;
    virtual void          Write(unsigned int address, unsigned char value) = 0;
    virtual unsigned char Read(unsigned int address) = 0;
};

template<unsigned int B, unsigned int S> class BusComponent : public BusComponentBase {
public:
    virtual unsigned int GetStartAddress() { return(B); }
    virtual unsigned int GetRegionSize()   { return(S); }
};

template<unsigned int B, unsigned int S> class Bus : public BusComponent<B, S> {
public:
    int AddBusComponent(BusComponentBase *newComponent) {
      for (unsigned int dd = 0; dd < m_components.size(); dd++) {
          if (m_components.at(dd) == newComponent )
            return -1;
      }
      m_components.push_back(newComponent);
      OnComponentsUpdated();
      return 0;
    };

    int RemoveBusComponent(BusComponentBase *component) {
      for (unsigned int dd = 0; dd < m_components.size(); dd++) {
          if (m_components.at(dd) == component) {
              m_components.erase(m_components.begin()+dd);
              OnComponentsUpdated();
              return 0;
          }
      }

      return -1;
    };

protected:
    vector<BusComponentBase*> m_components;

    virtual void OnComponentsUpdated() {};
};

class Bus16 : public Bus<0x0000, 0x10000> {
public:
    Bus16() {
      memset(m_pagedComponents, 0, sizeof(m_pagedComponents));
    };

    void Write(unsigned int address, unsigned char value) {
      BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF)/1024];
      if (pComp)
          pComp->Write(address & 0xFFFF, value);
      };

    unsigned char Read(unsigned int address) {
      BusComponentBase *pComp = m_pagedComponents[(address & 0xFFFF)/1024];
      return pComp ? pComp->Read(address & 0xFFFF) : 0xFF;
    };

protected:
    BusComponentBase* m_pagedComponents[64];

    virtual void OnComponentsUpdated() {
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
    };
};

#endif

