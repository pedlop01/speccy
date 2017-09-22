/////////////////////////////////////////////////////////////////////////
// TapFile.CS
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

using System;
using System.IO;

// TAP file format:
// http://www.nvg.ntnu.no/sinclair/faq/fileform.html#TAPZ
// http://www.zxmodules.de/fileformats/tapformat.html

namespace Emulator
{
  class TapBlock
  {
    public TapBlock(BinaryReader stream)
    {
      // A TAP block consists of a two byte header plus data bytes.
      // Data is raw data as saved by Spectrum - and as expected
      // by ROM routines we are replacing.
      UInt16 size = stream.ReadUInt16();
      data = stream.ReadBytes(size);
    }

    public Byte[] Data
    {
      get
      {
        return (data);
      }
    }

    protected byte[] data = null;
  }

  class TapFile
  {
    public void Open(String fileName)
    {
      System.Collections.Generic.List<TapBlock> blockCol = new System.Collections.Generic.List<TapBlock>();

      try
      {
        using (var file = File.OpenRead(fileName))
        {
          var stream = new BinaryReader(file);
          while (stream.BaseStream.Position < stream.BaseStream.Length)
          {
            var newBlock = new TapBlock(stream);
            blockCol.Add(newBlock);
          }
        }
      }
      catch (Exception) { }

      blocks = blockCol.ToArray();
    }

    public TapBlock[] Blocks
    {
      get
      {
        return (blocks);
      }
    }

    protected TapBlock[] blocks = null;
  }
}
