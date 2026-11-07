#ifndef PLAYBACK_H
#define PLAYBACK_H
#include <array>
#include "emulator.h"

// Host playback speed affects the entire virtual timeline. CPU multiplier only
// affects how many instructions execute within each hardware frame.
class Playback {
public:
  explicit Playback(SpectrumMachine &machine) : machine(machine) {}
  bool paused = false;
  bool Rewinding() const { return rewinding; }
  bool BeginRewind() {
    if (rewinding || !machine.CanRewind()) return false;
    rewinding = true; rewindTime = 0; DiscardAudio();
    machine.RewindPrevious(); return true;
  }
  bool EndRewind() {
    if (!rewinding) return false;
    rewinding = false; machine.DiscardAudio(); DiscardAudio(); return true;
  }
  bool TickRewind(double elapsed) {
    if (!rewinding) return false;
    // 10 emulated frames every 50 ms: about 4x reverse. Bound catch-up
    // after a stalled window so input remains responsive.
    rewindTime += std::max(0.0, std::min(elapsed, 0.1));
    bool changed = false;
    while (rewindTime >= 0.05) {
      rewindTime -= 0.05;
      changed = machine.RewindPrevious() || changed;
    }
    return changed;
  }
  double Speed() const { return speed; }
  std::string SpeedText() const {
    if (speed == 0.25) return "0.25x";
    if (speed == 0.5) return "0.5x";
    return std::to_string(static_cast<unsigned>(speed)) + "x";
  }
  void DiscardAudio() { repeatsLeft = 0; }
  bool SetSpeed(double value) {
    for (double allowed : {0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0}) {
      if (value == allowed) { speed = value; DiscardAudio(); return true; }
    }
    return false;
  }
  bool IncreaseSpeed() {
    if (speed == 16) return false;
    return SetSpeed(speed * 2);
  }
  bool DecreaseSpeed() {
    if (speed == 0.25) return false;
    return SetSpeed(speed / 2);
  }
  bool NormalSpeed() {
    if (speed == 1) return false;
    return SetSpeed(1);
  }
  void ReadAudio(short *output, size_t count) {
    if (paused || rewinding) { std::fill(output, output + count, 0); return; }
    if (speed == 1) { machine.ReadAudio(output, count); return; }
    if (speed < 1) {
      const unsigned repeats = static_cast<unsigned>(1 / speed);
      while (count) {
        if (!repeatsLeft) {
          machine.ReadAudio(&heldSample, 1);
          repeatsLeft = repeats;
        }
        size_t step = std::min(count, size_t(repeatsLeft));
        std::fill(output, output + step, heldSample);
        repeatsLeft -= step; output += step; count -= step;
      }
      return;
    }
    const unsigned multiplier = static_cast<unsigned>(speed);
    std::array<short, SAMPLES_PER_FRAME * 16> samples;
    while (count) {
      size_t step = std::min(count, size_t(SAMPLES_PER_FRAME));
      machine.ReadAudio(samples.data(), step * multiplier);
      // Average each group of virtual samples; the host stays at 48 kHz.
      for (size_t i = 0; i < step; ++i) {
        int sum = 0;
        for (unsigned j = 0; j < multiplier; ++j) sum += samples[i * multiplier + j];
        output[i] = static_cast<short>(sum / int(multiplier));
      }
      output += step; count -= step;
    }
  }
private:
  bool rewinding = false;
  double rewindTime = 0;
  double speed = 1;
  unsigned repeatsLeft = 0;
  short heldSample = 0;
  SpectrumMachine &machine;
};
#endif
