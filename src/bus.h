#include <vector>

class BusComponentBase {
public:
    virtual unsigned int  GetStartAddress() = 0;
    virtual unsigned int  GetRegionSize() = 0;
    virtual void          Write(unsigned in address, unsigned char value) = 0;
    virtual unsigned char Read(unsigned int address) = 0;
}

template<unsigned int B, unsigned int S> class BusComponent : public BusComponentBase {
public:
    virtual unsigned int GetStartAddress() { return(B); }
    virtual unsigned int GetRegionSize()   { return(S); }
}

template<unsigned int B, unsigned int S> class Bus : public BusComponent {
public:
    int AddBusComponent(BusComponentBase *newComponent);
    int RemoveBusComponent(BusComponentBase *component);

protected:
    vector<BusComponentBase*> m_components;

    virtual void OnComponentsUpdated() {};
}