/////////////////////////////////////////////////////////////////////////
// Spectrum.CPP
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
//	The managed class Emulation::Spectrum implements a complete ZX
//	Spectrum computer.

#include "stdafx.h"
#include <vcclr.h>
#include "Spectrum.h"
#include "Core\ULA.h"
#include "Audio\AudioManager.h"
#include "resource.h"

using namespace System::Windows;
using namespace System::Windows::Media;

#pragma managed

namespace Emulation
{
	/////////////////////////////////////////////////////////////////////////
	// Constructor

	Spectrum::Spectrum()
	{
    emulatorScreen = nullptr;
		forceReset = false;
    quitEmulation = false;
    screenData = nullptr;
		screenDataDirty = false;
		m_RenderDelegate = gcnew EventHandler(this,&Spectrum::OnRenderTick);
	}


	/////////////////////////////////////////////////////////////////////////
	// Start():
	//	This function starts the emulator thread.

	void Spectrum::Start()
	{
		if (emulatorThread != nullptr)
			return;		// Already running

		forceReset = false;
    quitEmulation = false;

		// Attach to WPF screen refresh to update
		CompositionTarget::Rendering += m_RenderDelegate;

		// Build the emulated screen and WPF render surface.
    if (emulatorScreen == nullptr)
    {
			// Emulator will draw on this bitmap
  		emulatorScreen = gcnew WriteableBitmap(320,240,72,72,PixelFormats::Bgra32,nullptr);
		  bytesPerScreenLine = emulatorScreen->BackBufferStride;

      emulatorScreen->Lock();
		  screenData = (LPDWORD)emulatorScreen->BackBuffer.ToPointer();

      for (int dd=0;dd<320*240;dd++)
        screenData[dd] = 0xFFC0C0C0;
		  emulatorScreen->Unlock();

			// ... and the bitmap is attached as the contents of the control's background.
		  ImageBrush^ bkgnd = gcnew ImageBrush();
      bkgnd->ImageSource = emulatorScreen;
      Background = bkgnd;
    }

    emulatorThread = gcnew Thread(gcnew ThreadStart(this,&Spectrum::emulatorMain));
		emulatorThread->Start();
	}


	/////////////////////////////////////////////////////////////////////////
	// Reset():
	//	This function allows for a remote reset request on the emulator.

	void Spectrum::Reset()
	{
		forceReset = true;
	}

	/////////////////////////////////////////////////////////////////////////
	// Stop():
	//	This function stops the emulator.

  void Spectrum::Stop()
  {
    quitEmulation = true;
		pCurrentUla = nullptr;
		pCurrentKempston = nullptr;

    if (emulatorThread != nullptr)
    {
      if (emulatorThread->Join(TimeSpan(0,0,3)) == false)
      {
        try
        {
          emulatorThread->Abort();
        }
        catch(Exception^) { }
      }
      emulatorThread = nullptr;
    }
		CompositionTarget::Rendering -= m_RenderDelegate;
  }


	/////////////////////////////////////////////////////////////////////////
	// PressKey():
	//	This function simulates a key being pressed on the machine. The
	//	arguments <keyRow> (0-3) and <keyCol> (0-9) specify the physical key location
	//	as per Spectrum's keyboard layout.

  void Spectrum::PressKey(int keyRow,int keyCol)
  {
    if (pCurrentUla != nullptr)
      pCurrentUla->PressKey(keyRow,keyCol,true);
  }


	/////////////////////////////////////////////////////////////////////////
	// ReleaseKey():
	//	This function simulates a key being released on the machine. The
	//	arguments <keyRow> (0-3) and <keyCol> (0-9) specify the physical key location
	//	as per Spectrum's keyboard layout. If the key was not previously
	//	pressed, it does nothing.

  void Spectrum::ReleaseKey(int keyRow,int keyCol)
  {
    if (pCurrentUla != nullptr)
      pCurrentUla->PressKey(keyRow,keyCol,false);
  }


	/////////////////////////////////////////////////////////////////////////
	// SetKempstonStatus():
	//	Establishes the current status of the virtual joystick. These flags
	//	will be forwarded to the emulated software.

	void Spectrum::SetKempstonStatus(KempstonFlags status)
	{
		if (pCurrentKempston != nullptr)
			pCurrentKempston->SetKempstonData((unsigned char)status);
	}


	/////////////////////////////////////////////////////////////////////////
	// OnRenderTick():
	//	WPF will notify this delegate for every screen refresh. If emulator
	// screen is dirty, mark the whole bitmap as a dirty section to make
	// the runtime update the contents.

  void Spectrum::OnRenderTick(Object^ sender,EventArgs ^e)
  {
    if (screenDataDirty)
    {
      emulatorScreen->Lock();
      emulatorScreen->AddDirtyRect(Int32Rect(0,0,320,240));
      screenDataDirty = false;
      emulatorScreen->Unlock();
    }
  }


