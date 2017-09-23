#ifndef ULA_H
#define ULA_H

#include "ram.hpp"

class ULAMemory : public RAM<16384, 16*1024> {
  protected:
    void Write(unsigned int address, unsigned char value) {
        // First, store data
        RAM::Write(address, value);
        // and forward the data to the ULA
        MemoryWrite(address, value);
    };

    virtual void MemoryWrite(unsigned int address, unsigned char value) = 0;
};

class ULAIO : public BusComponent<0xFE, 1> {
  protected:
    unsigned char Read(unsigned int address) {
        return (IORead(address));
    };

    void Write(unsigned int address, unsigned char value) {
        IOWrite(address, value);
    };

    virtual unsigned char IORead(unsigned int address) = 0;
    virtual void IOWrite(unsigned int address, unsigned char value) = 0;
};

class ULA : public ULAMemory, ULAIO {
  public:
    ULA();
    ~ULA();

    void MemoryWrite(unsigned int address, unsigned char value);
    void IOWrite(unsigned int address, unsigned char value);
    unsigned char IORead(unsigned int address);
};

#endif
