#ifndef ULA_H
#define ULA_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>


#include "ram.hpp"

#define TSTATES_PER_SCANLINE 224
#define TVSCANLINES          312
#define TSTATES_PER_FRAME    TSTATES_PER_SCANLINE*TVSCANLINES

// REVISIT: need to be written in allegro format
// Spectrum color table to RGBA
static const unsigned int dwColorTable[] =
{
  0x00000000,
  0x0000D700,
  0xD7000000,
  0xD700D700,
  0x00D70000,
  0x00D7D700,
  0xD7D70000,
  0xD7D7D700,

  0x00000000,
  0x0000FF00,
  0xFF000000,
  0xFF00FF00,
  0x00FF0000,
  0x00FFFF00,
  0xFFFF0000,
  0xFFFFFF00
};

class ULAMemory : public RAM<16384, 16*1024> {
  protected:
    void Write(unsigned int address, unsigned char value) {
        // First, store data
        RAM<16384,16*1024>::Write(address, value);
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
    bool            isDirty;
    ALLEGRO_BITMAP* bitmap;
    bool            blinkState;
    unsigned int    dwBorderRGBColor;
    unsigned int    dwCurrentScanLineBackColor[312];
    unsigned long   dwFrameTStates;
    unsigned long   dwScanLineTStates;
    unsigned long   dwScanLine;
    unsigned long   dwFrameCount;

  public:
    ULA();
    ~ULA();

    void SetBitmap(ALLEGRO_BITMAP *_bitmap);
    void UpdateChar(unsigned int nChar);
    void AddCycles(unsigned int cycles, bool& IRQ);
    void ScanLine(bool &IRQ);
    void UpdateBlink();
    void MemoryWrite(unsigned int address, unsigned char value);
    void IOWrite(unsigned int address, unsigned char value);
    unsigned char IORead(unsigned int address);
};

#endif
