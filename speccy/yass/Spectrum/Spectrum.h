/////////////////////////////////////////////////////////////////////////
// Spectrum.h
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

#pragma once
#include "Core\Bus.h"
#include "Core\BusComponents.h"
#include "Core\Z80.h"
#include "Core\SpectrumIO.h"
#include "Core\Kempston.H"

using namespace System;
using namespace System::Threading;
using namespace System::Windows::Media::Imaging;

namespace Emulation
{
  // Kempston joystick status values.
  [Flags]
  public enum class KempstonFlags
  {
    Right = KEMPSTON_RIGHT,
    Left = KEMPSTON_LEFT,
    Down = KEMPSTON_DOWN,
    Up = KEMPSTON_UP,
    Fire = KEMPSTON_FIRE
  };

  public delegate array<Byte>^ LoaderHandler(int bytesToLoad);

#pragma managed

  public ref class Spectrum : System::Windows::Controls::Border, IDisposable
  {
  public:
		Spectrum();
    ~Spectrum()
    {
      Stop();
      GC::SuppressFinalize(this);
    };
    !Spectrum()
    {
      Stop();
			emulatorScreen = nullptr;
    }

	public:
		void Start();
		void Reset();
    void Stop();
    void PressKey(int keyRow,int keyCol);
    void ReleaseKey(int keyRow,int keyCol);
		void SetKempstonStatus(KempstonFlags status);

	public:
		LoaderHandler^ OnLoad;

	protected:
		void emulatorMain();
		void LoadTrap(Z80 &cpu);
		void OnRenderTick(Object^ sender,EventArgs^ e);

	private:
		Thread^ emulatorThread;
		EventHandler ^m_RenderDelegate;
		WriteableBitmap^ emulatorScreen;
		LPDWORD screenData;
		bool screenDataDirty;
		int bytesPerScreenLine;
    int quitEmulation;
		int forceReset;

		// These pointers will be updated by the emulation thread and are used by the
		// functions that provide emulation API support (keyboard & joystick)
	private:
		ULA *pCurrentUla;
		Kempston *pCurrentKempston;
	};
}
