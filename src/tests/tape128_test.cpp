#include <cassert>
#include <cstdio>
#include "../emulator.h"
static void block(std::vector<unsigned char> &tap, const std::vector<unsigned char> &bytes) {
  unsigned size = bytes.size() + 2; tap.push_back(size & 255); tap.push_back(size >> 8);
  tap.push_back(255); unsigned char checksum = 255;
  for (auto b : bytes) { tap.push_back(b); checksum ^= b; } tap.push_back(checksum);
}
int main() {
  std::vector<unsigned char> payload(32), tap;
  for (unsigned n = 0; n < payload.size(); ++n) payload[n] = n * 71;
  block(tap, payload); block(tap, {0x5A, 0xA5});
  for (const char *file : {"48.rom", "128-0.rom", "128-spanish-0.rom", "plus2-0.rom"}) {
    for (bool fast : {true, false}) {
      SpectrumMachine m; std::string error;
      assert(m.LoadROM(FindROM(file), error));
      m.tape = tap; assert(m.deck.Mount(m.tape));
      const bool paged = m.Configuration().model != MachineModel::Spectrum48;
      m.cpu.regs.PC = 0x056B;
      if (paged) { assert(!m.TryFastLoad()); assert(m.tapePos == 0 && m.cpu.regs.PC == 0x056B); }
      m.SetFastLoad(fast);
      for (unsigned part = 0; part < 2; ++part) {
        if (paged) m.io.Write(0x7FFD, 0x10 | (part ? 6 : 3));
        assert(m.CueTape(part));
        m.cpu.regs.PC = 0x0556; m.cpu.regs.SP = 0xBF00;
        m.bus.Write(0xBF00, 0); m.bus.Write(0xBF01, 0x90);
        m.cpu.regs.A = 255; m.cpu.regs.CF = 1;
        m.cpu.regs.IX = 0xC000; m.cpu.regs.DE = part ? 2 : payload.size();
        if (!fast) assert(m.PlayTape());
        unsigned cycles = 0;
        while (m.cpu.regs.PC != 0x9000 && cycles < 30000000) {
          m.TryFastLoad(); m.cpu.tStates = 0; m.cpu.EmulateOne();
          bool irq = false; m.Advance(m.cpu.tStates, irq); cycles += m.cpu.tStates;
        }
        assert(m.cpu.regs.PC == 0x9000 && m.cpu.regs.CF && m.cpu.regs.DE == 0);
        if (fast) assert(cycles < 1000); else assert(cycles > 7000000);
        assert(m.bus.Read(0xC000) == (part ? 0x5A : payload[0]));
        assert(m.bus.Read(0xC001) == (part ? 0xA5 : payload[1]));
        if (!part) for (unsigned n = 0; n < payload.size(); ++n) assert(m.bus.Read(0xC000 + n) == payload[n]);
        m.PauseTape(); assert(!m.deck.IsPlaying());
      }
      if (paged) {
        assert(m.memory.ReadBank(3, 1) == payload[1] && m.memory.ReadBank(6, 1) == 0xA5);
      }
      printf("%s %s: two separate tape loads and bank contents passed\n", file, fast ? "fast" : "pulses");
    }
  }
  TapeDeck slow, reference, split;
  assert(slow.Mount(tap) && reference.Mount(tap) && split.Mount(tap));
  slow.SetClock(3546900); split.SetClock(3546900);
  slow.Play(); split.Play(); reference.Play();
  unsigned first = slow.UntilEdge(); slow.Advance(first - 1); assert(!slow.Level());
  slow.Advance(1); assert(slow.Level());
  slow.Rewind(); slow.Play();
  slow.Advance(3546900); reference.Advance(3500000);
  for (unsigned left = 3546900; left;) { unsigned n = std::min(left, 113u); split.Advance(n); left -= n; }
  assert(slow.Seconds() == 1.0 && slow.Seconds() == reference.Seconds());
  assert(slow.Level() == reference.Level() && slow.Level() == split.Level());
  assert(slow.UntilEdge() == split.UntilEdge() && slow.Seconds() == split.Seconds());
  puts("128K tape: page-aware traps, real ROM pulses, multicarga and CPU clock conversion passed");
}
