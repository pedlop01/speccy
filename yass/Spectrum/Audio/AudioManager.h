/////////////////////////////////////////////////////////////////////////
// AudioManager.H
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

/////////////////////////////////////////////////////////////////////////
// AudioManager
//  This base class defines the entry points to any AudioManager inherited
//  class.
//  An audio manager is a class that accepts, via the QueueAudio() function,
//  a block of samples worth 20ms (1/50 sec) and plays it. If too many
//  blocks are queued, the function pauses execution until a block is
//  available (that is, 20ms). This way the sound card is used as a "clock"
//  for our emulated Spectrum.

#pragma unmanaged

class AudioManager
{
public:
  virtual bool QueueAudio(short* audioData) = 0;
};

/////////////////////////////////////////////////////////////////////////
// WaveOutAudioManager
//  Implementation of an AudioManager class for Microsoft Windows.

#ifdef WIN32

#pragma unmanaged
#pragma comment(lib,"WinMM.lib")

/////////////////////////////////////////////////////////////////////////
// AudioBlock
//  This class represents a single block of 20ms worth of audio data
//  for a given sample rate. The class encapsulates both the Windows
//  header and the audio data itself in a single package.

template<unsigned int sampleRate> struct AudioBlock
{
  WAVEHDR Header;
  short AudioData[sampleRate / 50];
};

/////////////////////////////////////////////////////////////////////////
// WaveOutAudioManager class

#define   AUDIOBLOCK_COUNT      (3)

template<unsigned int sampleRate> class WaveOutAudioManager : public AudioManager
{
public:
  WaveOutAudioManager()
  {
    hWaveDev = NULL;
    hWaveEvent = INVALID_HANDLE_VALUE;
  }
  virtual ~WaveOutAudioManager()
  {
    Close();
  }

public:
  /////////////////////////////////////////////////////////////////////////
  // Open():
  //  The Open() function initializes the audio subsystem and the block
  //  array used to feed the sound card.

  int Open()
  {
    if (hWaveDev != NULL)
      return(ERROR_INVALID_STATE);

    if (hWaveEvent != INVALID_HANDLE_VALUE)
      CloseHandle(hWaveEvent);
    hWaveEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

    WAVEFORMATEX wf;
    ZeroMemory(&wf,sizeof(wf));
    wf.cbSize = sizeof(wf);
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 1;   // mono audio
    wf.nSamplesPerSec = sampleRate;
    wf.nAvgBytesPerSec = sampleRate * sizeof(short);
    wf.nBlockAlign = sizeof(short);
    wf.wBitsPerSample = 8 * sizeof(short);
    int result = waveOutOpen(&hWaveDev,WAVE_MAPPER,&wf,(DWORD_PTR)hWaveEvent,NULL,CALLBACK_EVENT);
    if (result != 0)
    {
      Close();
      return(result);
    }

    // Prepare audio buffers. Each one will be 20ms long
    for (int dd=0;dd<AUDIOBLOCK_COUNT;dd++)
    {
      ZeroMemory(&audioBlocks[dd],sizeof(AudioBlock<sampleRate>));
      audioBlocks[dd].Header.lpData = (LPSTR)&audioBlocks[dd].AudioData[0];
      audioBlocks[dd].Header.dwBufferLength = sizeof(audioBlocks[dd].AudioData);
      result = waveOutPrepareHeader(hWaveDev,&audioBlocks[dd].Header,sizeof(audioBlocks[dd].Header));
      if (result != MMSYSERR_NOERROR)
      {
        Close();
        return(result);
      }
    }

    // Everything ready...
    return(0);
  }

  /////////////////////////////////////////////////////////////////////////
  // Close():
  //  This function closes the audio system. It releases any pending audio
  //  block still queued and releases resources.

  void Close()
  {
    if (hWaveDev != NULL)
    {
      waveOutReset(hWaveDev);
      waveOutClose(hWaveDev);
      hWaveDev = NULL;
    }

    if (hWaveEvent != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hWaveEvent);
      hWaveEvent = INVALID_HANDLE_VALUE;
    }
  }

protected:
  /////////////////////////////////////////////////////////////////////////
  // FindAvailableBlock():
  //  This function looks thru the array of audio blocks to see if there is
  //  one available to feed more audio data. If no block is found, returns
  //  NULL.

  AudioBlock<sampleRate> *FindAvailableBlock()
  {
    for (int dd=0;dd<AUDIOBLOCK_COUNT;dd++)
    {
      if ((audioBlocks[dd].Header.dwFlags & WHDR_INQUEUE) == 0)
        return(&audioBlocks[dd]);
    }
    return(NULL);
  }

public:
  /////////////////////////////////////////////////////////////////////////
  // QueueAudio():
  //  This function queues 20 ms of audio data, and expects an audio buffer
  //  of that size. If no buffer is available, waits for an audio system
  //  event and searches again. As audio blocks contain 20ms of audio, that
  //  is the maximum wait time.

  bool QueueAudio(short* audioData)
  {
    // Check if any block is available right now
    AudioBlock<sampleRate> *block = FindAvailableBlock();
    // If no audio block is available, just wait for one (max 20ms)
    if (block == NULL)
    {
      WaitForSingleObject(hWaveEvent,1000);   // Wait for 1 second max
      block = FindAvailableBlock();
    }

    // This is weird... something really strange happened.
    if (block == NULL)
      return(false);

    // Fill block with audio data
    for (int dd=0;dd<sampleRate/50;dd++)
      block->AudioData[dd] = audioData[dd];
    // and send again to audio subsystem
    int result = waveOutWrite(hWaveDev,&block->Header,sizeof(block->Header));
    return(result == MMSYSERR_NOERROR ? true : false);
  }

protected:
  HANDLE hWaveEvent;
  HWAVEOUT hWaveDev;
  AudioBlock<sampleRate> audioBlocks[AUDIOBLOCK_COUNT];
};

#endif // WIN32
