#ifndef AY_H
#define AY_H
#include <algorithm>
#include <array>
#include <cstdint>

// AY-3-8912: three tone generators, shared 17-bit noise and 16-step envelope.
// Clocks are converted rationally from CPU cycles; writes do not restart tone.
// References: General Instrument AY-3-8910/8912 data manual; MAME ay8910.cpp.
class AY8912 {
public:
  AY8912() { Reset(); }
  void Configure(unsigned cpu, unsigned chip) { cpuHz = cpu; chipHz = chip; Reset(); }
  bool Enabled() const { return chipHz != 0; }
  void Reset() {
    regs.fill(0); selected = 0; phase = 0;
    toneCount.fill(0); toneHigh.fill(false);
    noiseCount = envelopeCount = 0;
    noiseDivider = false; noise = 1;
    envelopeLevel = 15; envelopeDirection = -1; holding = false;
  }
  void Select(unsigned char value) { selected = value & 15; }
  void Write(unsigned char value) {
    static const unsigned char masks[16] = {255,15,255,15,255,15,31,255,31,31,31,255,255,15,255,0};
    regs[selected] = value & masks[selected];
    if (selected == 13) {
      envelopeCount = 0; holding = false;
      envelopeDirection = (value & 4) ? 1 : -1;
      envelopeLevel = (value & 4) ? 0 : 15;
    }
  }
  unsigned char Read() const {
    if (selected == 15 || (selected == 14 && !(regs[7] & 0x40))) return 0xFF;
    return regs[selected];
  }
  unsigned UntilTick() const {
    return Enabled() ? static_cast<unsigned>((std::uint64_t(cpuHz) * 8 - phase + chipHz - 1) / chipHz) : ~0u;
  }
  void Advance(unsigned cycles) {
    if (!Enabled()) return;
    phase += std::uint64_t(cycles) * chipHz;
    while (phase >= std::uint64_t(cpuHz) * 8) {
      phase -= std::uint64_t(cpuHz) * 8;
      Tick();
    }
  }
  struct State {
    unsigned cpuHz, chipHz;
    std::uint64_t phase;
    std::array<unsigned char, 16> regs;
    unsigned selected;
    std::array<unsigned, 3> toneCount;
    std::array<bool, 3> toneHigh;
    unsigned noiseCount, envelopeCount, noise;
    bool noiseDivider, holding;
    int envelopeLevel, envelopeDirection;
  };
  State CaptureState() const {
    return {cpuHz, chipHz, phase, regs, selected, toneCount, toneHigh,
        noiseCount, envelopeCount, noise, noiseDivider, holding, envelopeLevel, envelopeDirection};
  }
  bool RestoreState(const State &s) {
    if (!s.cpuHz || s.phase >= std::uint64_t(s.cpuHz) * 8 || s.selected > 15 ||
        !s.noise || s.noise > 0x1FFFF || s.noiseCount > 31 || s.envelopeCount > 131070 ||
        s.envelopeLevel < 0 || s.envelopeLevel > 15 ||
        (s.envelopeDirection != -1 && s.envelopeDirection != 1)) return false;
    for (unsigned ch = 0; ch < 3; ++ch) if (s.toneCount[ch] > 4095) return false;
    for (unsigned r : {1u,3u,5u,13u}) if (s.regs[r] > 15) return false;
    for (unsigned r : {6u,8u,9u,10u}) if (s.regs[r] > 31) return false;
    if (s.regs[15]) return false;
    cpuHz=s.cpuHz; chipHz=s.chipHz; phase=s.phase; regs=s.regs; selected=s.selected;
    toneCount=s.toneCount; toneHigh=s.toneHigh; noiseCount=s.noiseCount;
    envelopeCount=s.envelopeCount; noise=s.noise; noiseDivider=s.noiseDivider;
    holding=s.holding; envelopeLevel=s.envelopeLevel; envelopeDirection=s.envelopeDirection;
    return true;
  }
  unsigned EnvelopeLevel() const { return envelopeLevel; }
  unsigned NoiseState() const { return noise; }
  int Output() const {
    // Approximate logarithmic DAC curve; mono sum with headroom for EAR/beeper.
    static const int volume[16] = {0,16,23,32,45,64,91,128,181,256,362,512,724,1024,1448,2048};
    int output = 0;
    for (unsigned ch = 0; ch < 3; ++ch) {
      bool toneGate = toneHigh[ch] || (regs[7] & (1u << ch));
      bool noiseGate = (noise & 1) || (regs[7] & (8u << ch));
      if (toneGate && noiseGate) output += volume[(regs[8 + ch] & 16) ? envelopeLevel : regs[8 + ch] & 15];
    }
    return Enabled() ? output : 0;
  }
private:
  unsigned cpuHz = 3500000, chipHz = 0;
  std::uint64_t phase = 0;
  std::array<unsigned char, 16> regs;
  unsigned selected = 0;
  std::array<unsigned, 3> toneCount;
  std::array<bool, 3> toneHigh;
  unsigned noiseCount = 0, envelopeCount = 0, noise = 1;
  bool noiseDivider = false, holding = false;
  int envelopeLevel = 15, envelopeDirection = -1;
  void Tick() {
    for (unsigned ch = 0; ch < 3; ++ch) {
      unsigned period = regs[ch * 2] | (regs[ch * 2 + 1] << 8);
      if (++toneCount[ch] >= std::max(1u, period)) {
        toneCount[ch] = 0; toneHigh[ch] = !toneHigh[ch];
      }
    }
    if (++noiseCount >= std::max(1u, unsigned(regs[6]))) {
      noiseCount = 0; noiseDivider = !noiseDivider;
      if (!noiseDivider) {
        unsigned feedback = (noise ^ (noise >> 3)) & 1;
        noise = (noise >> 1) | (feedback << 16);
      }
    }
    // One AY envelope step per 16 input clocks * period; 16 steps per ramp.
    unsigned period = regs[11] | (regs[12] << 8);
    unsigned ticks = period ? period * 2 : 1; // Zero is half the period of one.
    if (!holding && ++envelopeCount >= ticks) {
      envelopeCount = 0;
      envelopeLevel += envelopeDirection;
      if (envelopeLevel < 0 || envelopeLevel > 15) {
        const unsigned shape = regs[13];
        if (!(shape & 8)) { envelopeLevel = 0; holding = true; }
        else if (shape & 1) {
          envelopeLevel = ((shape & 4) != 0) ^ ((shape & 2) != 0) ? 15 : 0;
          holding = true;
        } else if (shape & 2) {
          envelopeDirection = -envelopeDirection;
          envelopeLevel = envelopeDirection > 0 ? 0 : 15;
        } else envelopeLevel = envelopeDirection > 0 ? 0 : 15;
      }
    }
  }
};
#endif
