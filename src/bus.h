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
    int AddBusComponent(BusComponentBase *newComponent);
    int RemoveBusComponent(BusComponentBase *component);

protected:
    vector<BusComponentBase*> m_components;

    virtual void OnComponentsUpdated() {};
};

class Bus16 : public Bus<0x0000, 0x10000> {
public:
    Bus16();

    void Write(unsigned int address, unsigned char value);
    unsigned char Read(unsigned int address);

protected:
    BusComponentBase* m_pagedComponents[64];
    virtual void OnComponentsUpdated();
};
