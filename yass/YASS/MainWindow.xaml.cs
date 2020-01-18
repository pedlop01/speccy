/////////////////////////////////////////////////////////////////////////
// MainWindow.xaml.cs
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
using System.Threading;
using System.Windows;
using Microsoft.Win32;

namespace Emulator
{
  public partial class MainWindow : Window
  {
    #region Members
    Emulation.KempstonFlags kempston;
    TapFile currentTape = null;
    int currentTapeBlock = 0;
    #endregion

    #region Constructor
    public MainWindow()
    {
      InitializeComponent();
      kempston = 0;

      // Set up load traps
      emulator.OnLoad = TapeLoad;

      emulator.Start();
    }
    #endregion

    #region Menu handling

    #region OnStart()
    private void OnStart(object sender, RoutedEventArgs e)
    {
      emulator.Start();
      currentTapeBlock = 0;
    }
    #endregion

    #region OnReset()
    private void OnReset(object sender, RoutedEventArgs e)
    {
      emulator.Reset();
      // and rewind tape!! ;)
      currentTapeBlock = 0;
    }
    #endregion

    #region OnStop()
    private void OnStop(object sender, RoutedEventArgs e)
    {
      emulator.Stop();
    }
    #endregion

    #region OnClose()
    private void OnClose(object sender, RoutedEventArgs e)
    {
      Close();
    }
    #endregion

    #region OnSelectTape()
    private void OnSelectTape(object sender, RoutedEventArgs e)
    {
      FileDialog dialog = new OpenFileDialog();
      dialog.AddExtension = true;
      dialog.CheckFileExists = true;
      dialog.CheckPathExists = true;
      dialog.Filter = "Tape files (*.tap)|*.tap";
      dialog.DefaultExt = "tap";
      bool? result = dialog.ShowDialog(this);
      if (result == null)
        return;
      if ((bool)result == false)
        return;

      try
      {
        var tape = new TapFile();
        tape.Open(dialog.FileName);
        currentTape = tape;
        currentTapeBlock = 0;

        // Type Load!! ;)
        EmulateLoad();
      }
      catch (Exception)
      {
        MessageBox.Show("Unable to load tape file");
      }
    }
    #endregion

    #region OnTVMode()
    private void OnTVMode(object sender, RoutedEventArgs e)
    {
      // Invert visibility of tv overlay
      tvOverlay.Visibility = tvOverlay.Visibility == Visibility.Collapsed ? Visibility.Visible : Visibility.Collapsed;
      tvModeMenu.IsChecked = tvOverlay.Visibility == Visibility.Visible;
    }
    #endregion

    #region OnAbout()
    private void OnAbout(object sender, RoutedEventArgs e)
    {
      MessageBox.Show("YASS - Yet Another Spectrum Simulator\n\nCopyright (c) 2012, Fernando Marcos\nAll rights reserved.", "About YASS", MessageBoxButton.OK);
    }
    #endregion

    #endregion

    #region Implementation

    #region EmulateLoad()
    protected void EmulateLoad()
    {
      // This function emulates the user typing 'Load ""+CR'
      emulator.PressKey(2, 6);    // 'J' (LOAD) key
      Thread.Sleep(100);
      emulator.ReleaseKey(2, 6);
      emulator.PressKey(3, 8);    // Symbol shift
      emulator.PressKey(1, 9);    // 'P' (") key
      Thread.Sleep(100);
      emulator.ReleaseKey(1, 9);
      Thread.Sleep(100);
      emulator.PressKey(1, 9);    // 'P' (") key
      Thread.Sleep(100);
      emulator.ReleaseKey(1, 9);
      emulator.ReleaseKey(3, 8);
      Thread.Sleep(100);
      emulator.PressKey(2, 9);    // 'Enter" key
      Thread.Sleep(100);
      emulator.ReleaseKey(2, 9);
    }
    #endregion

    #region TapeLoad()
    protected Byte[] TapeLoad(int bytesToLoad)
    {
      TapBlock currentBlock = currentTape.Blocks[currentTapeBlock++];
      
      return (currentBlock.Data);
    }
    #endregion

    #region Keyboard/Kempston emulation

