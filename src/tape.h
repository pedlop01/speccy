#ifndef TAPE_H
#define TAPE_H

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <stdexcept>
#include <limits>
#include <vector>
#include "tzx.h"

inline bool ValidateTape(const std::vector<unsigned char> &data) {
  if (TZX::Signature(data)) {try {TZX::Parse(data);return true;}catch(const std::exception &) {return false;}}
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

// Spectrum TAP and TZX playback, timed at the tape reference clock.
// https://worldofspectrum.net/faq/reference/48kreference.htm
class TapeDeck {
public:
  enum class State { Stopped, Playing, Paused, End };

  bool Mount(const std::vector<unsigned char> &bytes,const std::vector<unsigned> &selectedChoices={}) {
    if (TZX::Signature(bytes)) {
      std::vector<TZX::Block> parsed;
      try {parsed=TZX::Parse(bytes,selectedChoices);}catch(const std::exception &) {return false;}
      data=&bytes;choices=selectedChoices;tzx=std::move(parsed);blocks.clear();fastCompatible=true;
      size_t previous=0;
      for(const auto &b:tzx) {
        if(b.kind!=0x10 || (!blocks.empty() && b.offset<=previous))fastCompatible=false;
        previous=b.offset;
        unsigned char checksum=0;
        if(b.kind==0x10)for(size_t i=0;i<b.length;++i)checksum^=bytes[b.start+i];
        std::string label=b.label;
        if(b.kind==0x10 && b.length==19 && bytes[b.start]==0) {
          label="Header  ";for(size_t i=2;i<12;++i)label+=bytes[b.start+i]>=32 && bytes[b.start+i]<127?char(bytes[b.start+i]):'?';
        }
        blocks.push_back({b.offset,b.length,label,checksum==0,b.duration});
      }
      Cue(0);return true;
    }
    if (!ValidateTape(bytes)) return false;
    if(!selectedChoices.empty())return false;
    choices.clear();tzx.clear();fastCompatible=true;
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
    Cue(0);
    return true;
  }

  struct TransportState {
    size_t current, pulse;
    unsigned cpuClock, remaining;
    std::uint64_t clockPhase, elapsed;
    bool level;
    State state;
    std::vector<unsigned> choices;
  };
  TransportState CaptureState() const {
    return {current, pulse, cpuClock, remaining, clockPhase, elapsed, level, state, choices};
  }
  bool RestoreState(const TransportState &saved) {
    if (!saved.cpuClock || saved.clockPhase >= saved.cpuClock) return false;
    TapeDeck candidate = *this;
    if(saved.choices!=choices && (!data || !candidate.Mount(*data,saved.choices)))return false;
    if(saved.current>candidate.blocks.size())return false;
    candidate.Cue(saved.current);
    if (saved.current < candidate.blocks.size()) {
      if (saved.pulse >= candidate.pulses.size() || !saved.remaining ||
          saved.remaining > candidate.pulses[saved.pulse] || saved.elapsed > candidate.blocks[saved.current].duration ||
          saved.state == State::End) return false;
    } else if (saved.state != State::End) return false;
    candidate.pulse = saved.pulse;
    candidate.cpuClock = saved.cpuClock;
    candidate.remaining = saved.remaining;
    candidate.clockPhase = saved.clockPhase;
    candidate.elapsed = saved.elapsed;
    candidate.level = saved.level;
    candidate.state = saved.state;
    *this = std::move(candidate);
    return true;
  }
  const std::vector<TZX::Option> &Options() const {
    static const std::vector<TZX::Option> empty;
    return current<tzx.size()?tzx[current].options:empty;
  }
  size_t SelectionCount() const{return choices.size();}
  bool NeedsChoice() const {return !Options().empty();}
  bool Choose(unsigned choice) {
    if(!data || choice>=Options().size())return false;
    auto selected=choices;selected.push_back(choice);TapeDeck candidate=*this;size_t block=current;
    if(!candidate.Mount(*data,selected) || !candidate.Cue(block))return false;
    candidate.SetClock(cpuClock);candidate.Play();*this=std::move(candidate);return true;
  }
  bool FastLoadCompatible() const { return fastCompatible; }
  bool ValidOffset(size_t offset) const {
    if(data && offset==data->size())return true;
    for(const auto &b:blocks)if(b.offset==offset)return true;
    return false;
  }
  const std::vector<TapeBlock> &Blocks() const { return blocks; }
  size_t CurrentBlock() const { return current; }
  size_t Offset() const { return current < blocks.size() ? blocks[current].offset : (data ? data->size() : 0); }
  State GetState() const { return state; }
  bool IsPlaying() const { return state == State::Playing; }
  bool Level() const { return !IsPlaying() || level; }
  bool HasSignal() const { return IsPlaying() && pulse + 1 < pulses.size(); }
  // TAP durations are specified at 3.5 MHz, not in the active CPU clock.
  // Keep the fractional conversion across instructions and pulse boundaries.
  void SetClock(unsigned hz) {
    if (!hz) throw std::invalid_argument("Tape CPU clock must be positive");
    if (cpuClock != hz) { cpuClock = hz; clockPhase = 0; }
  }
  unsigned UntilEdge() const {
    if(!IsPlaying())return 0;
    const auto cycles=(std::uint64_t(remaining)*cpuClock-clockPhase+Clock-1)/Clock;
    return static_cast<unsigned>(std::min<std::uint64_t>(cycles,std::numeric_limits<unsigned>::max()));
  }
  double Progress() const {
    return current < blocks.size() ? double(elapsed) / blocks[current].duration : 1.0;
  }
  double Seconds() const { return double(elapsed) / Clock; }
  double DurationSeconds(size_t block) const { return double(blocks.at(block).duration) / Clock; }

  bool Cue(size_t block) {
    if (block > blocks.size()) return false;
    clockPhase = 0;
    current = block;
    state = current == blocks.size() ? State::End : State::Stopped;
    BuildPulses();
    return true;
  }
  bool CueFileBlock(size_t index) {
    if(tzx.empty())return Cue(index);
    for(size_t n=0;n<tzx.size();++n)if(tzx[n].sourceIndex>=index)return Cue(n);
    return false;
  }
  void CueOffset(size_t offset) {
    size_t index = 0;
    while (index < blocks.size() && blocks[index].offset < offset) ++index;
    Cue(index);
  }
  void Rewind() {if(!choices.empty() && data){Mount(*data);return;}Cue(0);}
  bool Play() {
    if(NeedsChoice())return false;
    while(current<tzx.size() && tzx[current].stop && !NeedsChoice()) Cue(current+1);
    if(NeedsChoice())return false;
    if (current >= blocks.size()) return false;
    state = State::Playing;
    return true;
  }
  void Pause() { if (IsPlaying()) state = State::Paused; }
  void Stop() { Cue(current); }

  void Advance(unsigned cycles) {
    if (!IsPlaying()) return;
    const std::uint64_t units = std::uint64_t(cycles) * Clock + clockPhase;
    cycles = static_cast<unsigned>(units / cpuClock);
    clockPhase = units % cpuClock;
    while (IsPlaying() && cycles) {
      unsigned step = std::min(cycles, remaining);
      remaining -= step;
      cycles -= step;
      elapsed += step;
      if (remaining) continue;
      ++pulse;
      if (pulse == pulses.size()) {
        const auto phase = clockPhase;
        Cue(current + 1);
        while(current<tzx.size() && tzx[current].stop48 && cpuClock!=Clock) Cue(current+1);
        clockPhase = phase;
        if (current < blocks.size() && !(current<tzx.size() && tzx[current].stop &&
            (!tzx[current].stop48 || cpuClock==Clock))) state = State::Playing;
      } else {
        level = tzx.empty() ? (pulse + 1 == pulses.size() ? true : !level) : pulseLevels[pulse];
        remaining = pulses[pulse];
      }
    }
  }

private:
  enum : unsigned { Clock = 3500000, PauseCycles = 3500000 };
  const std::vector<unsigned char> *data = nullptr;
  std::vector<TapeBlock> blocks;
  std::vector<unsigned> pulses;
  std::vector<TZX::Block> tzx;
  bool fastCompatible=true;
  std::vector<bool> pulseLevels;
  std::vector<unsigned> choices;
  size_t current = 0, pulse = 0;
  unsigned cpuClock = Clock;
  std::uint64_t clockPhase = 0;
  unsigned remaining = 0;
  std::uint64_t elapsed = 0;
  bool level = false;
  State state = State::End;

  void BuildPulses() {
    pulses.clear();pulseLevels.clear();
    pulse = 0;
    elapsed = 0;
    level = false;
    remaining = 0;
    if (current >= blocks.size()) return;
    if(!tzx.empty()) {
      TZX::Pulses(tzx[current],*data,[&](unsigned ticks,bool high){pulses.push_back(ticks);pulseLevels.push_back(high);});
      remaining=pulses.front();level=pulseLevels.front();return;
    }
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
