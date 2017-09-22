/////////////////////////////////////////////////////////////////////////
// ULA.h
//	Copyright (c) 2012, Fernando Marcos
//	All rights reserved.
//	
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met: 
//	
//	1. Redistributions of source code must retain the above copyright notice, this
//	   list of conditions and the following disclaimer. 
//	2. Redistributions in binary form must reproduce the above copyright notice,
//	   this list of conditions and the following disclaimer in the documentation
//	   and/or other materials provided with the distribution. 
//	
//	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
//	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
//	This file implements a Spectrum ULA chip. This chip handles both the
//	computer's IO and video generation and handles the lower 16 KB of RAM, so it
//	has to attach to both data & I/O busses.
//  ToDo: Sound!!!

#pragma once

#include "BusComponents.h"

#pragma unmanaged

// Spectrum color table to ARGB
static const unsigned int dwColorTable[] =
{
  0xFF000000,
  0xFF0000CD,
  0xFFCD0000,
  0xFFCD00CD,
  0xFF00CD00,
  0xFF00CDCD,
  0xFFCDCD00,
  0xFFCDCDCD,

  0xFF000000,
  0xFF0000FF,
  0xFFFF0000,
  0xFFFF00FF,
  0xFF00FF00,
  0xFF00FFFF,
  0xFFFFFF00,
  0xFFFFFFFF
};


/////////////////////////////////////////////////////////////////////////
// ULAMemory
//	This is the RAM handled by the ULA, the lower 16 kilobytes of system
//	RAM.

class ULAMemory : public RAM<16384,16*1024>
{
protected:
	void Write(unsigned int address,unsigned char value)
	{
    // First, store data
    __super::Write(address,value);
    // and forward to ULA
		MemoryWrite(address,value);
	}
	virtual void MemoryWrite(unsigned int address,unsigned char value) = 0;
};


/////////////////////////////////////////////////////////////////////////
// ULAIO
//	This is the I/O related access to the ULA.

class ULAIO : public BusComponent<0xFE,1>
{
protected:
	unsigned char Read(unsigned int address)
	{
		return(IORead(address));
	}

	void Write(unsigned int address,unsigned char value)
	{
		IOWrite(address,value);
	}

	virtual unsigned char IORead(unsigned int address) = 0;
	virtual void IOWrite(unsigned int address,unsigned char value) = 0;
};


/////////////////////////////////////////////////////////////////////////
// ULA
//	This is the full ULA implementation of a ZX Spectrum. This component
//	inherits from two bus components: ULAMemory & ULAIO, and each of this
//	bases is mapped on to a different cpu bus.

// Screen timing
// 224 tStates per scanline x 312 scanlines x 50 frames per second
// equals aproximately 3.494400 MHz

#define   TSTATES_PER_SCANLINE      (224)
#define   TVSCANLINES               (312)
#define   TSTATES_PER_FRAME         (TSTATES_PER_SCANLINE * TVSCANLINES)
#define   AUDIO_SAMPLE_RATE         (48000)
#define   SAMPLES_PER_FRAME         (AUDIO_SAMPLE_RATE / 50)

class ULA : public ULAMemory, ULAIO
{
public:
  ULA()
  {
		// Memory initialization
    nativeBitmap = NULL;
    dwordsPerRow = 0;
		dwFrameTStates = 0;				// TStates into current frame
		dwScanLineTStates = 0;		// TStates into current scanline
		dwScanLine = 0;						// Scan line counter
    IsDirty = false;
		blinkState = false;
		dwFrameCount = 0;					// Used for blinking attributes
		dwBorderRGBColor = 0x00000000;
    audioOutput = 0;          // Analog audio output
    dcAverage = 0;            // DC removal average
		memset(dwCurrentScanLineBackColor,0x00,sizeof(dwCurrentScanLineBackColor));

		// IO initialization
    ZeroMemory(keyMatrix,sizeof(keyMatrix));
		autoKeyRelease = false;
  }

public:
	/////////////////////////////////////////////////////////////////////////
	// SetNativeBitmap():
	//	This function is used to setup the bitmap that the ULA will update with
	//	the Spectrum's screen memory. pBitmap should point to a buffer holding
	//	320x240 DWORDS as the ULA video works in 32bpp. Bitmap memory layout
	//	will be BGRA.

