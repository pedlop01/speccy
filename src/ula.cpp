//REVISIT: to remove
#include <stdio.h>
#include "ula.h"

ULA::ULA() {

}

ULA::~ULA() {

}

void ULA::MemoryWrite(unsigned int address, unsigned char value) {
  unsigned int row = (address >> 5);
  row = ((row & 0xC0) |
         ((row & 0x38) >> 3) |
         ((row & 0x07) << 3));

  unsigned int column = (address & 0x001F) << 3;

  for (unsigned int p = 0; p < 8; p++)
    if (value & (0x01 << (7 - p))) {
      printf("ULA: x=%d, y=%d, value=%d\n", column + p + 32, row + 24, value);
//            al_put_pixel(column+j+32, row + 24, al_map_rgb(0x255, 0x255, 0x255));
    }
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
