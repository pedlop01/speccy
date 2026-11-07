#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <allegro5/allegro.h>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <limits>
#include <stdexcept>
#include "platform.h"
#include "emulator.h"

namespace SaveStates {
constexpr unsigned SlotCount = 12, QuickSlot = 0, ThumbWidth = 160, ThumbHeight = 120;
constexpr unsigned MaxTape = 16 * 1024 * 1024, MaxFile = 64 * 1024 * 1024;
struct Record {
  std::uint64_t created = 0;
  std::string label, tapePath;
  ROMSet rom;
  std::vector<unsigned char> tape, thumbnail;
  SpectrumMachine::State core{};
};
struct Writer {
  std::vector<unsigned char> bytes;
  void u8(unsigned v) { bytes.push_back(v); }
  void u16(unsigned v) { u8(v); u8(v >> 8); }
  void u32(std::uint32_t v) { for (unsigned i=0;i<4;++i) u8(v >> (i*8)); }
  void u64(std::uint64_t v) { for (unsigned i=0;i<8;++i) u8(v >> (i*8)); }
  void raw(const unsigned char *p, size_t n) { if(n) bytes.insert(bytes.end(), p, p+n); }
  void blob(const std::vector<unsigned char> &v) { u32(v.size()); raw(v.data(),v.size()); }
  void text(const std::string &v) { u32(v.size()); raw(reinterpret_cast<const unsigned char *>(v.data()),v.size()); }
};
struct Reader {
  const std::vector<unsigned char> &bytes; size_t pos = 0;
  explicit Reader(const std::vector<unsigned char> &source):bytes(source){}
  void need(size_t n) const { if (n > bytes.size()-pos) throw std::runtime_error("Truncated saved state"); }
  unsigned u8() { need(1); return bytes[pos++]; }
  bool bit() { unsigned v=u8(); if(v>1) throw std::runtime_error("Invalid saved boolean"); return v; }
  unsigned u16() { unsigned lo=u8(); return lo | (u8()<<8); }
  std::uint32_t u32() { std::uint32_t v=0; for(unsigned i=0;i<4;++i) v |= std::uint32_t(u8())<<(i*8); return v; }
  std::uint64_t u64() { std::uint64_t v=0; for(unsigned i=0;i<8;++i) v |= std::uint64_t(u8())<<(i*8); return v; }
  std::vector<unsigned char> blob(size_t limit) {
    size_t n=u32(); if(n>limit) throw std::runtime_error("Saved field is too large"); need(n);
    std::vector<unsigned char> v(bytes.begin()+pos,bytes.begin()+pos+n); pos+=n; return v;
  }
  std::string text(size_t limit) { auto v=blob(limit); return std::string(v.begin(),v.end()); }
  short sample() { unsigned v=u16(); return static_cast<short>(v<32768 ? int(v) : int(v)-65536); }
};
inline void NormalizeROM(ROMSet &rom) {
  std::vector<std::uint64_t> hashes;
  for(const auto &page:rom.pages) hashes.push_back(ROMFingerprint(page.data(),page.size()));
  for(const auto &profile:ROMProfiles()) if(hashes==profile.fingerprints) {
    if(rom.model!=profile.model) throw std::runtime_error("Saved ROM does not match its model");
    rom.name=profile.name; rom.fastLoadCompatible=true;
    rom.fastLoadPage=rom.model==MachineModel::Spectrum48?0:rom.model==MachineModel::Plus3?3:1;
    return;
  }
  rom.name=std::string(HardwareFor(rom.model).name)+" (embedded ROM)";rom.fastLoadCompatible=false;rom.fastLoadPage=-1;
}
inline void WriteCore(Writer &w,const SpectrumMachine::State &s,unsigned version=3) {
  w.u64(s.frame); w.u32(s.tstates); w.u32(s.multiplier); w.u32(s.clockPhase);
#define REG(field) w.u16(s.registers.field)
  REG(AF);REG(BC);REG(DE);REG(HL);REG(IX);REG(IY);REG(SP);REG(PC);REG(IR);REG(XX);
  REG(altAF);REG(altBC);REG(altDE);REG(altHL);
#undef REG
  w.u8(s.registers.iff1A); w.u8(s.registers.iff1B); w.u8(s.registers.IM);
  w.u8(s.memory.paging); w.blob(s.memory.ram);
  const auto &u=s.ula;
  w.u8(u.blinkState);w.u8(u.earInput);w.u8(u.tapeMonitor);w.u32(u.borderColor);
  for(auto v:u.lineColors) w.u32(v);
  w.u32(u.frameTStates);w.u32(u.lineTStates);w.u32(u.line);w.u32(u.frameCount);
  for(auto v:u.keys) w.u8(v);
  w.u8(u.ioData);
  for(auto v:u.frameAudio) w.u16(static_cast<unsigned short>(v));
  for(auto v:u.pendingAudio) w.u16(static_cast<unsigned short>(v));
  w.u32(u.frameAudioCount);w.u32(u.audioPhase);w.u32(u.audioIndex);w.u64(u.audioSum);
  static_assert(sizeof(double)==8 && std::numeric_limits<double>::is_iec559,"IEEE double required");
  std::uint64_t dc; memcpy(&dc,&u.dcAverage,8); w.u64(dc);
  auto a=u.ay.CaptureState();
  w.u32(a.cpuHz);w.u32(a.chipHz);w.u64(a.phase);
  for(auto v:a.regs) w.u8(v);
  w.u32(a.selected);
  for(auto v:a.toneCount) w.u32(v);
  for(auto v:a.toneHigh) w.u8(v);
  w.u32(a.noiseCount);w.u32(a.envelopeCount);w.u32(a.noise);
  w.u8(a.noiseDivider);w.u8(a.holding);w.u8(a.envelopeLevel);w.u8(a.envelopeDirection==1);
  auto d=s.deck;
  w.u32(d.current);w.u32(d.pulse);w.u32(d.cpuClock);w.u32(d.remaining);
  w.u64(d.clockPhase);w.u64(d.elapsed);w.u8(d.level);w.u8(static_cast<unsigned>(d.state));
  w.u32(s.tapePos);w.u32(s.audioOffset);w.u32(s.audioAvailable);w.u8(s.fastLoad);
  if(version<2)return;
  w.u32(d.choices.size());for(auto choice:d.choices)w.u32(choice);
  w.u32(s.levels?s.levels->size():0);
  if(s.levels)for(const auto &level:*s.levels){w.u32(level.first);w.blob(level.second);}
  w.u8(s.snapshotHalted);
  w.u8(s.memory.pagingPlus3);
  const auto &f=s.floppy;
  for(const auto &drive:f.drives){w.u8(bool(drive.image));w.text(drive.path);w.u8(drive.writeProtected);w.u8(drive.dirty);if(drive.image)w.blob(drive.image->Encode());}
  for(auto x:f.cylinder)w.u32(x);for(auto x:f.target)w.u32(x);for(auto x:f.seek)w.u32(x);for(auto x:f.rotation)w.u32(x);
  w.u32(f.interrupts.size());for(auto x:f.interrupts)w.u32(x);
  w.blob(f.command);w.blob(f.buffer);w.blob(f.result);w.u32(f.position);w.u32(f.sector);
  w.u32(f.ready);w.u32(f.unit);w.u32(f.head);w.u32(f.endSector);w.u32(f.ncode);w.u32(f.st0);w.u32(f.st1);w.u32(f.st2);w.u32(f.lastR);w.u32(f.formatCount);
  w.u8(f.motor);w.u8(f.deleted);w.u8(f.multi);w.u8(static_cast<unsigned>(f.phase));
  if(version>=3)w.blob(u.screenRGB);
}
inline SpectrumMachine::State ReadCore(Reader &r,MachineModel model,unsigned version=3) {
  SpectrumMachine::State s{}; s.model=model; s.ula.timing=HardwareFor(model).timing;
  s.frame=r.u64();s.tstates=r.u32();s.multiplier=r.u32();s.clockPhase=r.u32();
#define REG(field) s.registers.field=r.u16()
  REG(AF);REG(BC);REG(DE);REG(HL);REG(IX);REG(IY);REG(SP);REG(PC);REG(IR);REG(XX);
  REG(altAF);REG(altBC);REG(altDE);REG(altHL);
#undef REG
  s.registers.iff1A=r.bit();s.registers.iff1B=r.bit();s.registers.IM=r.u8();
  s.memory.paging=r.u8();s.memory.ram=r.blob(128*1024);
  auto &u=s.ula;
  u.blinkState=r.bit();u.earInput=r.bit();u.tapeMonitor=r.bit();u.borderColor=r.u32();
  for(auto &v:u.lineColors) v=r.u32();
  u.frameTStates=r.u32();u.lineTStates=r.u32();u.line=r.u32();u.frameCount=r.u32();
  for(auto &v:u.keys) v=r.u8();
  u.ioData=r.u8();
  for(auto &v:u.frameAudio) v=r.sample();
  for(auto &v:u.pendingAudio) v=r.sample();
  u.frameAudioCount=r.u32();u.audioPhase=r.u32();u.audioIndex=r.u32();
  auto sum=r.u64(); if(sum>std::uint64_t(30720)*u.audioPhase) throw std::runtime_error("Invalid saved audio sum");
  u.audioSum=static_cast<std::int64_t>(sum);
  auto dc=r.u64();memcpy(&u.dcAverage,&dc,8);
  AY8912::State a{};
  a.cpuHz=r.u32();a.chipHz=r.u32();a.phase=r.u64();
  for(auto &v:a.regs) v=r.u8();
  a.selected=r.u32();
  for(auto &v:a.toneCount) v=r.u32();
  for(auto &v:a.toneHigh) v=r.bit();
  a.noiseCount=r.u32();a.envelopeCount=r.u32();a.noise=r.u32();
  a.noiseDivider=r.bit();a.holding=r.bit();a.envelopeLevel=r.u8();a.envelopeDirection=r.bit()?1:-1;
  const auto &hardware=HardwareFor(model);
  if(a.cpuHz!=hardware.timing.cpuClockHz || a.chipHz!=hardware.devices.ayClockHz || !u.ay.RestoreState(a))
    throw std::runtime_error("Invalid saved AY state");
  auto &d=s.deck;
  d.current=r.u32();d.pulse=r.u32();d.cpuClock=r.u32();d.remaining=r.u32();
  d.clockPhase=r.u64();d.elapsed=r.u64();d.level=r.bit();auto transport=r.u8();
  if(transport>static_cast<unsigned>(TapeDeck::State::End)) throw std::runtime_error("Invalid saved transport");
  d.state=static_cast<TapeDeck::State>(transport);
  s.tapePos=r.u32();s.audioOffset=r.u32();s.audioAvailable=r.u32();s.fastLoad=r.bit();
  if(version>=2) {
    unsigned choices=r.u32();if(choices>4096)throw std::runtime_error("Too many saved TZX selections");
    for(unsigned n=0;n<choices;++n)d.choices.push_back(r.u32());
    unsigned count=r.u32();if(count>256)throw std::runtime_error("Too many SLT levels");
    auto levels=std::make_shared<std::map<unsigned,std::vector<unsigned char>>>();
    for(unsigned n=0;n<count;++n){unsigned id=r.u32();auto data=r.blob(65536);if(id>255 || data.empty() || levels->count(id))throw std::runtime_error("Invalid saved SLT level");levels->emplace(id,std::move(data));}
    s.levels=levels;s.snapshotHalted=r.bit();
    s.memory.pagingPlus3=r.u8();auto &f=s.floppy;
    for(auto &drive:f.drives){bool inserted=r.bit();drive.path=r.text(4096);drive.writeProtected=r.bit();drive.dirty=r.bit();if(inserted)drive.image=std::make_shared<const Disks::Image>(Disks::Image::Parse(r.blob(16*1024*1024)));}
    for(auto &x:f.cylinder)x=r.u32();for(auto &x:f.target)x=r.u32();for(auto &x:f.seek)x=r.u32();for(auto &x:f.rotation)x=r.u32();
    unsigned irq=r.u32();if(irq>32)throw std::runtime_error("Too many disk interrupts");for(unsigned n=0;n<irq;++n)f.interrupts.push_back(r.u32());
    f.command=r.blob(9);f.buffer=r.blob(65536);f.result=r.blob(7);f.position=r.u32();f.sector=r.u32();
    f.ready=r.u32();f.unit=r.u32();f.head=r.u32();f.endSector=r.u32();f.ncode=r.u32();f.st0=r.u32();f.st1=r.u32();f.st2=r.u32();f.lastR=r.u32();f.formatCount=r.u32();
    f.motor=r.bit();f.deleted=r.bit();f.multi=r.bit();unsigned phase=r.u8();if(phase>4)throw std::runtime_error("Invalid FDC phase");f.phase=static_cast<Disks::Controller::Phase>(phase);
    Disks::Controller controller;if(!controller.RestoreState(f))throw std::runtime_error("Invalid saved disk controller");
    if(model!=MachineModel::Plus3 && (s.memory.pagingPlus3 || f.drives[0].image || f.drives[1].image))throw std::runtime_error("Disks require Spectrum +3");
  }
  if(version>=3) {
    u.screenRGB=r.blob(320*240*3);
    if(!u.screenRGB.empty() && u.screenRGB.size()!=320*240*3)throw std::runtime_error("Invalid saved raster");
  }
  const auto &t=u.timing;
  if(s.registers.IM>2 || s.tstates>1024 || (s.multiplier!=1 && s.multiplier!=2 && s.multiplier!=4) ||
      s.clockPhase>=s.multiplier || s.memory.ram.size()!=hardware.memory.RAMBytes() ||
      (model==MachineModel::Spectrum48 && s.memory.paging) || u.line>=t.linesPerFrame ||
      u.lineTStates>=t.tstatesPerLine || u.frameTStates!=u.line*t.tstatesPerLine+u.lineTStates || u.frameCount>=16 ||
      u.frameAudioCount>SAMPLES_PER_FRAME || u.audioIndex>SAMPLES_PER_FRAME || u.audioPhase>=t.cpuClockHz ||
      !std::isfinite(u.dcAverage) || u.dcAverage<0 || u.dcAverage>30720 ||
      s.audioOffset>SAMPLES_PER_FRAME || s.audioAvailable>SAMPLES_PER_FRAME-s.audioOffset ||
      (s.audioAvailable && s.audioOffset+s.audioAvailable>u.frameAudioCount) || d.cpuClock!=t.cpuClockHz)
    throw std::runtime_error("Invalid saved CPU/video/audio state");
  auto futureSamples=(std::uint64_t(t.TStatesPerFrame()-u.frameTStates)*AUDIO_SAMPLE_RATE+u.audioPhase)/t.cpuClockHz;
  if(u.audioIndex+futureSamples>SAMPLES_PER_FRAME) throw std::runtime_error("Invalid saved sample phase");
  for(auto key:u.keys) if(key & ~31u) throw std::runtime_error("Invalid saved keyboard");
  return s;
}
inline std::vector<unsigned char> Encode(const Record &record) {
  Writer payload;
  payload.u64(record.created);payload.text(record.label);payload.text(record.tapePath);
  payload.u8(static_cast<unsigned>(record.rom.model));payload.text(record.rom.source);
  payload.u32(record.rom.pages.size());for(const auto &page:record.rom.pages) payload.blob(page);
  payload.blob(record.tape);payload.blob(record.thumbnail);WriteCore(payload,record.core);
  if(payload.bytes.size()>MaxFile) throw std::runtime_error("Saved state is too large");
  Writer file;
  const unsigned char magic[8]={'G','O','M','A','S','S','T',0};file.raw(magic,8);
  file.u32(3);file.u32(payload.bytes.size());file.u64(ROMFingerprint(payload.bytes.data(),payload.bytes.size()));
  file.raw(payload.bytes.data(),payload.bytes.size());return file.bytes;
}
inline Record Decode(const std::vector<unsigned char> &file) {
  Reader header{file}; const unsigned char magic[8]={'G','O','M','A','S','S','T',0};
  for(auto v:magic) if(header.u8()!=v) throw std::runtime_error("Not a Gomas saved state");
  unsigned version=header.u32();if(version!=1 && version!=2 && version!=3) throw std::runtime_error("Unsupported saved-state version");
  auto size=header.u32();auto hash=header.u64();
  if(size>MaxFile || size!=file.size()-header.pos || ROMFingerprint(file.data()+header.pos,size)!=hash)
    throw std::runtime_error("Saved-state checksum or size is invalid");
  std::vector<unsigned char> payload(file.begin()+header.pos,file.end());Reader r{payload}; Record record;
  record.created=r.u64();record.label=r.text(128);record.tapePath=r.text(4096);
  auto model=r.u8();if(model>static_cast<unsigned>(MachineModel::Plus3)) throw std::runtime_error("Unknown saved model");
  record.rom.model=static_cast<MachineModel>(model);record.rom.source=r.text(4096);
  auto pages=r.u32();if(!pages || pages>4) throw std::runtime_error("Invalid saved ROM pages");
  for(unsigned i=0;i<pages;++i) record.rom.pages.push_back(r.blob(SPECTRUM_BANK_BYTES));
  std::string error;
  if(!ValidateROMHardware(record.rom,error) || !SupportsHardware(record.rom.model))
    throw std::runtime_error(error.empty()?"Saved model is not supported":error);
  NormalizeROM(record.rom);
  record.tape=r.blob(MaxTape);if(!ValidateTape(record.tape)) throw std::runtime_error("Invalid embedded tape");
  record.thumbnail=r.blob(ThumbWidth*ThumbHeight*3);
  if(record.thumbnail.size()!=ThumbWidth*ThumbHeight*3) throw std::runtime_error("Invalid saved thumbnail");
  record.core=ReadCore(r,record.rom.model,version);
  if(r.pos!=payload.size() || (record.core.fastLoad && !record.rom.fastLoadCompatible))
    throw std::runtime_error("Invalid saved-state contents");
  TapeDeck deck;deck.Mount(record.tape);
  if(!deck.RestoreState(record.core.deck) || !deck.ValidOffset(record.core.tapePos) ||
      (record.core.fastLoad && !deck.FastLoadCompatible())) throw std::runtime_error("Invalid saved tape position");
  return record;
}
inline Record Capture(const SpectrumMachine &machine) {
  Record r;r.created=std::time(nullptr);r.rom=machine.systemROM;r.tape=machine.tape;r.tapePath=machine.tapePath;
  if(r.tape.size()>MaxTape) throw std::runtime_error("Tape is too large to embed in a saved state");
  r.label=r.tapePath.empty()?r.rom.name:r.tapePath.substr(r.tapePath.find_last_of("/\\")+1);
  r.label=r.label.substr(0,128);r.core=machine.CaptureState();
  auto rgb=machine.ula.CaptureRGB();r.thumbnail.resize(ThumbWidth*ThumbHeight*3);
  for(unsigned y=0;y<ThumbHeight;++y) for(unsigned x=0;x<ThumbWidth;++x) for(unsigned c=0;c<3;++c) {
    unsigned sum=0;for(unsigned dy=0;dy<2;++dy) for(unsigned dx=0;dx<2;++dx) sum+=rgb[((y*2+dy)*320+x*2+dx)*3+c];
    r.thumbnail[(y*ThumbWidth+x)*3+c]=sum/4;
  }
  return r;
}
inline std::string Directory() {
  if(const char *overridePath=std::getenv("GOMAS_SAVE_DIR")) return overridePath;
  std::unique_ptr<ALLEGRO_PATH,decltype(&al_destroy_path)> path(al_get_standard_path(ALLEGRO_USER_DATA_PATH),al_destroy_path);
  if(!path) return "";al_append_path_component(path.get(),"saves");
  return al_path_cstr(path.get(),ALLEGRO_NATIVE_PATH_SEP);
}
class Store {
public:
  explicit Store(std::string directory):directory(std::move(directory)){}
  std::string Path(unsigned slot) const {
    if(directory.empty() || slot>=SlotCount) return "";
    return directory+(directory.back()=='/'?"":"/")+"slot-"+std::to_string(slot)+".gss";
  }
  bool Exists(unsigned slot) const { auto path=Path(slot);return !path.empty() && al_filename_exists(path.c_str()); }
  bool Read(unsigned slot,Record &out,std::string &error) const {
    try {
      auto path=Path(slot);if(path.empty()) throw std::runtime_error("Save directory or slot is invalid");
      std::ifstream input(path,std::ios::binary|std::ios::ate);
      if(!input) throw std::runtime_error("This slot has no saved game");
      auto size=input.tellg();if(size<0 || size>std::streamoff(MaxFile+24)) throw std::runtime_error("Saved file is too large");
      std::vector<unsigned char> bytes(static_cast<size_t>(size));input.seekg(0);
      if(!bytes.empty()) input.read(reinterpret_cast<char *>(bytes.data()),bytes.size());
      if(!input) throw std::runtime_error("Cannot read saved state");
      auto candidate=Decode(bytes);out=std::move(candidate);error.clear();return true;
    } catch(const std::exception &e) {error=e.what();return false;}
  }
  bool Save(unsigned slot,const SpectrumMachine &machine,std::string &error) const {
    std::string temporary;
    try {
      auto path=Path(slot);if(path.empty()) throw std::runtime_error("Save directory or slot is invalid");
      auto bytes=Encode(Capture(machine));Decode(bytes); // Validate before replacing any existing slot.
      if(!al_make_directory(directory.c_str())) throw std::runtime_error("Cannot create save directory");
      std::string pattern=path+".XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
      int fd=Platform::Temporary(name.data());if(fd<0) throw std::runtime_error("Cannot create temporary saved state");
      temporary=name.data();FILE *file=Platform::File(fd,"wb");
      if(!file) {Platform::Close(fd);throw std::runtime_error("Cannot write saved state");}
      bool ok=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();
      ok=(std::fflush(file)==0)&&ok;ok=(Platform::Sync(Platform::Descriptor(file))==0)&&ok;ok=(std::fclose(file)==0)&&ok;
      if(!ok || Platform::Replace(temporary,path)!=0) throw std::runtime_error("Cannot replace saved state");
      error.clear();return true;
    } catch(const std::exception &e) {if(!temporary.empty()) std::remove(temporary.c_str());error=e.what();return false;}
  }
  bool Load(unsigned slot,SpectrumMachine &machine,std::string &error) const {
    Record saved;if(!Read(slot,saved,error)) return false;
    return machine.ImportSession(std::move(saved.rom),std::move(saved.tape),std::move(saved.tapePath),saved.core,error);
  }
private:
  std::string directory;
};
}
#endif