    #region OnKeyDown()
    protected override void OnKeyDown(System.Windows.Input.KeyEventArgs e)
    {
      base.OnKeyDown(e);
      switch (e.Key)
      {
        // Row 0
        case System.Windows.Input.Key.D1: emulator.PressKey(0, 0); break;
        case System.Windows.Input.Key.D2: emulator.PressKey(0, 1); break;
        case System.Windows.Input.Key.D3: emulator.PressKey(0, 2); break;
        case System.Windows.Input.Key.D4: emulator.PressKey(0, 3); break;
        case System.Windows.Input.Key.D5: emulator.PressKey(0, 4); break;
        case System.Windows.Input.Key.D6: emulator.PressKey(0, 5); break;
        case System.Windows.Input.Key.D7: emulator.PressKey(0, 6); break;
        case System.Windows.Input.Key.D8: emulator.PressKey(0, 7); break;
        case System.Windows.Input.Key.D9: emulator.PressKey(0, 8); break;
        case System.Windows.Input.Key.D0: emulator.PressKey(0, 9); break;

        // Row 1
        case System.Windows.Input.Key.Q: emulator.PressKey(1, 0); break;
        case System.Windows.Input.Key.W: emulator.PressKey(1, 1); break;
        case System.Windows.Input.Key.E: emulator.PressKey(1, 2); break;
        case System.Windows.Input.Key.R: emulator.PressKey(1, 3); break;
        case System.Windows.Input.Key.T: emulator.PressKey(1, 4); break;
        case System.Windows.Input.Key.Y: emulator.PressKey(1, 5); break;
        case System.Windows.Input.Key.U: emulator.PressKey(1, 6); break;
        case System.Windows.Input.Key.I: emulator.PressKey(1, 7); break;
        case System.Windows.Input.Key.O: emulator.PressKey(1, 8); break;
        case System.Windows.Input.Key.P: emulator.PressKey(1, 9); break;

        // Row 2
        case System.Windows.Input.Key.A: emulator.PressKey(2, 0); break;
        case System.Windows.Input.Key.S: emulator.PressKey(2, 1); break;
        case System.Windows.Input.Key.D: emulator.PressKey(2, 2); break;
        case System.Windows.Input.Key.F: emulator.PressKey(2, 3); break;
        case System.Windows.Input.Key.G: emulator.PressKey(2, 4); break;
        case System.Windows.Input.Key.H: emulator.PressKey(2, 5); break;
        case System.Windows.Input.Key.J: emulator.PressKey(2, 6); break;
        case System.Windows.Input.Key.K: emulator.PressKey(2, 7); break;
        case System.Windows.Input.Key.L: emulator.PressKey(2, 8); break;
        case System.Windows.Input.Key.Return: emulator.PressKey(2, 9); break;

        // Row 3
        case System.Windows.Input.Key.LeftShift: emulator.PressKey(3, 0); break;
        case System.Windows.Input.Key.Z: emulator.PressKey(3, 1); break;
        case System.Windows.Input.Key.X: emulator.PressKey(3, 2); break;
        case System.Windows.Input.Key.C: emulator.PressKey(3, 3); break;
        case System.Windows.Input.Key.V: emulator.PressKey(3, 4); break;
        case System.Windows.Input.Key.B: emulator.PressKey(3, 5); break;
        case System.Windows.Input.Key.N: emulator.PressKey(3, 6); break;
        case System.Windows.Input.Key.M: emulator.PressKey(3, 7); break;
        case System.Windows.Input.Key.RightCtrl: emulator.PressKey(3, 8); break;
        case System.Windows.Input.Key.Space: emulator.PressKey(3, 9); break;

        // Keyboard shortcuts:
        case System.Windows.Input.Key.Back:
          emulator.PressKey(3, 0);      // Shift
          emulator.PressKey(0, 9);      // '0'
          break;

        // Kempston
        case System.Windows.Input.Key.Left: kempston |= Emulation.KempstonFlags.Left; break;
        case System.Windows.Input.Key.Right: kempston |= Emulation.KempstonFlags.Right; break;
        case System.Windows.Input.Key.Up: kempston |= Emulation.KempstonFlags.Up; break;
        case System.Windows.Input.Key.Down: kempston |= Emulation.KempstonFlags.Down; break;
        case System.Windows.Input.Key.LeftCtrl: kempston |= Emulation.KempstonFlags.Fire; break;
      }
      emulator.SetKempstonStatus(kempston);
    }
    #endregion

