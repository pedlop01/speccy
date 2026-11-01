#include <cassert>
#include <cstdio>
#include "../emulator.h"
static void reg(AY8912 &ay, unsigned r, unsigned v) { ay.Select(r); ay.Write(v); }
int main() {
  AY8912 ay;
  ay.Configure(8000, 8000); // One internal tick per eight CPU clocks.
  const unsigned masks[] = {255,15,255,15,255,15,31,255,31,31,31,255,255,15};
  for (unsigned r = 0; r < 14; ++r) {
    reg(ay, r, 255); assert(ay.Read() == masks[r]);
  }
  reg(ay, 7, 0); reg(ay, 14, 0x42); assert(ay.Read() == 0xFF);
  reg(ay, 7, 0x40); ay.Select(14); assert(ay.Read() == 0x42);
  reg(ay, 15, 0x42); assert(ay.Read() == 0xFF);
  for (unsigned ch = 0; ch < 3; ++ch) {
    ay.Reset(); reg(ay, ch * 2, 3); reg(ay, 8 + ch, 15);
    reg(ay, 7, 0x3F & ~(1u << ch));
    assert(ay.Output() == 0);
    ay.Advance(23); assert(ay.Output() == 0);
    ay.Advance(1); assert(ay.Output() == 2048);
    ay.Advance(24); assert(ay.Output() == 0);
    reg(ay, 7, 0x3F); assert(ay.Output() == 2048); // Both disabled = gate high.
    reg(ay, 8 + ch, 0); assert(ay.Output() == 0);
  }
  ay.Reset(); reg(ay, 6, 1);
  assert(ay.NoiseState() == 1);
  ay.Advance(15); assert(ay.NoiseState() == 1);
  ay.Advance(1); assert(ay.NoiseState() == 65536);
  ay.Advance(16); assert(ay.NoiseState() == 32768);
  for (unsigned i = 0; i < 10000; ++i) { ay.Advance(16); assert(ay.NoiseState() != 0); }

  // All shapes: attack direction, ramp length, repeat/alternate/hold endpoints.
  for (unsigned shape = 0; shape < 16; ++shape) {
    ay.Reset(); reg(ay, 11, 2); reg(ay, 13, shape);
    for (unsigned step = 0; step < 64; ++step) {
      unsigned ramp = step / 16, n = step % 16;
      bool attack = shape & 4;
      unsigned expected;
      if (ramp && !(shape & 8)) expected = 0;
      else if (ramp && (shape & 1)) expected = attack ^ bool(shape & 2) ? 15 : 0;
      else {
        if ((shape & 2) && (ramp & 1)) attack = !attack;
        expected = attack ? n : 15 - n;
      }
      assert(ay.EnvelopeLevel() == expected);
      ay.Advance(32); // Period 2: each envelope step takes 32 AY input clocks.
    }
    reg(ay, 13, shape); assert(ay.EnvelopeLevel() == ((shape & 4) ? 0 : 15));
  }
  // Period zero behaves as period one for tone; envelope zero is half period one.
  ay.Reset(); reg(ay, 7, 0x3E); reg(ay, 8, 15);
  ay.Advance(8); assert(ay.Output() == 2048);
  reg(ay, 13, 12); ay.Advance(8); assert(ay.EnvelopeLevel() == 1);

  ULA ula;
  SpectrumMemory memory(ula); SpectrumIO io(ula, memory);
  ula.Configure(MachineModel::Spectrum128);
  io.Write(0xFFFD, 1); io.Write(0xBFFD, 0xFF); assert(io.Read(0xFFFD) == 15);
  io.Write(0xC001, 2); io.Write(0x8001, 0x42); assert(io.Read(0xC001) == 0x42);
  io.Write(0xC003, 1); io.Write(0x8003, 0x91); assert(io.Read(0xFFFD) == 0x42); // A1 rejects.
  ula.Configure(MachineModel::Spectrum48);
  io.Write(0xFFFD, 2); io.Write(0xBFFD, 0x42);
  assert(!ula.ay.Enabled() && io.Read(0xFFFD) == 0xFF);

  ULA whole, split;
  whole.Configure(MachineModel::Spectrum128); split.Configure(MachineModel::Spectrum128);
  for (auto *u : {&whole, &split}) {
    reg(u->ay, 0, 123); reg(u->ay, 2, 71); reg(u->ay, 4, 237);
    reg(u->ay, 6, 11); reg(u->ay, 7, 0); reg(u->ay, 8, 15);
    reg(u->ay, 9, 12); reg(u->ay, 10, 16); reg(u->ay, 11, 200); reg(u->ay, 13, 14);
    u->IOWrite(0xFE, 0x10); u->SetTapeInput(true, true);
  }
  bool irq = false;
  int low = 32767, high = -32768;
  for (unsigned frame = 0; frame < 5; ++frame) {
    whole.AddCycles(70908, irq);
    for (unsigned left = 70908; left;) { unsigned n = std::min(left, 7u); split.AddCycles(n, irq); left -= n; }
    assert(whole.FrameAudioCount == split.FrameAudioCount);
    assert(!memcmp(whole.FrameAudio, split.FrameAudio, sizeof(whole.FrameAudio)));
    for (unsigned n = 0; n < whole.FrameAudioCount; ++n) {
      low = std::min(low, int(whole.FrameAudio[n])); high = std::max(high, int(whole.FrameAudio[n]));
    }
  }
  assert(high - low > 1000);
  whole.Reset(); assert(whole.ay.Output() == 0);
  puts("AY: registers, ports, tones, noise, 16 envelope shapes, reset and integrated mixing passed");
}
