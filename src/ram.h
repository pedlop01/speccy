#ifndef RAM_H
#define RAM_H

#include <cstring>
#include "bus.h"

template<unsigned int B, unsigned int S> class RAM: public BusComponent<B, S> {

  protected:
    unsigned char data[S];

  public:
    RAM();
    ~RAM();

    void Write(unsigned int address, unsigned char value);
    unsigned char Read(unsigned int address);
};

#endif