	/////////////////////////////////////////////////////////////////////////
	// emulatorMain():
	//	The main emulator process. This funcion runs in a separate thread.

  void Spectrum::emulatorMain()
  {
    // Components
    Bus16 bus;              // Main Z80 data bus
    SpectrumIOBus ioBus;    // Main Z80 I/O bus
    ROM<0,16384> rom;       // Main system ROM
    ULA ula;                // Spectrum's ULA chip + 16KB
    RAM<32768,32*1024> ram; // Remaining RAM (32KB)
    Kempston kempston;      // Kempston joystick
    Z80 cpu;                // and finally, the CPU core
    // Native audio driver
    WaveOutAudioManager<AUDIO_SAMPLE_RATE> audMgr;

    // Load ROM contents
    #ifdef WIN32
    if (rom.Load(MAKEINTRESOURCE(IDR_ROM_SPECTRUM48),L"ROM"))
      throw gcnew System::IO::FileNotFoundException("Unable to load rom '48.ROM'");
    #else
    if (rom.Load("48.rom"))
      throw gcnew System::IO::FileNotFoundException("Unable to load rom '48.ROM'");
    #endif

    // Configure the ULA with the native Windows bitmap used to emulate the screen
    ula.SetNativeBitmap((LPBYTE)screenData,bytesPerScreenLine);

    // Populate buses
    bus.AddBusComponent(&rom);
    bus.AddBusComponent((ULAMemory*)&ula);
    bus.AddBusComponent(&ram);

    ioBus.ULA = (ULAIO*)&ula;
    ioBus.Kempston = &kempston;

    // And attach busses to cpu core
    cpu.DataBus = &bus;
    cpu.IOBus = &ioBus;

    // To emulate the system, we publish a number of components via member variables
    // so that external functions can manipulate them (press and release keys, move
    // the joystick).
    pCurrentUla = &ula;
    pCurrentKempston = &kempston;

    // Initialize audio subsystem
    audMgr.Open();

    // Ready to roll!!
    do
    {
      // The forceReset member variable allows our host to reset the CPU
      if (forceReset)
      {
        cpu.regs.PC = 0x0000;
        cpu.regs.iff1A = 0;
        cpu.regs.iff1B = 0;
        cpu.regs.IM = 0;
        forceReset = false;
      }

      // Tape load trap
      if ((cpu.regs.PC == 0x056B) && (OnLoad))
      {
        try
        {
          LoadTrap(cpu);
          cpu.regs.BC = 0xB001;
          cpu.regs.altAF = 0x0145;
          cpu.regs.CF = 1;		// Carry flag set: Success
        }
        catch(Exception^)
        {
          cpu.regs.CF = 0;		// Carry flag reset: Failure
        }
        cpu.regs.PC = 0x05e2;	// "Return" from the "tape block load" routine
      }

      // And emulate next instruction
      cpu.tStates = 0;
      cpu.EmulateOne();

      // After each instruction, report the ULA the number of cycles we've used
      bool irq = false;
      ula.AddCycles(cpu.tStates,irq);

      // As in the real Spectrum, the ULA will trigger an IRQ for every frame. This
      // implementation uses cpu clock cycles to know where the screen beam is.
      if (irq)		// Ula signals a frame interrupt
      {
        cpu.INT();		// Generate system interrupt

        // If screen contents have been modified, set a flag for the WPF rendering event.
        if (ula.IsDirty)
        {
          screenDataDirty = true;
          ula.IsDirty = false;
        }

        // Submit audio to driver. The "driver" will hold the loop until an audio buffer
        // is available - that is, 20 ms. This will match the speed of our emulated
        // Spectrum to the real one.
        audMgr.QueueAudio(ula.FrameAudio);
      }
    } while(quitEmulation == false);

    pCurrentUla = nullptr;
    pCurrentKempston = nullptr;
  }


	/////////////////////////////////////////////////////////////////////////
	// LoadTrap():
	//	When active (a handler has been attached to the OnLoad delegate),
	//	the component will intercept calls to the "Load Block" function in the
	//	Spectrum ROM. Here, those requests will be forwarded to the programmer's
	//	provided component that will serve them.

  void Spectrum::LoadTrap(Z80& cpu)
  {
    // Call the delegate to obtain the next tape block
    array<Byte>^ data = OnLoad(cpu.regs.DE);

    // First byte of data contains value for the A register on return. Last
    // byte is blocks checksum (not using it).
    int nBytes = data->Length-2;
    if (cpu.regs.DE < nBytes)
      nBytes = cpu.regs.DE;

    // We must place data read from tape at IX base address onwards
    // DE is the number of bytes to read, IX increments with each byte read.
    for (int dd=0;dd<nBytes;dd++)
    {
      // Write block using cpu's data bus and cpu's registers...
      cpu.DataBus->Write(cpu.regs.IX++,data[dd+1]);
      cpu.regs.DE--;
    }
  }
};
