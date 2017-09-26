#include "ula.h"

ULA::ULA() {

}

ULA::~ULA() {

}

void ULA::UpdateChar(unsigned int nChar) {
  unsigned int pmemOffset;
  unsigned char attr;
  unsigned int ink;
  unsigned int paper;
  unsigned int row;
  unsigned int column;

  pmemOffset = ((nChar & 0x300) << 3) | (nChar & 0x0FF);
  attr = data[nChar + 0x1800];

  row = (nChar >> 5) << 3;
  column = (nChar & 0x1F) << 3;

  if((attr & 0x80) && blinkState) {
    paper = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
    ink = dwColorTable[(attr & 0x78) >> 3];
  } else {
    paper = dwColorTable[(attr & 0x78) >> 3];
    ink   = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
  }

  // Redraw 8x8 pixels
  for (int y = 0; y < 8; y++) {
    unsigned char pixelData = data[pmemOffset];
    for (int x = 0; x < 8; x++) {
      bool is_ink = (pixelData & (0x01 << (7 - x)));
      unsigned char r = ((is_ink ? ink : paper) & 0xFF000000) >> 24;
      unsigned char g = ((is_ink ? ink : paper) & 0x00FF0000) >> 16;
      unsigned char b = ((is_ink ? ink : paper) & 0x0000FF00) >> 8;
      unsigned char a = ((is_ink ? ink : paper) & 0x000000FF);

      al_draw_pixel(column + x + 32, row + y + 24, al_map_rgb(r,g,b));
    }
    pmemOffset += 256;
  }

}

void ULA::AddCycles(unsigned int cycles, bool& IRQ) {
  dwFrameTStates += cycles;
  dwScanLineTStates += cycles;

  if (dwScanLineTStates > TSTATES_PER_SCANLINE)
    ScanLine(IRQ);
}

void ULA::ScanLine(bool& IRQ) {
  unsigned int bitmapLine;

  dwScanLineTStates %= TSTATES_PER_SCANLINE;
  dwScanLine++;


  if (dwScanLine >= TVSCANLINES) {
    // Fame complete - trigger IRQ
    IRQ = true;
    dwFrameTStates %= TSTATES_PER_FRAME;
    dwScanLine = 0;
    dwFrameCount++;
    if (dwFrameCount > 16) {    // each 16 full frames
      dwFrameCount = 0;         // reset frame counter
      blinkState = !blinkState; // invert blink
      UpdateBlink();            // update screen content
    }
  }

  // Skip non-visible lines
  if((dwScanLine < 36) || (dwScanLine > (312-36)))
    return;

  // Check if current background color for scanline is
  // different from the required one
  bitmapLine = dwScanLine-36;
  if (dwCurrentScanLineBackColor[bitmapLine] == dwBorderRGBColor)
    return;

  unsigned char r = 0x33;//(dwBorderRGBColor & 0xFF000000) >> 24;
  unsigned char g = 0x33;//(dwBorderRGBColor & 0x00FF0000) >> 16;
  unsigned char b = 0x33;//(dwBorderRGBColor & 0x0000FF00) >> 8;
  unsigned char a = 0x33;//(dwBorderRGBColor & 0x000000FF);

  // redraw current scanline with proper color
  if ((bitmapLine < 24) || (bitmapLine > 24+191)) {
    // top/bottom border
    al_draw_line(0, bitmapLine, 319, bitmapLine, al_map_rgb(r, g, b), 1);
  } else {
    // screen contents
    al_draw_line(0,   bitmapLine, 31,  bitmapLine, al_map_rgb(r, g, b), 1);
    al_draw_line(288, bitmapLine, 319, bitmapLine, al_map_rgb(r, g, b), 1);
  }

  // Save border color for line
  dwCurrentScanLineBackColor[bitmapLine] = dwBorderRGBColor;
  isDirty = true;
}

void ULA::UpdateBlink() {
  for (unsigned int nChar = 0; nChar < 32*24; nChar++)
    if (data[nChar + 0x1800] & 0x80)
      UpdateChar(nChar);
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


    column = (address & 0x001F) * 8;

    // Get the attribute
    attr = data[0x1800 + ((row >> 3) << 5) + (address & 0x001F)];

    if((attr & 0x80) && blinkState) {
      paper = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
      ink = dwColorTable[(attr & 0x78) >> 3];
    } else {
      paper = dwColorTable[(attr & 0x78) >> 3];
      ink   = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
    }

    for (unsigned int p = 0; p < 8; p++) {
      bool is_ink = (value & (0x01 << (7 - p)));
      unsigned char r = ((is_ink ? ink : paper) & 0xFF000000) >> 24;
      unsigned char g = ((is_ink ? ink : paper) & 0x00FF0000) >> 16;
      unsigned char b = ((is_ink ? ink : paper) & 0x0000FF00) >> 8;
      unsigned char a = ((is_ink ? ink : paper) & 0x000000FF);

      al_draw_pixel(column + p + 32, row + 24, al_map_rgb(r,g,b));
      
      //printf("row=%d, column=%d, value=%x, is_ink=%d, ink=%x, paper=%x, r=%x, g=%x, b=%x, a=%x\n", row, column+p, value, is_ink, ink, paper, r, g, b, a);
    }
  } else {
    // Redraa affected "character block"
    UpdateChar(address - 0x1800);
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
