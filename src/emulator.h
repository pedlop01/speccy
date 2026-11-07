#ifndef EMULATOR_H
#define EMULATOR_H

#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <unistd.h>
#include <cstdio>
#include "snapshot.h"
#include "extended_snapshot.h"
#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "rom_loader.h"
#include "ula.h"
#include "tape.h"
#include "tape_loader.h"
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
    snapshotHalted=false;
    cpu = Z80();
    memory.Reset();io.floppy.Reset();
    ula.Configure(configuration->model);
    deck.SetClock(configuration->timing.cpuClockHz);
    deck.Rewind();
    tapePos = deck.Offset();
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
    if(configuration->model!=MachineModel::Plus3){io.floppy.Eject(0);io.floppy.Eject(1);}
    Reset();
    error.clear();
    return true;
  }
  const MachineConfiguration &Configuration() const { return *configuration; }
  bool CanFastLoad() const { return systemROM.fastLoadCompatible && deck.FastLoadCompatible(); }

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
  std::uint64_t SessionGeneration() const{return generation;}
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
    std::shared_ptr<const std::map<unsigned,std::vector<unsigned char>>> levels;
    bool snapshotHalted;
    Disks::Controller::State floppy;
  };
  State CaptureState() const {
    return {generation, framesExecuted, configuration->model, cpu.regs, cpu.tStates,
        cpuMultiplier, cpuClockPhase, memory.CaptureState(), ula.CaptureState(), deck.CaptureState(),
        tapePos, audioOffset, audioAvailable, fastLoad, levels, snapshotHalted,io.floppy.CaptureState()};
  }
  bool RestoreState(const State &state) {
    if (state.generation != generation || state.model != configuration->model ||
        (state.multiplier != 1 && state.multiplier != 2 && state.multiplier != 4) ||
        state.clockPhase >= state.multiplier || state.memory.ram.size() != configuration->memory.RAMBytes() ||
        state.tapePos > tape.size() || state.audioOffset + state.audioAvailable > SAMPLES_PER_FRAME) return false;
    TapeDeck transport = deck;
    if (!transport.RestoreState(state.deck)) return false;
    Disks::Controller floppy;
    if(!floppy.RestoreState(state.floppy))return false;
    if (!memory.RestoreState(state.memory)) return false;
    io.floppy=std::move(floppy);
    ula.RestoreState(state.ula);
    deck = std::move(transport);
    cpu.regs = state.registers; cpu.tStates = state.tstates;
    cpuMultiplier = state.multiplier; cpuClockPhase = state.clockPhase;
    tapePos = state.tapePos; fastLoad = state.fastLoad;
    audioOffset = state.audioOffset; audioAvailable = state.audioAvailable;
    framesExecuted = state.frame;
    levels=state.levels;snapshotHalted=state.snapshotHalted;
    AttachCPU();
    while (!history.empty() && history.back().frame > framesExecuted) history.pop_back();
    return true;
  }
  // Prepare a complete session before touching the running machine. Saved
  // media is embedded; original ROM/TAP paths are informational only.
  bool ImportSession(ROMSet rom, std::vector<unsigned char> bytes, std::string path,
      const State &saved, std::string &error) {
    if (!ValidateROMHardware(rom, error)) return false;
    if (!SupportsHardware(rom.model) || saved.model != rom.model || !ValidateTape(bytes)) {
      error = "Invalid or unsupported saved machine"; return false;
    }
    std::unique_ptr<SpectrumMachine> candidate(new SpectrumMachine);
    if (!candidate->memory.Configure(rom, error)) return false;
    candidate->configuration = &HardwareFor(rom.model);
    candidate->systemROM = rom;
    candidate->tape = bytes;
    if (!candidate->deck.Mount(candidate->tape)) { error = "Invalid saved tape"; return false; }
    candidate->Reset();
    State state = saved; state.generation = candidate->generation;
    if (!candidate->RestoreState(state)) { error = "Invalid saved state"; return false; }
    // All data and transport validation succeeded. Keep this machine's buses,
    // display bitmap and ULA object at their original addresses.
    if (!memory.Configure(rom, error)) return false;
    configuration = &HardwareFor(rom.model);
    systemROM = std::move(rom); tape = std::move(bytes); tapePath = std::move(path);
    deck.Mount(tape); Reset();
    state.generation = generation;
    RestoreState(state);
    EnableRewind(rewindEnabled);
    error.clear(); return true;
  }
  void EnableRewind(bool enabled = true) {
    rewindEnabled = enabled;
    history.clear();
    if (enabled) history.push_back(CaptureState());
  }
  bool CanRewind() const { return !history.empty() && history.front().frame < framesExecuted; }
  size_t HistorySize() const { return history.size(); }
  // One stored frame at a time, for continuous reverse playback.
  bool RewindPrevious() {
    if (!CanRewind()) return false;
    size_t index = history.size() - 1;
    while (index && history[index].frame >= framesExecuted) --index;
    State saved = history[index];
    return RestoreState(saved);
  }
  double RewindSeconds() const {
    return history.empty() ? 0 : (framesExecuted - history.front().frame) / 50.0;
  }
  bool RewindOneSecond() {
    if (!CanRewind()) return false;
    const std::uint64_t target = framesExecuted > RewindStepFrames ? framesExecuted - RewindStepFrames : 0;
    size_t index = 0;
    while (index + 1 < history.size() && history[index + 1].frame <= target) ++index;
    State saved = history[index]; // Restore discards future entries of this branch.
    return RestoreState(saved);
  }

  bool OpenSnapshot(const Snapshots::Image &image,const std::string &path,std::string &error) {
    std::unique_ptr<SpectrumMachine> candidate(new SpectrumMachine);
    ROMSet rom=systemROM;
    if(!image.romPages.empty()) {
      rom=ROMSet();rom.model=image.model;rom.pages=image.romPages;rom.source="Embedded SZX ROM";
      rom.name=std::string(HardwareFor(image.model).name)+" (embedded ROM)";rom.fastLoadCompatible=false;rom.fastLoadPage=-1;
      std::vector<std::uint64_t> hashes;for(const auto &page:rom.pages)hashes.push_back(ROMFingerprint(page.data(),page.size()));
      for(const auto &profile:ROMProfiles())if(profile.model==rom.model && profile.fingerprints==hashes){rom.name=profile.name;rom.fastLoadCompatible=true;rom.fastLoadPage=rom.model==MachineModel::Spectrum48?0:rom.model==MachineModel::Plus3?3:1;}
    }
    if(rom.pages.empty() || rom.model!=image.model) {
      char exe[4096];ssize_t n=readlink("/proc/self/exe",exe,sizeof(exe)-1);
      std::string executable=n>0?std::string(exe,n):"";
      const char *file=image.model==MachineModel::Spectrum48?"48.rom":image.model==MachineModel::Plus2?"plus2-0.rom":image.model==MachineModel::Plus3?"plus3-0.rom":"128-0.rom";
      if(!LoadROMSet(FindROM(file,executable),rom,error))return false;
    }
    if(!candidate->memory.Configure(rom,error))return false;
    candidate->configuration=&HardwareFor(image.model);candidate->systemROM=rom;candidate->Reset();
    candidate->ula.IOWrite(0xfe,image.hasIO?image.ioData:image.border);
    for(unsigned reg=0;reg<16;++reg) {candidate->ula.ay.Select(reg);candidate->ula.ay.Write(image.ay[reg]);}
    candidate->ula.ay.Select(image.selectedAY);
    auto state=candidate->CaptureState();state.registers=image.registers;
    state.memory.ram=image.ram;state.memory.paging=image.paging;state.memory.pagingPlus3=image.pagingPlus3;
    state.ula.frameTStates=image.frameTStates;
    state.ula.line=image.frameTStates/state.ula.timing.tstatesPerLine;
    state.ula.lineTStates=image.frameTStates%state.ula.timing.tstatesPerLine;
    for(auto &color:state.ula.lineColors)color=state.ula.borderColor;
    state.fastLoad=rom.fastLoadCompatible;
    state.floppy.motor=(image.pagingPlus3&8)!=0;state.floppy.drives=image.disks;
    state.levels=std::make_shared<const std::map<unsigned,std::vector<unsigned char>>>(image.levels);
    state.snapshotHalted=image.halted;
    if(!candidate->deck.Mount(image.tape) || !candidate->deck.CueFileBlock(image.tapeBlock)){error="Invalid SZX tape block";return false;}
    state.deck=candidate->deck.CaptureState();state.tapePos=candidate->deck.Offset();
    if(!candidate->deck.FastLoadCompatible())state.fastLoad=false;
    return ImportSession(std::move(rom),image.tape,path,state,error);
  }
  bool Open(const std::string &path, std::string &error) {
    vector<unsigned char> candidate;std::string name;
    if(!TapeFiles::ReadMedia(path,candidate,name,error))return false;
    try {
      if(TapeFiles::Extension(name,".dsk"))return OpenDisk(Disks::Image::Parse(candidate),path,0,error);
      if(TapeFiles::Extension(name,".sna"))return OpenSnapshot(Snapshots::SNA(candidate),path,error);
      if(TapeFiles::Extension(name,".z80"))return OpenSnapshot(Snapshots::Z80(candidate),path,error);
      if(TapeFiles::Extension(name,".szx")) {
        auto image=Snapshots::SZX(candidate);
        if(!image.externalTape.empty()) {
          std::string mediaName;std::vector<unsigned char> media;
          if(TapeFiles::Extension(path,".zip")) {
            std::string member=TapeFiles::Normalize(ROMDirectory(name)+image.externalTape);
            if(!TapeFiles::ReadMedia(path,media,mediaName,error,member))return false;
          } else {
            std::string linked=image.externalTape;
            if(linked.empty() || linked[0]!='/')linked=ROMDirectory(path)+linked;
            if(!TapeFiles::ReadMedia(linked,media,mediaName,error))return false;
          }
          if(!ValidateTape(media)){error="Invalid tape referenced by SZX";return false;}
          image.tape=std::move(media);
        }
        return OpenSnapshot(image,path,error);
      }
      if(TapeFiles::Extension(name,".slt"))return OpenSnapshot(Snapshots::SLT(candidate),path,error);
      if(TZX::Signature(candidate))TZX::Parse(candidate);
      else if(TapeFiles::Extension(name,".tzx") || !ValidateTape(candidate))throw std::runtime_error("Invalid TAP/TZX file");
    }catch(const std::exception &e){error=e.what();return false;}
    levels.reset();
    tape.swap(candidate);tapePath=path;deck.Mount(tape);
    if(!CanFastLoad())fastLoad=false;
    Reset();error.clear();return true;
  }

  bool OpenDisk(Disks::Image disk,const std::string &path,unsigned unit,std::string &error) {
    if(unit>1){error="Invalid disk drive";return false;}
    Snapshots::Image image;image.model=MachineModel::Plus3;image.ram.resize(131072);
    image.disks[unit].image=std::make_shared<const Disks::Image>(std::move(disk));image.disks[unit].path=path;
    return OpenSnapshot(image,path,error);
  }
  bool InsertDisk(const std::string &path,unsigned unit,std::string &error) {
    std::vector<unsigned char> bytes;std::string name;if(unit>1){error="Invalid disk drive";return false;}if(!TapeFiles::ReadMedia(path,bytes,name,error))return false;
    try {
      auto image=Disks::Image::Parse(bytes);
      if(configuration->model!=MachineModel::Plus3)return OpenDisk(std::move(image),path,unit,error);
      io.floppy.Insert(unit,std::move(image),path);ClearTimeline();error.clear();return true;
    }catch(const std::exception &e){error=e.what();return false;}
  }
  void EjectDisk(unsigned unit){io.floppy.Eject(unit);ClearTimeline();}
  void ClearTimeline(){++generation;history.clear();if(rewindEnabled)history.push_back(CaptureState());DiscardAudio();}
  bool SaveDisk(unsigned unit,const std::string &path,std::string &error) {
    if(unit>1 || !io.floppy.state.drives[unit].image){error="No disk in this drive";return false;}
    std::string temporary;
    try {
      auto bytes=io.floppy.state.drives[unit].image->Encode();std::string pattern=path+".XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
      int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("Cannot create disk output");temporary=name.data();FILE *file=fdopen(fd,"wb");
      if(!file){close(fd);throw std::runtime_error("Cannot write disk image");}
      bool ok=fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();ok=(fflush(file)==0)&&ok;ok=(fsync(fileno(file))==0)&&ok;ok=(fclose(file)==0)&&ok;
      if(!ok || rename(temporary.c_str(),path.c_str()))throw std::runtime_error("Cannot save disk image");
      io.floppy.state.drives[unit].path=path;io.floppy.state.drives[unit].dirty=false;error.clear();return true;
    }catch(const std::exception &e){if(!temporary.empty())remove(temporary.c_str());error=e.what();return false;}
  }
  void Close() {
    io.floppy.Eject(0);io.floppy.Eject(1);
    levels.reset();
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
  bool ChooseTape(unsigned choice){bool ok=deck.Choose(choice);tapePos=deck.Offset();UpdateTapeInput();return ok;}
  bool PlayTape() { bool playing=!fastLoad && deck.Play();tapePos=deck.Offset();UpdateTapeInput();return playing; }
  void PauseTape() { deck.Pause(); UpdateTapeInput(); }
  void StopTape() { deck.Stop(); tapePos = deck.Offset(); UpdateTapeInput(); }
  bool CueTape(size_t block) {
    if(block==0 && deck.SelectionCount())deck.Rewind();
    else if (!deck.Cue(block)) return false;
    if(!CanFastLoad())fastLoad=false;
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
    tapePos=deck.Offset();
    if (loaded) { cpu.regs.BC = 0xB001; cpu.regs.altAF = 0x0145; }
    cpu.regs.CF = loaded;
    cpu.regs.PC = 0x05E2;
    return true;
  }
  bool TryLevelLoad() {
    if(!levels || levels->empty() || bus.Read(cpu.regs.PC)!=0xed || bus.Read((cpu.regs.PC+1)&65535)!=0xfb)return false;
    auto entry=levels->find(cpu.regs.A);
    if(entry!=levels->end())for(size_t n=0;n<entry->second.size();++n)bus.Write((cpu.regs.HL+n)&65535,entry->second[n]);
    cpu.regs.PC+=2;cpu.regs.R60++;cpu.regs.R60++;cpu.tStates+=8;return true;
  }
  // The normal frame loop and debugger share the same instruction/device path.
  // Emulated loader traps are kept active, as they are during normal execution.
  unsigned StepInstruction() {
    TryFastLoad();cpu.tStates=0;
    if(snapshotHalted){cpu.regs.R60++;cpu.tStates=4;}
    else if(!TryLevelLoad())cpu.EmulateOne();
    unsigned cycles=cpu.tStates;bool irq=false;Advance(cycles,irq);
    if(irq) {
      cpu.tStates=0;bool halted=snapshotHalted;
      if(cpu.regs.iff1A)snapshotHalted=false;
      cpu.INT(!halted);cycles+=cpu.tStates;bool unused=false;Advance(cpu.tStates,unused);
      ++framesExecuted;
      if(rewindEnabled){
        if(framesExecuted%HistoryStride==0)history.push_back(CaptureState());
        while(history.size()>1 && framesExecuted-history.front().frame>HistoryLimitFrames)history.pop_front();
      }
    }
    return cycles;
  }
  void Frame() {
    const auto frame=framesExecuted;
    do{StepInstruction();}while(framesExecuted==frame);
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
      if(configuration->devices.floppyController)io.floppy.Advance(step);
      irq = irq || frame;
      if (!fastLoad) { deck.Advance(step); tapePos = deck.Offset(); }
      cycles -= step;
    }
    UpdateTapeInput();
  }

private:
  enum : unsigned { HistoryStride = 10, HistoryLimitFrames = 500, RewindStepFrames = 50 };
  std::shared_ptr<const std::map<unsigned,std::vector<unsigned char>>> levels;
  bool snapshotHalted=false;
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
