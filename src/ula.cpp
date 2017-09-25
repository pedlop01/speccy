#include "ula.h"

ULA::ULA() {

}

ULA::~ULA() {

}

void ULA::MemoryWrite(unsigned int address, unsigned char value) {
  unsigned int ink;
  unsigned int paper;
  unsigned int row;
  unsigned int column;
  unsigned char attr;

  address &= 0x3FFF;
  if(address > 0x1AFF)
    return;

  if(address < 0x1800) {  // Bitmap graphics

    row = (address >> 5);
    row = ((row & 0xC0) |
           ((row & 0x38) >> 3) |
           ((row & 0x07) << 3));

    column = (address % 32) * 8;

    // Get the attribute
    attr = data[0x1800 + ((row >> 3) << 5) + (address & 0x001F)];

    if((attr & 0x80) /* && blinkState*/) {
      ink   = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
      paper = dwColorTable[(attr & 0x78) >> 3];
    } else {
      ink   = dwColorTable[(attr & 0x78) >> 3];
      paper = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
    }

    for (unsigned int p = 0; p < 8; p++) {
      bool is_ink = (value & (0x01 << (7 - p)));
      unsigned char r = ((is_ink ? ink : paper) & 0xFF000000) >> 24;
      unsigned char g = ((is_ink ? ink : paper) & 0x00FF0000) >> 16;
      unsigned char b = ((is_ink ? ink : paper) & 0x0000FF00) >> 8;
      unsigned char a = ((is_ink ? ink : paper) & 0x000000FF);

      al_draw_pixel(column + p + 32, row + 24, al_map_rgb(r,g,a));
      
      printf("row=%d, column=%d, value=%x, is_ink=%d, ink=%x, paper=%x, r=%x, g=%x, b=%x, a=%x\n", row, column+p, value, is_ink, ink, paper, r, g, b, a);
    }
  } else {
  }
}

void ULA::SetBitmap(ALLEGRO_BITMAP* _bitmap) {
  bitmap = _bitmap;
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