  void SetNativeBitmap(LPBYTE pBitmap,int bytesPerScanLine)
  {
    nativeBitmap = (LPDWORD)pBitmap;
    dwordsPerRow = bytesPerScanLine / 4;
  }

	/////////////////////////////////////////////////////////////////////////
	// AddCycles():
	//	This function keeps the current screen scan state, updating it with
	//	the number of CPU cycles used.

  void AddCycles(unsigned int cycles,bool& IRQ)
  {
    dwFrameTStates += cycles;
    dwScanLineTStates += cycles;

    // Update the analog audio output from ULA
    // First, compute audio output value for this cycle
    int signal = 0;
    signal = (ULAIOData & 0x10) ? +16384 : -16384;
    signal += (ULAIOData & 0x08) ? +8192 : -8192;

    // update DC average (DC removal)
    dcAverage = (dcAverage + signal) / 2;

    // Now, add audio output over an 8 tap filter:
    // 1: Maintain 7/8ths of the original signal
    audioOutput -= (audioOutput / 8);
    // 2: ...and add 1/8th of the new one
    audioOutput += (signal / 8);

    // Update the audio sample corresponding to this screen tState
    unsigned int offset = (dwFrameTStates * SAMPLES_PER_FRAME) / (TSTATES_PER_SCANLINE * TVSCANLINES);
    // As clocks don't match and this is a quick approximation, limit offset output
    if (offset < SAMPLES_PER_FRAME)
      FrameAudio[offset] = audioOutput; // - dcAverage;   // audio with DC removed

    if (dwScanLineTStates > TSTATES_PER_SCANLINE)
      ScanLine(IRQ);
  }


	/////////////////////////////////////////////////////////////////////////
	// ScanLine():
	//	The function AddCycles() invokes this function whenever a scanline is
	//	completed. This function keeps track of the video beam and performs
	//	three functions:
	//	- when a complete frame has been generated, notifies our caller that
	//	an IRQ should be triggered.
	//	- when 16 frames have been completed, invokes the function to blink
	//	characters that have that attribute.
	//	- as scanlines advance, it updates the screen's border color.

  void ScanLine(bool &IRQ)
  {
    dwScanLineTStates %= TSTATES_PER_SCANLINE;
    dwScanLine++;
    if (dwScanLine >= TVSCANLINES)
    {
      // Frame complete - trigger IRQ
      IRQ = true;
      dwFrameTStates %= TSTATES_PER_FRAME;
      dwScanLine = 0;
      if (++dwFrameCount > 16)		// each 16 full frames...
      {
        dwFrameCount = 0;					// reset counter...
        blinkState = !blinkState;	// ... invert blink ...
        UpdateBlink();						// ... and update screen contents
      }
    }

    // Check if current scanline (0-311) lies within the "visible portion"
    // of screen bitmap (lines 0-239), i.e., skip first and last 36 lines (front/back porches)
    if ((dwScanLine < 36) || (dwScanLine > (239+36))) return;

    // Now check if current background color for scanline is different
    // from the required one
    DWORD bitmapLine = dwScanLine-36;
    if (dwCurrentScanLineBackColor[bitmapLine] == dwBorderRGBColor)  // Already right color
      return;

    // Redraw current scanline with proper color
    LPDWORD scr = nativeBitmap;
    scr += (bitmapLine * 320);	// Get pointer to start of line

    // top/bottom borders?
    if ((bitmapLine < 24) || (bitmapLine > 24+191))
    {
      // Draw whole line
      unsigned int bCount = 320;
      do
      {
        *scr++ = dwBorderRGBColor;
      }while (--bCount);
      }
    else
    // screen contents area
    {
      // First 32 pixels
      unsigned int bCount = 32;
      do
      {
        *scr++ = dwBorderRGBColor;
      } while (--bCount);

      // Last 32 pixels
      scr += 256;
      bCount = 32;
      do
      {
        *scr++ = dwBorderRGBColor;
      } while (--bCount);
    }

    // and remember this scanline's colour
    dwCurrentScanLineBackColor[bitmapLine] = dwBorderRGBColor;
    IsDirty = true;
  }

  
  /////////////////////////////////////////////////////////////////////////
	// MemoryWrite():
	//	This is the memory related accesses to the ULA. It handles writes to
	//	the video memory, updating the supplied nativeBitmap with the updated
	//	contents. When nativeBitmap has been modified, the boolean flag IsDirty
	//	is set, so the upper layer knows it has to refresh the screen.

