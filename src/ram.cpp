#include "ram.h"

template<unsigned int B, unsigned int S>
RAM<B, S> :: RAM() {
    memset(data, 0, S);
}

template<unsigned int B, unsigned int S>
RAM<B, S> :: ~RAM() {
}

template<unsigned int B, unsigned int S>
void RAM<B, S> :: Write(unsigned int address, unsigned char value) {
    data[address-B] = value;
}

template<unsigned int B, unsigned int S>
unsigned char RAM<B, S> :: Read(unsigned int address) {
    return data[address-B];
}

