#include "ula.h"

ULA::ULA() {

}

ULA::~ULA() {

}

void ULA::MemoryWrite(unsigned int address, unsigned char value) {

}

void ULA::IOWrite(unsigned int address, unsigned char value) {
  // ULA is selected when reading an even address
  if (address & 0x01)
    return;
  // Update boder color
  dwBorderRGBColor = dwColorTable[value & 0x07];
}

unsigned char ULA::IORead(unsigned int address) {
  return (0xFF); // pull up on bus
}
