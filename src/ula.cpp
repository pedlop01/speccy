#include "ula.h"

ULA::ULA() {
  bitmap = NULL;
  dwScanLineTStates = 0;
  dwScanLine = 0;
  isDirty = false;
  blinkState = false;
  dwFrameCount = 0;
  dwBorderRGBColor = 0x0000000;
  memset(dwCurrentScanLineBackColor, 0x00, sizeof(dwCurrentScanLineBackColor));

  memset(keyMatrix, 0, sizeof(keyMatrix));

  memset(FrameAudio, 0, sizeof(FrameAudio));
  audioOutput = 0;
  dcAverage = 0;
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

  // Update the analog audio output from ULA
  // First, compute audio output value for this cycle
  int signal = 0;
  signal  = (ULAIOData & 0x10) ? +16384 : 0;
  //signal += (ULAIOData & 0x08) ?  +8192 : 0;

  // update DC average (DC removal)
  dcAverage = (dcAverage + signal) / 2;

  // Now, add audio output over an 8 tap filter:
  // 1: Maintain 7/8ths of the original signal
  //audioOutput -= audioOutput / 8;
  // 2: add 1/8th of the new one
  audioOutput = signal / 8;

  // Update the audio sample corresponding to this screen tState
  unsigned int offset = (dwFrameTStates * SAMPLES_PER_FRAME) /
                        (TSTATES_PER_SCANLINE * TVSCANLINES);
  
  // As clocks don't match and this is a quick approximation, limit
  // offset output
  if (offset < SAMPLES_PER_FRAME)    
    FrameAudio[offset] = audioOutput/* - dcAverage*/;

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

  unsigned char r = /*0x33;*/(dwBorderRGBColor & 0xFF000000) >> 24;
  unsigned char g = /*0x33;*/(dwBorderRGBColor & 0x00FF0000) >> 16;
  unsigned char b = /*0x33;*/(dwBorderRGBColor & 0x0000FF00) >> 8;
  unsigned char a = /*0x33;*/(dwBorderRGBColor & 0x000000FF);

  // redraw current scanline with proper color
  if ((bitmapLine < 24) || (bitmapLine > 24+191)) {
    // top/bottom border
    al_draw_line(0, bitmapLine, 319, bitmapLine, al_map_rgb(r, g, b), 1);
  } else {
    // screen contents
    al_draw_line(0,   bitmapLine, 31,  bitmapLine, al_map_rgb(r, g, b), 1);
    // REVISIT: 287 or 288?
    al_draw_line(287, bitmapLine, 319, bitmapLine, al_map_rgb(r, g, b), 1);
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
  
  // save a copy of ULA's state
  ULAIOData = value;

  // Update boder color
  dwBorderRGBColor = dwColorTable[value & 0x07];
}

unsigned char ULA::IORead(unsigned int address) {
  // ULA is selected by A0 being low
  if ((address & 0x01) == 0) {
    // Get the scancodes
    unsigned char kData = 0xFF;   // Pull ups
    unsigned char row = (address >> 8) ^ 0xFF;
    for (int i = 0; i < 8; i++) {
      if (row & (1 << i)) {       // travers all selected rows
        kData &= ~keyMatrix[i];   // pull down bits representing pressed key
      }
    }
    return kData;
  }
  return 0xFF; // pull up on bus
}

void ULA::PressKey(unsigned int keyRow, unsigned int keyCol, bool down) {

  if (keyCol > 9)
    return;
  if (keyRow > 3)
    return;

  int rowNdx;
  int bitMask;
  if (keyCol < 5) {  // Left bank
    rowNdx  = 3 - keyRow;
    bitMask = 0x01 << keyCol;
  } else {
    rowNdx  = 4 + keyRow;
    bitMask = 0x01 << (9 - keyCol);
  }

  if (down)
    keyMatrix[rowNdx] |= bitMask;
  else
    keyMatrix[rowNdx] &= ~bitMask;
}

bool ULA::GetIsDirty() {
  return isDirty;
}

void ULA::ClearFrameAudio() {
  for(unsigned int i = 0; i < SAMPLES_PER_FRAME; i++) {
    FrameAudio[i] = 0;    
  }
}