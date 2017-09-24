#ifndef ULA_H
#define ULA_H

#include <allegro5/allegro.h>

#include "ram.hpp"

// Spectrum color table to RGBA
static const unsigned int dwColorTable[] =
{
  0x0000FF00,
  0x0000FFCD,
  0x00CDFF00,
  0x00CDFFCD,
  0xCD00FF00,
  0xCD00FFCD,
  0xCDCDFF00,
  0xCDCDFFCD,

  0x0000FF00,
  0x0000FFFF,
  0x00FFFF00,
  0x00FFFFFF,
  0xFF00FF00,
  0xFF00FFFF,
  0xFFFFFF00,
  0xFFFFFFFF
};

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
  protected:
    unsigned int dwBorderRGBColor;

  public:
    ULA();
    ~ULA();

    void MemoryWrite(unsigned int address, unsigned char value);
    void IOWrite(unsigned int address, unsigned char value);
    unsigned char IORead(unsigned int address);

    unsigned char MemRead(unsigned int address) {
      return ULAMemory::Read(address);
    };
};

#endif
