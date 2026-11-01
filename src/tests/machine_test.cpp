#include <cassert>
#include <cstdio>
#include <unistd.h>
#include "../emulator.h"

int main() {
  assert(al_init());
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  ALLEGRO_BITMAP *bitmap = al_create_bitmap(320, 240);
  assert(bitmap);
  SpectrumMachine machine;
  string romError;
  assert(machine.LoadROM(FindROM("48.rom"), romError));
  machine.ula.SetBitmap(bitmap);
  assert(machine.ula.Present());
  string error;
  assert(machine.Open("cobra.tap", error));
  auto originalTape = machine.tape;
  assert(machine.tapePath == "cobra.tap");
  machine.cpu.regs.PC = 0x9000;
  machine.tapePos = 21;
  machine.bus.Write(0x8000, 0x42);
  machine.bus.Write(0x4000, 0xFF);
  machine.ula.PressKey(0, 0, true);
  machine.ula.IOWrite(0xFE, 0x17);
  bool irq = false;
  machine.ula.AddCycles(TSTATES_PER_FRAME, irq);
  assert(!machine.Open("this-tape-does-not-exist.tap", error));
  assert(!error.empty() && machine.cpu.regs.PC == 0x9000);
  assert(machine.tape == originalTape && machine.tapePos == 21);
  assert(machine.bus.Read(0x8000) == 0x42);

  // Invalid Open is transactional: it must preserve the running game too.
  char path[] = "/tmp/gomas-invalid-XXXXXX";
  int fd = mkstemp(path);
  assert(fd >= 0);
  const unsigned char malformed[] = {0xFF, 0xFF, 0};
  assert(write(fd, malformed, sizeof(malformed)) == sizeof(malformed));
  close(fd);
  assert(!machine.Open(path, error));
  unlink(path);
  assert(machine.tape == originalTape && machine.cpu.regs.PC == 0x9000);

  machine.Reset();
  assert(machine.cpu.regs.PC == 0 && machine.cpu.regs.SP == 0);
  assert(machine.cpu.DataBus == &machine.bus && machine.cpu.IOBus == &machine.io);
  assert(machine.bus.Read(0x8000) == 0 && machine.bus.Read(0x4000) == 0);
  assert(machine.bus.Read(0) == machine.systemROM.pages[0][0]);
  assert(machine.ula.IORead(0xF7FE) == 0xFF);
  for (short sample : machine.ula.FrameAudio) assert(sample == 0);
  assert(machine.tapePos == 0 && machine.tape == originalTape);
  assert(machine.tapePath == "cobra.tap");
  assert(machine.ula.GetIsDirty() && machine.ula.Present()); // Bitmap survives reset.
  for (int frame = 0; frame < 100; ++frame) machine.Frame();
  assert(machine.bus.Read(0x5CB2) == 0x57 && machine.bus.Read(0x5CB3) == 0xFF);
  assert(machine.Open("manic.tap", error));
  assert(machine.tapePath == "manic.tap" && machine.tapePos == 0);
  assert(machine.cpu.regs.PC == 0 && machine.tape != originalTape);
  machine.Close();
  assert(machine.tape.empty() && machine.tapePath.empty() && machine.tapePos == 0);
  assert(!LoadTrap(machine.cpu, machine.tape, machine.tapePos));
  for (int frame = 0; frame < 100; ++frame) machine.Frame();
  assert(machine.bus.Read(0x5CB2) == 0x57 && machine.bus.Read(0x5CB3) == 0xFF);
  al_destroy_bitmap(bitmap);
  al_uninstall_system();
  puts("Machine: Open, invalid Open, Reset, tape replacement, Close and ROM boot passed");
}
