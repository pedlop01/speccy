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

// Validate TAP framing before starting; malformed files must not overrun memory.
inline bool ValidateTape(const vector<unsigned char> &data) {
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

// One emulated frame produces one immutable audio fragment. Instruction
// overshoot is retained in the ULA, including interrupt acknowledge cycles.
inline void EmulateFrame(Z80 &cpu, ULA &ula, const vector<unsigned char> &tape, size_t &pos) {
  bool irq = false;
  while (!irq) {
    if (cpu.regs.PC == 0x056B) {
      const bool loaded = LoadTrap(cpu, tape, pos);
      if (loaded) {
        cpu.regs.BC = 0xB001;
        cpu.regs.altAF = 0x0145;
      }
      cpu.regs.CF = loaded;
      cpu.regs.PC = 0x05E2;
    }
    cpu.tStates = 0;
    cpu.EmulateOne();
    ula.AddCycles(cpu.tStates, irq);
  }
  cpu.tStates = 0;
  cpu.INT();
  bool unused = false;
  ula.AddCycles(cpu.tStates, unused);
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
    Reset();
    error.clear();
    return true;
  }

  void Close() {
    tape.clear();
    tapePath.clear();
    Reset();
  }

  void Frame() { EmulateFrame(cpu, ula, tape, tapePos); }

private:
  void AttachCPU() {
    cpu.DataBus = &bus;
    cpu.IOBus = static_cast<ULAIO *>(&ula);
  }
};

#endif
