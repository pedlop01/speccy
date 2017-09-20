#ifndef RAM_H
#define RAM_H

#include <cstring>
#include "bus.h"

template<unsigned int B, unsigned int S> class RAM: public BusComponent<B, S> {
public:
    RAM() { memset(data, 0, S); }

    void Write(unsigned int address, unsigned char value) { data[address-B] = value; }
    unsigned char Read(unsigned int address)              { return data[address-B]; }

protected:
    unsigned char data[S];
};

#endif
