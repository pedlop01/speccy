#include <cassert>
#include <cstdio>
#include <unistd.h>
#include "../emulator.h"

static void block(std::vector<unsigned char> &tap, unsigned char flag, const std::vector<unsigned char> &payload) {
  unsigned size = payload.size() + 2;
  tap.push_back(size & 255); tap.push_back(size >> 8); tap.push_back(flag);
  unsigned char checksum = flag;
  for (auto b : payload) { tap.push_back(b); checksum ^= b; }
  tap.push_back(checksum);
}

int main() {
  std::vector<unsigned char> bytes;
  block(bytes, 0xFF, {0x80});
  block(bytes, 0xFF, {0x37});
  TapeDeck deck;
  assert(deck.Mount(bytes) && deck.Blocks().size() == 2);
  assert(deck.Blocks()[0].checksumOK && deck.Offset() == 0);
  assert(deck.Play() && !deck.Level() && deck.UntilEdge() == 2168);
  deck.Advance(2167);
  assert(!deck.Level() && deck.UntilEdge() == 1);
  deck.Pause();
  double paused = deck.Seconds();
  deck.Advance(9000);
  assert(deck.GetState() == TapeDeck::State::Paused && deck.Seconds() == paused);
  assert(deck.Play()); deck.Advance(1);
  assert(deck.Level() && deck.UntilEdge() == 2168);
  deck.Advance(2168 * 3222);
  assert(deck.UntilEdge() == 667);
  deck.Advance(667); assert(deck.UntilEdge() == 735);
  deck.Advance(735); assert(deck.UntilEdge() == 1710); // Flag FF: first bit is 1.
  deck.Advance(1710); assert(deck.UntilEdge() == 1710);
  deck.Advance(1710 * 15); assert(deck.UntilEdge() == 1710); // First payload bit.
  deck.Advance(1710 * 2); assert(deck.UntilEdge() == 855); // Following zero bit.
  deck.Stop(); assert(deck.Seconds() == 0 && deck.CurrentBlock() == 0);
  assert(deck.Cue(1) && deck.Offset() == 5);
  assert(!deck.Cue(3) && deck.CurrentBlock() == 1);
  assert(deck.Play());
  deck.Advance(static_cast<unsigned>(deck.Blocks()[1].duration));
  assert(deck.GetState() == TapeDeck::State::End && !deck.Play());
  deck.Rewind(); assert(deck.CurrentBlock() == 0 && deck.GetState() == TapeDeck::State::Stopped);

  // Advancing by whole blocks or irregular instruction sizes gives identical output.
  TapeDeck one, split;
  assert(one.Mount(bytes) && split.Mount(bytes));
  one.Play(); split.Play();
  unsigned total = static_cast<unsigned>(one.Blocks()[0].duration) + 3200;
  one.Advance(total);
  for (unsigned left = total; left;) {
    unsigned step = std::min(left, 113u);
    split.Advance(step); left -= step;
  }
  assert(one.CurrentBlock() == 1 && split.CurrentBlock() == 1);
  assert(one.Level() == split.Level() && one.UntilEdge() == split.UntilEdge());
  assert(one.Seconds() == split.Seconds());

  std::vector<unsigned char> header(17, 0);
  header[0] = 3; // CODE header with a printable label and payload length.
  const char *name = "LEVEL ONE ";
  for (int i = 0; i < 10; ++i) header[i + 1] = name[i];
  header[11] = 123;
  std::vector<unsigned char> named;
  block(named, 0, header);
  TapeDeck metadata;
  assert(metadata.Mount(named));
  assert(metadata.Blocks()[0].label == "Code  LEVEL ONE (123 bytes)");
  metadata.Play(); metadata.Advance(2168 * 8063);
  assert(metadata.UntilEdge() == 667); // Longer header pilot.
  named.back() ^= 1;
  assert(metadata.Mount(named) && !metadata.Blocks()[0].checksumOK);

  // Verify real ROM LD-BYTES reads pulses, rather than the instant-load trap.
  std::vector<unsigned char> tap, payload;
  for (unsigned i = 0; i < 32; ++i) payload.push_back((i * 71) & 255);
  block(tap, 0xFF, payload);
  block(tap, 0xFF, {0x5A, 0xA5}); // A separately cueable multi-load part.
  char path[] = "/tmp/gomas-tape-XXXXXX";
  int fd = mkstemp(path); assert(fd >= 0);
  assert(write(fd, tap.data(), tap.size()) == static_cast<ssize_t>(tap.size())); close(fd);
  SpectrumMachine machine;
  assert(machine.rom.Load("48.rom") == 0);
  std::string error;
  assert(machine.Open(path, error)); unlink(path);
  assert(machine.fastLoad && !machine.PlayTape());
  machine.SetFastLoad(false);
  auto loadROM = [&](size_t index, unsigned address, unsigned length) {
    assert(machine.CueTape(index));
    machine.cpu.regs.PC = 0x0556;
    machine.cpu.regs.SP = 0xFF00;
    machine.bus.Write(0xFF00, 0x00); machine.bus.Write(0xFF01, 0x90);
    machine.cpu.regs.A = 0xFF;
    machine.cpu.regs.CF = 1;
    machine.cpu.regs.IX = address;
    machine.cpu.regs.DE = length;
    assert(machine.PlayTape());
    unsigned cycles = 0;
    while (machine.cpu.regs.PC != 0x9000 && cycles < 30000000) {
      machine.cpu.tStates = 0;
      machine.cpu.EmulateOne();
      bool irq = false;
      machine.Advance(machine.cpu.tStates, irq);
      cycles += machine.cpu.tStates;
    }
    assert(machine.cpu.regs.PC == 0x9000 && machine.cpu.regs.CF);
    assert(machine.cpu.regs.DE == 0 && machine.cpu.regs.IX == address + length);
    machine.PauseTape();
    assert(machine.deck.GetState() == TapeDeck::State::Paused);
  };
  loadROM(0, 0x8000, payload.size());
  for (unsigned i = 0; i < payload.size(); ++i) assert(machine.bus.Read(0x8000 + i) == payload[i]);
  double position = machine.deck.Seconds();
  machine.Frame();
  assert(machine.deck.Seconds() == position && machine.deck.GetState() == TapeDeck::State::Paused);
  assert(machine.cpu.regs.PC != 0x9000); // Pausing the tape does not pause the Spectrum.
  unsigned short pc = machine.cpu.regs.PC;
  assert(machine.CueTape(1) && machine.cpu.regs.PC == pc); // Seeking never resets CPU.
  loadROM(1, 0x8100, 2);
  assert(machine.bus.Read(0x8100) == 0x5A && machine.bus.Read(0x8101) == 0xA5);
  machine.SetFastLoad(true);
  assert(machine.fastLoad && !machine.deck.IsPlaying());
  machine.Reset(); assert(machine.fastLoad && machine.deck.CurrentBlock() == 0);
  machine.SetFastLoad(false); machine.Reset(); assert(!machine.fastLoad);
  machine.Close(); assert(machine.deck.Blocks().empty() && !machine.PlayTape());
  puts("Tape: pulse timings, transport, block seeking and two ROM pulse loads passed");
}