  void MemoryWrite(unsigned int address,unsigned char value)
  {
    if (nativeBitmap == NULL)
      return;

    address &= 0x3FFF;
    if (address > 0x1AFF)   // Above bitmap+attrs?
      return;

    // Video memory: 0x0000->0x17FF bitmap graphics, 0x1800->0x1AFF attributes
    if (address < 0x1800)   // Bitmap graphics
    {
      // Convert from Spectrum memory video to plain buffer
      unsigned int rowbase = address >> 5;
      rowbase = ((rowbase & 0xC0) |
      ((rowbase & 0x38) >> 3) |
      ((rowbase & 0x07) << 3));

      LPDWORD scr = nativeBitmap;
      // Center pixel video (256x192) in the whole bitmap (320x240) == offset 32 horz, 24 vert
      scr += (320 * 24);
      scr += 32;
      scr += (rowbase * dwordsPerRow);
      scr += ((address & 0x001F) * 8);

      // Fetch attribute
      unsigned char attr = data[0x1800 + ((rowbase / 8) * 32) + (address & 0x001F)];
      DWORD dwInk;
      DWORD dwPaper;
      if ((attr & 0x80) && blinkState)
      {
        dwPaper = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
        dwInk = dwColorTable[(attr & 0x78) >> 3];
      }
      else
      {
        dwInk = dwColorTable[((attr & 0x40) >> 3) | (attr & 0x07)];
        dwPaper = dwColorTable[(attr & 0x78) >> 3];
      }

      for (int dd=7;dd >= 0;dd--)
      {
        *scr++ = (value & (1 << dd)) ? dwInk : dwPaper;
      }
      IsDirty = true;
    }
    else    // Attribute memory
    {
      // Redraw whole affected "character block"
      UpdateChar(address - 0x1800);
    }
  }


	/////////////////////////////////////////////////////////////////////////
	// UpdateChar():
	//	This function redraws a complete character on the screen. It is
	//	typically invoked when an attrbiute has changed for a character or
	//	to "blink" characters.

	void UpdateChar(unsigned int nChar)
	{
    int memOffset = ((nChar & 0x300) << 3) | (nChar & 0xFF);
		unsigned char value = data[nChar + 0x1800];

		LPDWORD scr = nativeBitmap;
    // Center pixel video (256x192) in the whole bitmap (320x240) == offset 32 horz, 24 vert
    scr += (320 * 24);
    scr += 32;
		scr += (((nChar / 32) * 8) * dwordsPerRow);
		scr += ((nChar & 0x001F) * 8);

		DWORD dwInk;
		DWORD dwPaper;
		if ((value & 0x80) && blinkState)
		{
			dwPaper = dwColorTable[((value & 0x40) >> 3) | (value & 0x07)];
			dwInk = dwColorTable[(value & 0x78) >> 3];
		}
		else
		{
			dwInk = dwColorTable[((value & 0x40) >> 3) | (value & 0x07)];
			dwPaper = dwColorTable[(value & 0x78) >> 3];
		}

    // Redraw 8x8 pixels
    for (int yy=0;yy<8;yy++)
    {
      unsigned char scanData = data[memOffset];
      LPDWORD pixel = scr;
		  for (int dd=7;dd >= 0;dd--)
		  {
			  *pixel++ = (scanData & (1 << dd)) ? dwInk : dwPaper;
		  }
      memOffset += 256;   // Next scanline on Spectrums memory
      scr += dwordsPerRow; // Next scanline on native bitmap
    }
    IsDirty = true;
	}


