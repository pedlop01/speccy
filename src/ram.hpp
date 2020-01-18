#ifndef RAM_H
#define RAM_H

#include <stdio.h>
#include <cstring>
#include "bus.hpp"

template<unsigned int B, unsigned int S>
class RAM : public BusComponent<B,S> {

  protected:
    unsigned char data[S];

  public:
    RAM() {
      memset(data, 0, S);
    };
    ~RAM() {};

    void Write(unsigned int address, unsigned char value) {
      data[address-B] = value;
    };

    unsigned char Read(unsigned int address) {
      return data[address-B];
    };
};

#endif

