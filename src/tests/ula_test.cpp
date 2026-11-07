#include <cassert>
#include <cstring>
#include <cstdio>
#include "../ula.h"
#include "../z80.h"
#include "../rom.hpp"

struct TestULA : ULA {
  using ULA::dwScanLine;
  using ULA::dwScanLineTStates;
  using ULA::dwFrameTStates;
  using ULA::audioIndex;
  using ULA::blinkState;
};

static void write(ULA &ula, unsigned address, unsigned char value) {
  static_cast<BusComponentBase *>(static_cast<ULAMemory *>(&ula))->Write(address, value);
}

static void pixel(ALLEGRO_BITMAP *bitmap, int x, int y, int r, int g, int b) {
  unsigned char actualR, actualG, actualB, alpha;
  al_unmap_rgba(al_get_pixel(bitmap, x, y), &actualR, &actualG, &actualB, &alpha);
  assert(actualR == r && actualG == g && actualB == b && alpha == 255);
}

int main() {
  assert(al_init()); // Memory bitmaps require neither a display nor audio hardware.
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  ALLEGRO_BITMAP *bitmap = al_create_bitmap(320, 240);
  assert(bitmap);
  TestULA ula;
  ula.SetBitmap(bitmap);
  assert(ula.Present() && !ula.GetIsDirty());
  bool irq = false;
  write(ula, 0x5800, 0x0A); // Red ink, blue paper.
  write(ula, 0x4000, 0x80);
  assert(!ula.GetIsDirty()); // Writes affect RAM; the beam paints the screen.
  ula.AddCycles(TSTATES_PER_FRAME, irq);
  assert(ula.GetIsDirty() && ula.Present() && !ula.GetIsDirty());
  pixel(bitmap, 32, 24, 215, 0, 0);
  pixel(bitmap, 33, 24, 0, 0, 215);
  write(ula, 0x5800, 0x4A); // BRIGHT also updates existing pixels.
  ula.AddCycles(TSTATES_PER_FRAME, irq);
  assert(ula.Present());
  pixel(bitmap, 32, 24, 255, 0, 0);
  pixel(bitmap, 33, 24, 0, 0, 255);
  write(ula, 0x5AFF, 0x04); // Last attribute and last pixel, nonlinear VRAM layout.
  write(ula, 0x57FF, 0x01);
  ula.AddCycles(TSTATES_PER_FRAME, irq);
  assert(ula.Present());
  pixel(bitmap, 287, 215, 0, 215, 0);

  ula.IOWrite(0xFE, 7);
  ula.AddCycles(TSTATES_PER_FRAME - 1, irq);
  assert(!irq);
  ula.AddCycles(1, irq);
  assert(irq && ula.dwScanLine == 0 && ula.dwScanLineTStates == 0);
  assert(ula.Present());
  pixel(bitmap, 319, 239, 215, 215, 215);
  pixel(bitmap, 288, 215, 215, 215, 215);
  pixel(bitmap, 287, 215, 0, 215, 0); // Border must preserve display contents.
  write(ula, 0x5800, 0x8A);
  for (int frame = 1; frame < 16; ++frame) ula.AddCycles(TSTATES_PER_FRAME, irq);
  assert(ula.blinkState && ula.Present());
  pixel(bitmap, 32, 24, 0, 0, 215);
  pixel(bitmap, 33, 24, 215, 0, 0);
  ula.AddCycles(224, irq);
  assert(!irq && ula.dwScanLine == 1 && ula.dwScanLineTStates == 0);

  TestULA whole, split;
  whole.IOWrite(0xFE, 0x10);
  split.IOWrite(0xFE, 0x10);
  whole.AddCycles(TSTATES_PER_FRAME + 13, irq);
  for (unsigned left = TSTATES_PER_FRAME + 13; left;) {
    unsigned step = left > 17 ? 17 : left;
    split.AddCycles(step, irq);
    left -= step;
  }
  assert(whole.dwFrameTStates == 13 && split.dwFrameTStates == 13);
  assert(whole.audioIndex == 0);
  assert(memcmp(whole.FrameAudio, split.FrameAudio, sizeof(whole.FrameAudio)) == 0);
  assert(whole.FrameAudio[0] > 16000 && whole.FrameAudio[whole.FrameAudioCount - 1] > 0);
  // Generating the next frame must leave the completed audio snapshot intact.
  short saved[SAMPLES_PER_FRAME];
  memcpy(saved, whole.FrameAudio, sizeof(saved));
  whole.IOWrite(0xFE, 0);
  whole.AddCycles(1000, irq);
  assert(memcmp(saved, whole.FrameAudio, sizeof(saved)) == 0);
  for (int frame = 0; frame < 10; ++frame) split.AddCycles(TSTATES_PER_FRAME, irq);
  assert(split.FrameAudio[0] == 0); // Constant DC decays to silence.

  // Boot the included ROM through the same CPU/ULA timing path as the application.
  ROM<0, 16384> rom;
  assert(rom.Load("../roms/48.rom") == 0);
  RAM<32768, 32768> ram;
  TestULA boot;
  Bus16 bus;
  bus.AddBusComponent(&rom);
  bus.AddBusComponent(&ram);
  bus.AddBusComponent(static_cast<ULAMemory *>(&boot));
  Z80 cpu;
  cpu.DataBus = &bus;
  cpu.IOBus = static_cast<ULAIO *>(&boot);
  for (int frame = 0; frame < 500; ++frame) {
    do {
      cpu.tStates = 0;
      cpu.EmulateOne();
      boot.AddCycles(cpu.tStates, irq);
    } while (!irq);
    cpu.tStates = 0;
    cpu.INT();
    boot.AddCycles(cpu.tStates, irq);
  }
  assert(bus.Read(0x5CB2) == 0x57 && bus.Read(0x5CB3) == 0xFF); // RAMTOP
  assert(bus.Read(0x5C53) == 0xCB && bus.Read(0x5C54) == 0x5C); // PROG
  assert(cpu.regs.SP > 0x5B00 && cpu.regs.SP < 0xFF58);
  // A square wave must be independent of instruction partitioning too.
  TestULA toneA, toneB;
  for (unsigned t = 0; t < TSTATES_PER_FRAME * 3; t += 112) {
    unsigned char level = (t / 112) % 2 ? 0x10 : 0;
    toneA.IOWrite(0xFE, level);
    toneB.IOWrite(0xFE, level);
    toneA.AddCycles(112, irq);
    for (int i = 0; i < 28; ++i) toneB.AddCycles(4, irq);
    assert(memcmp(toneA.FrameAudio, toneB.FrameAudio, sizeof(toneA.FrameAudio)) == 0);
  }
  int low = 32767, high = -32768;
  for (short sample : toneA.FrameAudio) {
    if (sample < low) low = sample;
    if (sample > high) high = sample;
  }
  assert(low < -5000 && high > 5000);
  al_destroy_bitmap(bitmap);
  al_uninstall_system();
  puts("ULA: pixels, border, FLASH, timing, audio and ROM boot passed");
}
