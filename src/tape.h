#ifndef TAPE_H
#define TAPE_H

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

inline bool ValidateTape(const std::vector<unsigned char> &data) {
  size_t pos = 0;
  while (pos < data.size()) {
    if (data.size() - pos < 2) return false;
    size_t length = data[pos] | (data[pos + 1] << 8);
    pos += 2;
    if (length < 2 || length > data.size() - pos) return false;
    pos += length;
  }
  return true;
}

struct TapeBlock {
  size_t offset, length;
  std::string label;
  bool checksumOK;
  std::uint64_t duration;
};

// Standard Spectrum TAP encoding, timed in Z80 t-states.
// https://worldofspectrum.net/faq/reference/48kreference.htm
class TapeDeck {
public:
  enum class State { Stopped, Playing, Paused, End };

  bool Mount(const std::vector<unsigned char> &bytes) {
    if (!ValidateTape(bytes)) return false;
    data = &bytes;
    blocks.clear();
    for (size_t pos = 0; pos < bytes.size();) {
      size_t length = bytes[pos] | (bytes[pos + 1] << 8);
      TapeBlock block{pos, length, "Data", true, 0};
      unsigned char checksum = 0;
      for (size_t i = 0; i < length; ++i) checksum ^= bytes[pos + 2 + i];
      block.checksumOK = checksum == 0;
      const unsigned char flag = bytes[pos + 2];
      if (flag == 0 && length == 19) {
        static const char *types[] = {"Program", "Number array", "Character array", "Code"};
        unsigned type = bytes[pos + 3];
        block.label = type < 4 ? types[type] : "Header";
        block.label += "  ";
        for (size_t i = pos + 4; i < pos + 14; ++i)
          block.label += bytes[i] >= 32 && bytes[i] < 127 ? char(bytes[i]) : '?';
        while (!block.label.empty() && block.label.back() == ' ') block.label.pop_back();
        unsigned payload = bytes[pos + 14] | bytes[pos + 15] << 8;
        block.label += " (" + std::to_string(payload) + " bytes)";
      } else {
        block.label += " (" + std::to_string(length - 2) + " bytes, flag " + std::to_string(flag) + ")";
      }
      block.duration = std::uint64_t(flag < 128 ? 8063 : 3223) * 2168 + 667 + 735 + PauseCycles;
      for (size_t i = pos + 2; i < pos + 2 + length; ++i)
        for (unsigned mask = 128; mask; mask >>= 1)
          block.duration += (bytes[i] & mask) ? 3420 : 1710;
      blocks.push_back(block);
      pos += length + 2;
    }
    Rewind();
    return true;
  }

  const std::vector<TapeBlock> &Blocks() const { return blocks; }
  size_t CurrentBlock() const { return current; }
  size_t Offset() const { return current < blocks.size() ? blocks[current].offset : (data ? data->size() : 0); }
  State GetState() const { return state; }
  bool IsPlaying() const { return state == State::Playing; }
  bool Level() const { return !IsPlaying() || level; }
  bool HasSignal() const { return IsPlaying() && pulse + 1 < pulses.size(); }
  unsigned UntilEdge() const { return IsPlaying() ? remaining : 0; }
  double Progress() const {
    return current < blocks.size() ? double(elapsed) / blocks[current].duration : 1.0;
  }
  double Seconds() const { return double(elapsed) / Clock; }
  double DurationSeconds(size_t block) const { return double(blocks.at(block).duration) / Clock; }

  bool Cue(size_t block) {
    if (block > blocks.size()) return false;
    current = block;
    state = current == blocks.size() ? State::End : State::Stopped;
    BuildPulses();
    return true;
  }
  void CueOffset(size_t offset) {
    size_t index = 0;
    while (index < blocks.size() && blocks[index].offset < offset) ++index;
    Cue(index);
  }
  void Rewind() { Cue(0); }
  bool Play() {
    if (current >= blocks.size()) return false;
    state = State::Playing;
    return true;
  }
  void Pause() { if (IsPlaying()) state = State::Paused; }
  void Stop() { Cue(current); }

  void Advance(unsigned cycles) {
    while (IsPlaying() && cycles) {
      unsigned step = std::min(cycles, remaining);
      remaining -= step;
      cycles -= step;
      elapsed += step;
      if (remaining) continue;
      ++pulse;
      if (pulse == pulses.size()) {
        Cue(current + 1);
        if (current < blocks.size()) state = State::Playing;
      } else {
        level = pulse + 1 == pulses.size() ? true : !level;
        remaining = pulses[pulse];
      }
    }
  }

private:
  enum : unsigned { Clock = 3500000, PauseCycles = 3500000 };
  const std::vector<unsigned char> *data = nullptr;
  std::vector<TapeBlock> blocks;
  std::vector<unsigned> pulses;
  size_t current = 0, pulse = 0;
  unsigned remaining = 0;
  std::uint64_t elapsed = 0;
  bool level = false;
  State state = State::End;

  void BuildPulses() {
    pulses.clear();
    pulse = 0;
    elapsed = 0;
    level = false;
    remaining = 0;
    if (current >= blocks.size()) return;
    const auto &block = blocks[current];
    size_t start = block.offset + 2;
    pulses.assign((*data)[start] < 128 ? 8063 : 3223, 2168);
    pulses.push_back(667);
    pulses.push_back(735);
    for (size_t i = start; i < start + block.length; ++i) {
      for (unsigned mask = 128; mask; mask >>= 1) {
        unsigned duration = ((*data)[i] & mask) ? 1710 : 855;
        pulses.push_back(duration);
        pulses.push_back(duration);
      }
    }
    pulses.push_back(PauseCycles);
    remaining = pulses.front();
  }
};

#endif