    #region OnKeyUp()
    protected override void OnKeyUp(System.Windows.Input.KeyEventArgs e)
    {
      base.OnKeyUp(e);
      switch (e.Key)
      {
        // Row 0
        case System.Windows.Input.Key.D1: emulator.ReleaseKey(0, 0); break;
        case System.Windows.Input.Key.D2: emulator.ReleaseKey(0, 1); break;
        case System.Windows.Input.Key.D3: emulator.ReleaseKey(0, 2); break;
        case System.Windows.Input.Key.D4: emulator.ReleaseKey(0, 3); break;
        case System.Windows.Input.Key.D5: emulator.ReleaseKey(0, 4); break;
        case System.Windows.Input.Key.D6: emulator.ReleaseKey(0, 5); break;
        case System.Windows.Input.Key.D7: emulator.ReleaseKey(0, 6); break;
        case System.Windows.Input.Key.D8: emulator.ReleaseKey(0, 7); break;
        case System.Windows.Input.Key.D9: emulator.ReleaseKey(0, 8); break;
        case System.Windows.Input.Key.D0: emulator.ReleaseKey(0, 9); break;

        // Row 1
        case System.Windows.Input.Key.Q: emulator.ReleaseKey(1, 0); break;
        case System.Windows.Input.Key.W: emulator.ReleaseKey(1, 1); break;
        case System.Windows.Input.Key.E: emulator.ReleaseKey(1, 2); break;
        case System.Windows.Input.Key.R: emulator.ReleaseKey(1, 3); break;
        case System.Windows.Input.Key.T: emulator.ReleaseKey(1, 4); break;
        case System.Windows.Input.Key.Y: emulator.ReleaseKey(1, 5); break;
        case System.Windows.Input.Key.U: emulator.ReleaseKey(1, 6); break;
        case System.Windows.Input.Key.I: emulator.ReleaseKey(1, 7); break;
        case System.Windows.Input.Key.O: emulator.ReleaseKey(1, 8); break;
        case System.Windows.Input.Key.P: emulator.ReleaseKey(1, 9); break;

        // Row 2
        case System.Windows.Input.Key.A: emulator.ReleaseKey(2, 0); break;
        case System.Windows.Input.Key.S: emulator.ReleaseKey(2, 1); break;
        case System.Windows.Input.Key.D: emulator.ReleaseKey(2, 2); break;
        case System.Windows.Input.Key.F: emulator.ReleaseKey(2, 3); break;
        case System.Windows.Input.Key.G: emulator.ReleaseKey(2, 4); break;
        case System.Windows.Input.Key.H: emulator.ReleaseKey(2, 5); break;
        case System.Windows.Input.Key.J: emulator.ReleaseKey(2, 6); break;
        case System.Windows.Input.Key.K: emulator.ReleaseKey(2, 7); break;
        case System.Windows.Input.Key.L: emulator.ReleaseKey(2, 8); break;
        case System.Windows.Input.Key.Return: emulator.ReleaseKey(2, 9); break;

        // Row 3
        case System.Windows.Input.Key.LeftShift: emulator.ReleaseKey(3, 0); break;
        case System.Windows.Input.Key.Z: emulator.ReleaseKey(3, 1); break;
        case System.Windows.Input.Key.X: emulator.ReleaseKey(3, 2); break;
        case System.Windows.Input.Key.C: emulator.ReleaseKey(3, 3); break;
        case System.Windows.Input.Key.V: emulator.ReleaseKey(3, 4); break;
        case System.Windows.Input.Key.B: emulator.ReleaseKey(3, 5); break;
        case System.Windows.Input.Key.N: emulator.ReleaseKey(3, 6); break;
        case System.Windows.Input.Key.M: emulator.ReleaseKey(3, 7); break;
        case System.Windows.Input.Key.RightCtrl: emulator.ReleaseKey(3, 8); break;
        case System.Windows.Input.Key.Space: emulator.ReleaseKey(3, 9); break;

        // Keyboard shortcuts:
        case System.Windows.Input.Key.Back:
          emulator.ReleaseKey(3, 0);      // Shift
          emulator.ReleaseKey(0, 9);      // '0'
          break;

        // Kempston
        case System.Windows.Input.Key.Left: kempston &= ~Emulation.KempstonFlags.Left; break;
        case System.Windows.Input.Key.Right: kempston &= ~Emulation.KempstonFlags.Right; break;
        case System.Windows.Input.Key.Up: kempston &= ~Emulation.KempstonFlags.Up; break;
        case System.Windows.Input.Key.Down: kempston &= ~Emulation.KempstonFlags.Down; break;
        case System.Windows.Input.Key.LeftCtrl: kempston &= ~Emulation.KempstonFlags.Fire; break;
      }
      emulator.SetKempstonStatus(kempston);
    }
    #endregion

    #endregion

    #region Overrides

    #region OnClosed()
    protected override void OnClosed(EventArgs e)
    {
      base.OnClosed(e);
      emulator.Dispose();
    }
    #endregion

    #endregion

    #endregion
  }
}
