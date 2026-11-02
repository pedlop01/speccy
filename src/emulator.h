#ifndef EMULATOR_H
#define EMULATOR_H

#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <deque>
#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "rom_loader.h"
#include "ula.h"
#include "tape.h"
#include "spectrum_memory.h"

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
  SpectrumMemory memory;
  SpectrumIO io;
  Bus16 bus;
  vector<unsigned char> tape;
  size_t tapePos = 0;
  TapeDeck deck;
  bool fastLoad = true;
  std::string tapePath;
  ROMSet systemROM;

  SpectrumMachine() : memory(ula), io(ula, memory) {
    bus.AddBusComponent(&memory);
    AttachCPU();
  }
  SpectrumMachine(const SpectrumMachine &) = delete;
  SpectrumMachine &operator=(const SpectrumMachine &) = delete;

  void Reset() {
    audioOffset = 0;
    audioAvailable = 0;
    cpuClockPhase = 0;
    framesExecuted = 0;
    ++generation;
    history.clear();
    cpu = Z80();
    memory.Reset();
    ula.Configure(configuration->model);
    tapePos = 0;
    deck.SetClock(configuration->timing.cpuClockHz);
    deck.Rewind();
    AttachCPU();
    if (rewindEnabled) history.push_back(CaptureState());
  }

  bool LoadROM(const std::string &path, std::string &error, bool custom48 = false) {
    ROMSet candidate;
    if (!LoadROMSet(path, candidate, error, custom48)) return false;
    const auto &hardware = HardwareFor(candidate.model);
    if (!ValidateROMHardware(candidate, error)) return false;
    if (!SupportsHardware(candidate.model)) {
      error = "Detected " + candidate.name + " (" + std::to_string(candidate.pages.size()) +
          " ROM pages). Required hardware not implemented: " + MissingHardwareFeatures(hardware) +
          ". The current machine is unchanged.";
      return false;
    }
    if (!memory.Configure(candidate, error)) return false;
    configuration = &hardware;
    systemROM = std::move(candidate);
    if (!CanFastLoad()) fastLoad = false;
    Reset();
    error.clear();
    return true;
  }
  const MachineConfiguration &Configuration() const { return *configuration; }
  bool CanFastLoad() const { return systemROM.fastLoadCompatible; }

  unsigned CPUMultiplier() const { return cpuMultiplier; }
  bool SetCPUMultiplier(unsigned value) {
    if (value != 1 && value != 2 && value != 4) return false;
    if (cpuMultiplier != value) {
      cpuMultiplier = value;
      cpuClockPhase = 0;
      DiscardAudio();
    }
    return true;
  }
  void DiscardAudio() { audioOffset = audioAvailable = 0; }
  std::uint64_t FrameCounter() const { return framesExecuted; }

  struct State {
    std::uint64_t generation, frame;
    MachineModel model;
    Z80Registers registers;
    unsigned tstates, multiplier, clockPhase;
    SpectrumMemory::State memory;
    ULA::State ula;
    TapeDeck::TransportState deck;
    size_t tapePos, audioOffset, audioAvailable;
    bool fastLoad;
  };
  State CaptureState() const {
    return {generation, framesExecuted, configuration->model, cpu.regs, cpu.tStates,
        cpuMultiplier, cpuClockPhase, memory.CaptureState(), ula.CaptureState(), deck.CaptureState(),
        tapePos, audioOffset, audioAvailable, fastLoad};
  }
  bool RestoreState(const State &state) {
    if (state.generation != generation || state.model != configuration->model ||
        (state.multiplier != 1 && state.multiplier != 2 && state.multiplier != 4) ||
        state.clockPhase >= state.multiplier || state.memory.ram.size() != configuration->memory.RAMBytes() ||
        state.tapePos > tape.size() || state.audioOffset + state.audioAvailable > SAMPLES_PER_FRAME) return false;
    TapeDeck transport = deck;
    if (!transport.RestoreState(state.deck)) return false;
    if (!memory.RestoreState(state.memory)) return false;
    ula.RestoreState(state.ula);
    deck = std::move(transport);
    cpu.regs = state.registers; cpu.tStates = state.tstates;
    cpuMultiplier = state.multiplier; cpuClockPhase = state.clockPhase;
    tapePos = state.tapePos; fastLoad = state.fastLoad;
    audioOffset = state.audioOffset; audioAvailable = state.audioAvailable;
    framesExecuted = state.frame;
    AttachCPU();
    while (!history.empty() && history.back().frame > framesExecuted) history.pop_back();
    return true;
  }
  void EnableRewind(bool enabled = true) {
    rewindEnabled = enabled;
    history.clear();
    if (enabled) history.push_back(CaptureState());
  }
  bool CanRewind() const { return !history.empty() && history.front().frame < framesExecuted; }
  size_t HistorySize() const { return history.size(); }
  bool RewindOneSecond() {
    if (!CanRewind()) return false;
    const std::uint64_t target = framesExecuted > RewindStepFrames ? framesExecuted - RewindStepFrames : 0;
    size_t index = 0;
    while (index + 1 < history.size() && history[index + 1].frame <= target) ++index;
    State saved = history[index]; // Restore discards future entries of this branch.
    return RestoreState(saved);
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
    if (enabled && !CanFastLoad()) return;
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
  // Page-aware trap shared by normal frame execution and loader diagnostics.
  bool TryFastLoad() {
    if (!fastLoad || !CanFastLoad() || systemROM.fastLoadPage < 0 ||
        memory.ROMPage() != static_cast<unsigned>(systemROM.fastLoadPage) || cpu.regs.PC != 0x056B)
      return false;
    const bool loaded = LoadTrap(cpu, tape, tapePos);
    deck.CueOffset(tapePos);
    if (loaded) { cpu.regs.BC = 0xB001; cpu.regs.altAF = 0x0145; }
    cpu.regs.CF = loaded;
    cpu.regs.PC = 0x05E2;
    return true;
  }
  void Frame() {
    bool irq = false;
    while (!irq) {
      TryFastLoad();
      cpu.tStates = 0;
      cpu.EmulateOne();
      Advance(cpu.tStates, irq);
    }
    cpu.tStates = 0;
    cpu.INT();
    bool unused = false;
    Advance(cpu.tStates, unused);
    ++framesExecuted;
    if (rewindEnabled) {
      if (framesExecuted % HistoryStride == 0) history.push_back(CaptureState());
      while (history.size() > 1 && framesExecuted - history.front().frame > HistoryLimitFrames) history.pop_front();
    }
  }

  // Hardware frames have a fractional number of 48 kHz samples. Preserve
  // leftovers between fixed-size host fragments instead of stretching frames.
  void ReadAudio(short *output, size_t count) {
    while (count) {
      if (!audioAvailable) {
        Frame();
        audioOffset = 0;
        audioAvailable = ula.FrameAudioCount;
      }
      size_t step = std::min(count, audioAvailable);
      std::copy(ula.FrameAudio + audioOffset, ula.FrameAudio + audioOffset + step, output);
      audioOffset += step;
      audioAvailable -= step;
      output += step;
      count -= step;
    }
  }

  // Split audio integration at tape edges; playback also runs while CPU is HALTed.
  void Advance(unsigned cycles, bool &irq) {
    irq = false;
    // CPU overclock gives the Z80 more cycles per ULA/AY/tape cycle. Host
    // fast-forward is handled separately by Playback, leaving this clock alone.
    const unsigned total = cycles + cpuClockPhase;
    cycles = total / cpuMultiplier;
    cpuClockPhase = total % cpuMultiplier;
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
  enum : unsigned { HistoryStride = 10, HistoryLimitFrames = 500, RewindStepFrames = 50 };
  unsigned cpuMultiplier = 1, cpuClockPhase = 0;
  std::uint64_t framesExecuted = 0, generation = 0;
  bool rewindEnabled = false;
  std::deque<State> history;
  size_t audioOffset = 0, audioAvailable = 0;
  const MachineConfiguration *configuration = &HardwareFor(MachineModel::Spectrum48);

  void UpdateTapeInput() {
    ula.SetTapeInput(fastLoad || deck.Level(), !fastLoad && deck.HasSignal());
  }
  void AttachCPU() {
    cpu.DataBus = &bus;
    cpu.IOBus = &io;
  }
};

#endif
