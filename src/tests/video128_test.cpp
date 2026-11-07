#include <cassert>
#include <cstdio>
#include <vector>
#include "../emulator.h"

struct ProbeULA : ULA {
  void At(unsigned cycles) { dwFrameTStates = cycles; }
  unsigned Phase() const { return audioPhase; }
};
static void pixel(ALLEGRO_BITMAP *bitmap, int x, int y, unsigned r, unsigned g, unsigned b) {
  unsigned char aR, aG, aB, a;
  al_unmap_rgba(al_get_pixel(bitmap, x, y), &aR, &aG, &aB, &a);
  if (!(aR == r && aG == g && aB == b && a == 255)) fprintf(stderr, "pixel %d,%d actual %u,%u,%u expected %u,%u,%u\n", x,y,aR,aG,aB,r,g,b);
  assert(aR == r && aG == g && aB == b && a == 255);
}
int main() {
  assert(al_init());
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  ALLEGRO_BITMAP *bitmap = al_create_bitmap(320, 240);
  assert(bitmap);
  ProbeULA ula;
  SpectrumMemory memory(ula);
  SpectrumIO io(ula, memory);
  ROMSet rom;
  std::string error;
  assert(LoadROMSet(FindROM("128-0.rom"), rom, error));
  ula.Configure(rom.model);
  assert(memory.Configure(rom, error));
  ula.SetBitmap(bitmap);
  bool irq = false;
  memory.Write(0x5800, 0x0A); memory.Write(0x4000, 0x80); // Red ink, blue paper.
  io.Write(0x7FFD, 7);
  memory.Write(0xD800, 0x54); memory.Write(0xC000, 0x80); // Bright green ink/red paper.
  memory.Write(0xDAFF, 4); memory.Write(0xD7FF, 1);
  ula.AddCycles(70908, irq);
  assert(ula.Present()); pixel(bitmap, 32, 24, 215, 0, 0); // Hidden writes must not repaint.
  assert(!ula.GetIsDirty());
  io.Write(0x7FFD, 0x08); // Display 7 while CPU sees bank 0 at C000.
  assert(memory.RAMBank(2) == 0 && memory.ScreenBank() == 7);
  assert(!ula.GetIsDirty()); // Bank switching does not repaint scanned pixels.
  ula.AddCycles(70908, irq);
  assert(ula.GetIsDirty() && ula.Present());
  pixel(bitmap, 32, 24, 0, 255, 0); pixel(bitmap, 33, 24, 255, 0, 0);
  pixel(bitmap, 287, 215, 0, 215, 0);
  memory.Write(0x4000, 0); // Hidden bank 5 still keeps its data.
  assert(!ula.GetIsDirty());
  io.Write(0x7FFD, 0x0F);
  memory.Write(0xC000, 0x40);
  ula.AddCycles(70908, irq);
  assert(ula.Present()); pixel(bitmap, 32, 24, 255, 0, 0); pixel(bitmap, 33, 24, 0, 255, 0);
  memory.Write(0xD800, 0xD4); // FLASH and BRIGHT on bank 7.
  ula.AddCycles(70908 * 16, irq);
  assert(ula.Present()); pixel(bitmap, 32, 24, 0, 255, 0); pixel(bitmap, 33, 24, 255, 0, 0);
  io.Write(0x7FFD, 0);
  ula.AddCycles(70908, irq);
  assert(ula.Present()); pixel(bitmap, 32, 24, 0, 0, 215);

  // Exhaust fetch slots and blanking in both screen banks, independently of CPU bank.
  for (unsigned bank : {5u, 7u}) {
    io.Write(0x7FFD, bank);
    for (unsigned n = 0; n < 6912; ++n) memory.Write(0xC000 + n, (n * 17 + bank) & 255);
    io.Write(0x7FFD, bank == 7 ? 8 : 0);
    for (unsigned y = 0; y < 192; ++y) {
      for (unsigned x = 0; x < 228; ++x) {
        ula.At(14364 + y * 228 + x);
        unsigned char expected = 0xFF;
        if (x < 128 && (x & 7) < 4) {
          unsigned col = (x / 8) * 2 + ((x & 2) >> 1);
          unsigned offset = (x & 1) ? 0x1800 + (y / 8) * 32 + col :
              ((y & 0xC0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | col;
          expected = (offset * 17 + bank) & 255;
        }
        assert(io.Read(0xFFFF) == expected);
      }
    }
    ula.At(14363); assert(io.Read(0xFFFF) == 0xFF);
    ula.At(14364 + 192 * 228); assert(io.Read(0xFFFF) == 0xFF);
  }
  io.Write(0x7FFD, 0x28); io.Write(0x7FFD, 0);
  assert(memory.ScreenBank() == 7);
  memory.Reset(); ula.Configure(MachineModel::Spectrum128);
  assert(memory.ScreenBank() == 5 && !memory.PagingLocked());
  assert(ula.Present()); pixel(bitmap, 32, 24, 0, 0, 0);

  // Exactly 48000 samples per emulated second, with fractional phase retained
  // across frames. Split instruction batches must produce identical samples.
  for (auto model : {MachineModel::Spectrum48, MachineModel::Spectrum128, MachineModel::Plus2}) {
    ProbeULA whole, split;
    whole.Configure(model); split.Configure(model);
    const auto &timing = whole.Timing();
    // The host crop follows the model display phase, including the bottom border.
    whole.SetBitmap(bitmap);
    whole.IOWrite(0xFE, 7);
    unsigned firstLine = (timing.firstDisplayTstate + timing.tstatesPerLine - 1) / timing.tstatesPerLine;
    unsigned firstCrop = firstLine - 24;
    whole.AddCycles(firstCrop * timing.tstatesPerLine - 1, irq);
    assert(!irq && whole.Present()); pixel(bitmap, 0, 0, 0, 0, 0);
    whole.AddCycles(1, irq);
    assert(!irq && whole.Present()); pixel(bitmap, 0, 0, 215, 215, 215);
    whole.AddCycles((firstCrop + 239) * timing.tstatesPerLine - firstCrop * timing.tstatesPerLine, irq);
    assert(!irq && whole.Present()); pixel(bitmap, 319, 239, 215, 215, 215);
    whole.Reset();
    uint64_t total = 0;
    for (unsigned frame = 0; frame < 300; ++frame) {
      unsigned char level = frame & 1 ? 0x10 : 0;
      whole.IOWrite(0xFE, level); split.IOWrite(0xFE, level);
      whole.AddCycles(timing.TStatesPerFrame(), irq); assert(irq);
      for (unsigned left = timing.TStatesPerFrame(); left;) {
        unsigned step = std::min(left, 19u);
        split.AddCycles(step, irq); left -= step;
      }
      assert(whole.FrameAudioCount == split.FrameAudioCount);
      assert(memcmp(whole.FrameAudio, split.FrameAudio, sizeof(whole.FrameAudio)) == 0);
      total += whole.FrameAudioCount;
      uint64_t units = uint64_t(frame + 1) * timing.TStatesPerFrame() * AUDIO_SAMPLE_RATE;
      assert(total == units / timing.cpuClockHz);
      assert(whole.Phase() == units % timing.cpuClockHz);
    }
    whole.Reset(); whole.AddCycles(timing.cpuClockHz, irq);
    assert(whole.Phase() == 0);
  }

  // Host fragments of any size must preserve the same sequence, and reset
  // must discard the partial frame left over from the previous session.
  SpectrumMachine a, b;
  assert(a.LoadROM(FindROM("48.rom"), error)); assert(b.LoadROM(FindROM("48.rom"), error));
  std::vector<short> large(9600), pieces(9600);
  a.ReadAudio(large.data(), large.size());
  for (unsigned n = 0; n < pieces.size();) {
    unsigned count = std::min<unsigned>(137, pieces.size() - n);
    b.ReadAudio(pieces.data() + n, count); n += count;
  }
  assert(large == pieces && a.cpu.regs.PC == b.cpu.regs.PC);
  a.Reset(); b.Reset();
  a.ReadAudio(large.data(), large.size()); b.ReadAudio(pieces.data(), pieces.size());
  assert(large == pieces);
  al_destroy_bitmap(bitmap);
  al_uninstall_system();
  puts("128K video: shadow screen, hidden writes, FLASH, floating bus, clocked audio and host fragments passed");
}