	/////////////////////////////////////////////////////////////////////////
	// UpdateBlink():
	//	This function, invoked every 16 frames, updates any character block
	//	on screen that has the "blink" attribute set.

	void UpdateBlink()
	{
		// Check if any attribute has bit 0x80 set (blinking char)
		for (unsigned int nChar = 0;nChar < (32*24);nChar++)
		{
			if (data[0x1800 + nChar] & 0x80)
				UpdateChar(nChar);
		}
	}


	/////////////////////////////////////////////////////////////////////////
	// IOWrite():
	//	I/O related write function of the ULA. Currently just updates the
	//	screen's border color.

  void IOWrite(unsigned int address,unsigned char value)
  {
		// ULA is selected when reading an even address
    if (address & 0x01)
      return;

    // save a copy of ULA's state
    ULAIOData = value;
		// Update border color member variable.
		dwBorderRGBColor = dwColorTable[value & 0x07];
  }


	/////////////////////////////////////////////////////////////////////////
	// IORead():
	//	I/O related read functions. Returns the current state for the keyboard
	//	scan lines selected via the high order byte.

  unsigned char IORead(unsigned int address)
  {
    // ULA is selected by A0 being low
    if ((address & 0x01) == 0)
    {
      // Keyboard info:
      // http://www.users.globalnet.co.uk/~jg27paw4/yr01/yr01_58.htm

      // Get the scancodes
      unsigned char kData = 0xFF;   // Pull ups
      unsigned char row = (address >> 8) ^ 0xFF;
      for (int dd=0;dd<8;dd++)
      {
        if (row & (1 << dd))        // Select scanline?
        {
          kData &= ~keyMatrix[dd];  // pull down bits representing "pressed" lines
          if (autoKeyRelease)				// if automatic key release...
            keyMatrix[dd] = 0x00;		// ...clear pressed keys after reporting.
        }
      }
      return(kData);
    }
    return(0xFF);       // Pull-ups? garbage....?
  }

	/////////////////////////////////////////////////////////////////////////
	// PressKey():
	//	This is part of the "emulation API" of the ULA. Allows an external
	//	component to set the state of the keyboard.
	//	keyRow & keyCol correspond to the physical keyboard layout (4 rows by
	//	10 columns).

  void PressKey(unsigned int keyRow,unsigned int keyCol,bool down)
  {
    if (keyCol > 9)
      return;
    if (keyRow > 3)
      return;

    // Spectrum keyboard layout
    //       D0 D1 D2 D3 D4    D4 D3 D2 D1 D0
    // A11      ROW3              ROW4          A12
    // A10      ROW2              ROW5          A13
    //  A9      ROW1              ROW6          A14
    //  A8      ROW0              ROW7          A15

    int rowNdx;
    int bitMask;
    if (keyCol < 5)   // Left bank
    {
      rowNdx = 3 - keyRow;
      bitMask = 0x01 << keyCol;
    }
    else              // Right bank
    {
      rowNdx = 4 + keyRow;
      bitMask = 0x01 << (9 - keyCol);
    }

    if (down)
      keyMatrix[rowNdx] |= bitMask;
    else
      keyMatrix[rowNdx] &= ~bitMask;
  }

  public:
    bool IsDirty;
    // at 8KHz, 1/50th of a second == 20ms == 160 samples
    // at 48KHz, 1/50th of a second == 20 ms == 960 samples
    short FrameAudio[SAMPLES_PER_FRAME];
 
	protected:
		bool autoKeyRelease;
		unsigned char keyMatrix[8];

	protected:
		DWORD dwBorderRGBColor;
		DWORD dwScanLine;

	private:
		LPDWORD nativeBitmap;
		int dwordsPerRow;
		unsigned int dwFrameTStates;
		unsigned int dwScanLineTStates;
		unsigned int dwFrameCount;
		unsigned int blinkState;
		DWORD dwCurrentScanLineBackColor[240];
    unsigned char ULAIOData;
    short audioOutput;
    short dcAverage;
};
