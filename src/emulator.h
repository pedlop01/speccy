#ifndef EMULATOR_H
#define EMULATOR_H

#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"
#include "tape.h"

inline bool LoadTrap(Z80 &cpu, const vector<unsigned char> &data, size_t &pos) {
  if (pos == data.size()) return false;
  size_t length = data[pos] | (data[pos + 1] << 8);
  pos += 2;
  const size_t end = pos + length;
  const unsigned char flag = data[pos++];
  unsigned char checksum = flag;
  const unsigned char expectedFlag = cpu.regs.altAF >> 8;
  const bool load = (cpu.regs.altAF & 1) != 0;
  bool success = flag == expectedFlag && length - 2 == cpu.regs.DE;
  while (pos < end - 1) {
    unsigned char value = data[pos++];
    checksum ^= value;
    if (flag == expectedFlag && cpu.regs.DE) {
      if (load) cpu.DataBus->Write(cpu.regs.IX, value);
      else if (cpu.DataBus->Read(cpu.regs.IX) != value) success = false;
      ++cpu.regs.IX;
      --cpu.regs.DE;
    }
  }
  success = success && checksum == data[pos];
  pos = end;
  return success;
}

// Addresses of bus components remain stable across reset/open/close.
class SpectrumMachine {
public:
  Z80 cpu;
  ULA ula;
  ROM<0, 16384> rom;
  RAM<32768, 32768> ram;
  Bus16 bus;
  vector<unsigned char> tape;
  size_t tapePos = 0;
  TapeDeck deck;
  bool fastLoad = true;
  std::string tapePath;

  SpectrumMachine() {
    bus.AddBusComponent(&rom);
    bus.AddBusComponent(&ram);
    bus.AddBusComponent(static_cast<ULAMemory *>(&ula));
    AttachCPU();
  }
  SpectrumMachine(const SpectrumMachine &) = delete;
  SpectrumMachine &operator=(const SpectrumMachine &) = delete;

  void Reset() {
    cpu = Z80();
    ram.Clear();
    ula.Reset();
    tapePos = 0;
    deck.Rewind();
    AttachCPU();
  }

  bool Open(const std::string &path, std::string &error) {
    std::ifstream input(path, std::ios::binary);
    if (!input) { error = "Cannot open TAP file: " + path; return false; }
    vector<unsigned char> candidate((std::istreambuf_iterator<char>(input)), {});
    if (input.bad() || candidate.empty() || !ValidateTape(candidate)) {
      error = "Invalid or empty TAP file: " + path;
      return false;
    }
    tape.swap(candidate);
    tapePath = path;
    deck.Mount(tape);
    Reset();
    error.clear();
    return true;
  }

  void Close() {
    tape.clear();
    tapePath.clear();
    deck.Mount(tape);
    Reset();
  }

  void SetFastLoad(bool enabled) {
    if (fastLoad == enabled) return;
    deck.Stop();
    tapePos = deck.Offset();
    fastLoad = enabled;
    UpdateTapeInput();
  }
  bool PlayTape() { return !fastLoad && deck.Play(); }
  void PauseTape() { deck.Pause(); UpdateTapeInput(); }
  void StopTape() { deck.Stop(); tapePos = deck.Offset(); UpdateTapeInput(); }
  bool CueTape(size_t block) {
    if (!deck.Cue(block)) return false;
    tapePos = deck.Offset();
    UpdateTapeInput();
    return true;
  }
  void Frame() {
    bool irq = false;
    while (!irq) {
      if (fastLoad && cpu.regs.PC == 0x056B) {
        const bool loaded = LoadTrap(cpu, tape, tapePos);
        deck.CueOffset(tapePos);
        if (loaded) { cpu.regs.BC = 0xB001; cpu.regs.altAF = 0x0145; }
        cpu.regs.CF = loaded;
        cpu.regs.PC = 0x05E2;
      }
      cpu.tStates = 0;
      cpu.EmulateOne();
      Advance(cpu.tStates, irq);
    }
    cpu.tStates = 0;
    cpu.INT();
    bool unused = false;
    Advance(cpu.tStates, unused);
  }

  // Split audio integration at tape edges; playback also runs while CPU is HALTed.
  void Advance(unsigned cycles, bool &irq) {
    irq = false;
    while (cycles) {
      UpdateTapeInput();
      unsigned step = cycles;
      if (!fastLoad && deck.IsPlaying()) step = std::min(step, deck.UntilEdge());
      bool frame = false;
      ula.AddCycles(step, frame);
      irq = irq || frame;
      if (!fastLoad) { deck.Advance(step); tapePos = deck.Offset(); }
      cycles -= step;
    }
    UpdateTapeInput();
  }

private:
  void UpdateTapeInput() {
    ula.SetTapeInput(fastLoad || deck.Level(), !fastLoad && deck.HasSignal());
  }
  void AttachCPU() {
    cpu.DataBus = &bus;
    cpu.IOBus = static_cast<ULAIO *>(&ula);
  }
};

#endif
