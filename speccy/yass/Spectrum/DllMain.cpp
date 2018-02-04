/////////////////////////////////////////////////////////////////////////
// DllMain.CPP
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

#include "Stdafx.h"

#pragma unmanaged

HINSTANCE g_hDllInstance;

BOOL APIENTRY DllMain(HANDLE hModule,DWORD ulReason,LPVOID)
{
  if (ulReason == DLL_PROCESS_ATTACH)
    g_hDllInstance = (HINSTANCE)hModule;

  return TRUE;
}
