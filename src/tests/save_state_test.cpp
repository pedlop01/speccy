#include <cassert>
#include <cstdio>
#include <sys/stat.h>
#include "../save_state.h"

static std::vector<unsigned char> Core(const SpectrumMachine &m) {
  SaveStates::Writer w; SaveStates::WriteCore(w,m.CaptureState());return w.bytes;
}
static void Reject(const std::vector<unsigned char> &bytes) {
  bool rejected=false;try {SaveStates::Decode(bytes);}catch(const std::exception &) {rejected=true;}assert(rejected);
}
int main() {
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  char directory[]="/tmp/gomas-saves-XXXXXX";assert(mkdtemp(directory));
  SaveStates::Store store(directory);std::string error;
  for(const char *rom:{"48.rom","128-0.rom","plus2-0.rom"}) {
    SpectrumMachine source;assert(source.LoadROM(FindROM(rom),error));
    assert(source.Open("cobra.tap",error));source.SetFastLoad(false);assert(source.PlayTape());
    source.SetCPUMultiplier(2);source.EnableRewind();
    // Run a predictable RAM loop, retaining live tape pulses and partial audio.
    source.cpu.regs.PC=0x8000;source.cpu.regs.iff1A=source.cpu.regs.iff1B=0;
    const unsigned char code[]={0x03,0x3e,0x10,0xd3,0xfe,0xc3,0,0x80};
    for(unsigned i=0;i<sizeof(code);++i)source.bus.Write(0x8000+i,code[i]);
    for(unsigned i=0;i<6912;++i)source.bus.Write(0x4000+i,(i*13)>>3);
    if(source.Configuration().model!=MachineModel::Spectrum48) {
      source.io.Write(0x7ffd,0x0b); // bank 3 / shadow screen.
      source.io.Write(0xfffd,0);source.io.Write(0xbffd,39);
      source.io.Write(0xfffd,6);source.io.Write(0xbffd,7);
      source.io.Write(0xfffd,8);source.io.Write(0xbffd,16);
      source.io.Write(0xfffd,11);source.io.Write(0xbffd,17);
      source.io.Write(0xfffd,13);source.io.Write(0xbffd,10);
    }
    std::vector<short> warm(13007);source.ReadAudio(warm.data(),warm.size());
    assert(store.Save(0,source,error));assert(store.Save(7,source,error));
    auto before=Core(source);auto image=source.ula.CaptureRGB();
    SaveStates::Record record;assert(store.Read(7,record,error));
    assert(record.thumbnail.size()==160*120*3 && record.label=="cobra.tap");
    // Restoration does not need original media paths or an already loaded ROM.
    record.rom.source="/missing/rom";record.tapePath="/missing/cobra.tap";
    SpectrumMachine restored;restored.EnableRewind();
    assert(restored.ImportSession(record.rom,record.tape,record.tapePath,record.core,error));
    assert(Core(restored)==before);assert(restored.ula.CaptureRGB()==image);
    std::vector<short> a(57313),b(a.size());source.ReadAudio(a.data(),a.size());restored.ReadAudio(b.data(),b.size());
    assert(a==b && Core(source)==Core(restored));
    assert(store.Load(0,restored,error));assert(Core(restored)==before);
    assert(!restored.RewindOneSecond());assert(Core(restored)==before);
    auto bytes=SaveStates::Encode(record);
    for(size_t n:{size_t(0),size_t(12),size_t(24),bytes.size()-1})Reject(std::vector<unsigned char>(bytes.begin(),bytes.begin()+n));
    auto corrupt=bytes;corrupt.back()^=1;Reject(corrupt);
    corrupt=bytes;corrupt[8]=2;Reject(corrupt);
    auto invalid=record;invalid.core.multiplier=3;Reject(SaveStates::Encode(invalid));
    invalid=record;invalid.core.deck.remaining=0xffffffffu;Reject(SaveStates::Encode(invalid));
    invalid=record;invalid.core.ula.audioIndex=960;invalid.core.ula.frameTStates=0;invalid.core.ula.line=0;invalid.core.ula.lineTStates=0;
    Reject(SaveStates::Encode(invalid));
    // Invalid saves cannot replace a valid slot; failed loads leave the machine intact.
    SpectrumMachine empty;assert(!store.Save(0,empty,error));assert(store.Read(0,record,error));
    assert(!store.Load(11,restored,error));assert(Core(restored)==before);
    FILE *file=fopen(store.Path(8).c_str(),"wb");assert(file);fwrite(corrupt.data(),1,corrupt.size(),file);fclose(file);
    assert(!store.Load(8,restored,error));assert(Core(restored)==before);
    assert(!store.Save(12,source,error));assert(store.Path(12).empty());
  }
  // BASIC with no tape, plus an unknown 48K ROM carried entirely by the save.
  SpectrumMachine basic;assert(basic.LoadROM(FindROM("48.rom"),error));
  assert(store.Save(2,basic,error));SpectrumMachine fresh;assert(store.Load(2,fresh,error));
  assert(Core(basic)==Core(fresh) && fresh.tape.empty());
  auto custom=SaveStates::Capture(basic);custom.rom.pages[0][100]^=1;custom.core.fastLoad=false;
  custom=SaveStates::Decode(SaveStates::Encode(custom));
  assert(!custom.rom.fastLoadCompatible);
  assert(fresh.ImportSession(custom.rom,custom.tape,"",custom.core,error));
  assert(fresh.systemROM.pages==custom.rom.pages && !fresh.fastLoad);
  // A path that is a file cannot become a save directory.
  SaveStates::Store blocked(store.Path(0));SpectrumMachine valid;assert(valid.LoadROM(FindROM("48.rom"),error));
  assert(!blocked.Save(1,valid,error));
  for(unsigned i=0;i<12;++i)unlink(store.Path(i).c_str());assert(rmdir(directory)==0);
  puts("Saved states: 48K/128K/+2, embedded media, video, AY/audio/tape replay, corruption and transactional failures passed");
}
