#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include "../save_state.h"
using Bytes=std::vector<unsigned char>;
static void D(Bytes &v,unsigned n){v.push_back(n);v.push_back(n>>8);v.push_back(n>>16);v.push_back(n>>24);}
static void W(Bytes &v,unsigned n){v.push_back(n);v.push_back(n>>8);}
static void Put(Bytes &v,size_t p,unsigned n){v[p]=n;v[p+1]=n>>8;}
static void File(const std::string &p,const Bytes &v){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char *>(v.data()),v.size());assert(f.good());}
static Bytes Read(const std::string &p){std::ifstream f(p,std::ios::binary);assert(f);return {(std::istreambuf_iterator<char>(f)),{}};}
static Bytes Core(const SpectrumMachine &m){SaveStates::Writer w;SaveStates::WriteCore(w,m.CaptureState());return w.bytes;}
static Bytes MemoryBytes(unsigned banks) {
  Bytes v(banks*16384);for(unsigned b=0;b<banks;++b)std::fill(v.begin()+b*16384,v.begin()+(b+1)*16384,0x40+b);
  size_t pc=(banks==3?1:2)*16384;v[pc]=0xc3;v[pc+1]=0;v[pc+2]=0x80;return v;
}
static Bytes Pack(const Bytes &v) {
  Bytes out;for(size_t p=0;p<v.size();) {
    unsigned n=1;while(p+n<v.size() && v[p+n]==v[p] && n<255)++n;
    if(n>=5 || v[p]==0xed){out.push_back(0xed);out.push_back(0xed);out.push_back(n);out.push_back(v[p]);}
    else out.insert(out.end(),v.begin()+p,v.begin()+p+n);p+=n;
  }return out;
}
static Bytes Z80File(unsigned version,MachineModel model,bool compressed) {
  Bytes v(30);v[0]=0x12;v[1]=0x45;Put(v,2,0x2345);Put(v,4,0x3456);Put(v,6,version==1?0x8000:0);Put(v,8,0xf000);
  v[10]=0x3f;v[11]=0x12;v[12]=7|(compressed?32:0);Put(v,13,0x4567);Put(v,15,0x5678);Put(v,17,0x6789);Put(v,19,0x789a);
  v[21]=0x89;v[22]=0xab;Put(v,23,0x9876);Put(v,25,0x8765);v[29]=1;
  bool flat=model==MachineModel::Spectrum48;auto ram=MemoryBytes(flat?3:8);
  if(version==1){auto data=compressed?Pack(ram):ram;v.insert(v.end(),data.begin(),data.end());if(compressed)v.insert(v.end(),{0,0xed,0xed,0});return v;}
  unsigned length=version==2?23:54;v.resize(32+length);Put(v,30,length);Put(v,32,0x8000);
  v[34]=flat?0:model==MachineModel::Plus2?12:version==2?3:4;v[35]=flat?0:0x3f;
  v[38]=8;v[39]=27;v[39+8]=15;
  if(version==3){Put(v,55,100);v[57]=1;}
  for(unsigned b=0;b<(flat?3u:8u);++b) {
    Bytes bank(ram.begin()+b*16384,ram.begin()+(b+1)*16384);auto data=compressed?Pack(bank):bank;
    W(v,compressed?data.size():65535);v.push_back(flat?(b==0?8:b==1?4:5):b+3);v.insert(v.end(),data.begin(),data.end());
  }return v;
}
static Bytes SNAFile(int bank=-1) {
  Bytes v(27);v[0]=0x3f;Put(v,1,0x789a);Put(v,3,0x6789);Put(v,5,0x5678);Put(v,7,0x89ab);
  Put(v,9,0x3456);Put(v,11,0x4567);Put(v,13,0x2345);Put(v,15,0x9876);Put(v,17,0x8765);
  v[20]=0x92;Put(v,21,0x1245);Put(v,23,0xf000);v[25]=1;v[26]=3;
  if(bank<0){auto ram=MemoryBytes(3);Put(ram,0xb000,0x8000);v.insert(v.end(),ram.begin(),ram.end());}
  else {
    auto ram=MemoryBytes(8);for(unsigned b:{5u,2u,unsigned(bank)})v.insert(v.end(),ram.begin()+b*16384,ram.begin()+(b+1)*16384);
    W(v,0x8000);v.push_back(0x38|bank);v.push_back(0);
    for(unsigned b=0;b<8;++b)if(b!=5 && b!=2 && b!=unsigned(bank))v.insert(v.end(),ram.begin()+b*16384,ram.begin()+(b+1)*16384);
  }return v;
}
static void Zip(const std::string &path,const std::string &name,const Bytes &bytes) {
  auto *a=archive_write_new();assert(archive_write_set_format_zip(a)==ARCHIVE_OK);assert(archive_write_open_filename(a,path.c_str())==ARCHIVE_OK);
  auto *e=archive_entry_new();archive_entry_set_pathname(e,name.c_str());archive_entry_set_filetype(e,AE_IFREG);archive_entry_set_perm(e,0644);archive_entry_set_size(e,bytes.size());
  assert(archive_write_header(a,e)==ARCHIVE_OK);assert(archive_write_data(a,bytes.data(),bytes.size())==static_cast<la_ssize_t>(bytes.size()));
  archive_entry_free(e);assert(archive_write_close(a)==ARCHIVE_OK);archive_write_free(a);
}
static Bytes TZXHeader(){return {'Z','X','T','a','p','e','!',26,1,20};}
static void Tone(Bytes &v,unsigned ticks,unsigned count){v.push_back(0x12);W(v,ticks);W(v,count);}
static void Standard(Bytes &v,const Bytes &tap){for(size_t p=0;p<tap.size();){size_t n=tap[p]|(tap[p+1]<<8);v.push_back(0x10);W(v,1000);v.insert(v.end(),tap.begin()+p,tap.begin()+p+n+2);p+=n+2;}}
int main() {
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  char dir[]="/tmp/gomas-formats-XXXXXX";assert(mkdtemp(dir));std::string root=std::string(dir)+"/",error;
  SpectrumMachine machine;assert(machine.LoadROM(FindROM("48.rom"),error));machine.EnableRewind();
  SaveStates::Store saves(dir);
  auto snapshot=[&](const std::string &extension,const Bytes &bytes,MachineModel model) {
    File(root+"game"+extension,bytes);assert(machine.Open(root+"game"+extension,error));assert(machine.Configuration().model==model);
    assert(machine.tape.empty() && machine.cpu.regs.PC==0x8000 && machine.cpu.regs.AF==0x1245 && machine.cpu.regs.BC==0x2345);
    assert(machine.cpu.regs.IR==0x3f92 && machine.cpu.regs.altAF==0x89ab && machine.cpu.regs.IM==1);
    assert(machine.bus.Read(0x8000)==0xc3 && machine.bus.Read(0x4000)==(model==MachineModel::Spectrum48?0x40:0x45));
    auto before=Core(machine);assert(saves.Save(0,machine,error));SpectrumMachine restored;assert(saves.Load(0,restored,error));
    assert(before==Core(restored) && machine.ula.CaptureRGB()==restored.ula.CaptureRGB());
    std::vector<short>a(4000),b(a.size());machine.ReadAudio(a.data(),a.size());restored.ReadAudio(b.data(),b.size());assert(a==b && Core(machine)==Core(restored));
    Zip(root+"game.zip","nested/GAME"+extension,bytes);assert(machine.Open(root+"game.zip",error));assert(Core(machine)==before);
    auto reject=[&](Bytes bad){File(root+"bad"+extension,bad);assert(!machine.Open(root+"bad"+extension,error));assert(!error.empty() && Core(machine)==before);};
    reject(Bytes(bytes.begin(),bytes.begin()+26));reject(Bytes{});unlink((root+"game"+extension).c_str());unlink((root+"bad"+extension).c_str());
  };
  snapshot(".sna",SNAFile(),MachineModel::Spectrum48);assert(machine.cpu.regs.SP==0xf002);
  for(int bank:{0,2,5,7}) {snapshot(".sna",SNAFile(bank),MachineModel::Spectrum128);assert(machine.memory.PagingLatch()==(0x38|bank));}
  for(unsigned version:{1,2,3})for(bool compressed:{false,true}) {
    snapshot(".z80",Z80File(version,MachineModel::Spectrum48,compressed),MachineModel::Spectrum48);
    if(version>1)for(auto model:{MachineModel::Spectrum128,MachineModel::Plus2}) {
      snapshot(".z80",Z80File(version,model,compressed),model);assert(machine.ula.ay.CaptureState().regs[8]==15);
    }
  }
  // Strict rejection of corrupt RLE, missing/duplicate pages and unsupported hardware.
  auto before=Core(machine);auto bad=Z80File(2,MachineModel::Spectrum128,true);bad[34]=10;
  File(root+"bad.z80",bad);assert(!machine.Open(root+"bad.z80",error) && Core(machine)==before);
  bool rejected=false;try{Snapshots::Expand({0xed,0xed,0,42},16384);}catch(...){rejected=true;}assert(rejected);
  bad=SNAFile(0);bad[49182]=1;File(root+"bad.sna",bad);assert(!machine.Open(root+"bad.sna",error) && Core(machine)==before);
  auto invalidZ80=[&](const Bytes &bytes){File(root+"bad.z80",bytes);assert(!machine.Open(root+"bad.z80",error) && Core(machine)==before);};
  bad=Z80File(2,MachineModel::Spectrum128,true);
  size_t firstBlock=55,firstSize=Snapshots::Word(bad,firstBlock)+3;
  auto duplicate=bad;duplicate.insert(duplicate.end(),bad.begin()+firstBlock,bad.begin()+firstBlock+firstSize);invalidZ80(duplicate);
  auto missing=bad;missing.resize(missing.size()-firstSize);invalidZ80(missing);
  auto overflow=bad;overflow[firstBlock+3]=0xed;overflow[firstBlock+4]=0xed;overflow[firstBlock+5]=0;invalidZ80(overflow);
  // Exercise truncated boundaries of all new readers without changing live state.
  for(const auto &fixture:{SNAFile(),Z80File(1,MachineModel::Spectrum48,true),Z80File(3,MachineModel::Spectrum128,true)}) {
    for(size_t n=0;n<fixture.size();n+=std::max<size_t>(1,fixture.size()/47)) {
      Bytes truncated(fixture.begin(),fixture.begin()+n);
      try {if(fixture.size()==49179)Snapshots::SNA(truncated);else Snapshots::Z80(truncated);}catch(const std::exception &){}
    }
  }
  // Exact pulse intervals, loops, stop/resume and direct EAR levels.
  auto tzx=TZXHeader();tzx.push_back(0x24);W(tzx,2);Tone(tzx,100,2);tzx.push_back(0x25);tzx.push_back(0x20);W(tzx,0);Tone(tzx,200,2);
  File(root+"game.tzx",tzx);assert(machine.Open(root+"game.tzx",error));assert(!machine.fastLoad && !machine.CanFastLoad());
  assert(machine.deck.Blocks().size()==4 && machine.PlayTape());assert(machine.deck.UntilEdge()==102); // 128K clock conversion.
  machine.deck.Advance(406);assert(machine.deck.CurrentBlock()==2 && machine.deck.GetState()==TapeDeck::State::Stopped);
  assert(machine.PlayTape() && machine.deck.CurrentBlock()==3);machine.deck.Advance(406);assert(machine.deck.GetState()==TapeDeck::State::End);
  tzx=TZXHeader();tzx.insert(tzx.end(),{0x15,10,0,0,0,8,1,0,0,0xf0});
  TapeDeck deck;assert(deck.Mount(tzx) && deck.Play() && deck.Level() && deck.UntilEdge()==40);deck.Advance(40);assert(!deck.Level() && deck.UntilEdge()==40);
  // CSW RLE/Z-RLE and generalized alphabets preserve pulse timing.
  for(bool compressed:{false,true}) {
    Bytes encoded={10,20};
    if(compressed){Bytes packed(100);uLongf size=packed.size();assert(compress(packed.data(),&size,encoded.data(),encoded.size())==Z_OK);packed.resize(size);encoded=packed;}
    auto csw=TZXHeader();csw.push_back(0x18);D(csw,10+encoded.size());W(csw,0);csw.insert(csw.end(),{0x20,0xa1,0x07});csw.push_back(compressed?2:1);D(csw,2);
    csw.insert(csw.end(),encoded.begin(),encoded.end());assert(deck.Mount(csw) && deck.Play() && deck.UntilEdge()==70);deck.Advance(70);assert(deck.UntilEdge()==140);
  }
  auto longPulse=TZXHeader();longPulse.push_back(0x18);D(longPulse,15);W(longPulse,0);
  longPulse.insert(longPulse.end(),{0xe0,0x67,0x35,1});D(longPulse,1);longPulse.insert(longPulse.end(),{0,255,255,255,255});
  assert(deck.Mount(longPulse));deck.SetClock(3546900);assert(deck.Play() && deck.UntilEdge()==std::numeric_limits<unsigned>::max());
  deck.SetClock(3500000);
  auto general=TZXHeader();general.push_back(0x19);D(general,21);W(general,0);D(general,0);general.insert(general.end(),{0,0});D(general,2);general.insert(general.end(),{1,2});
  general.push_back(0);W(general,50);general.push_back(0);W(general,100);general.push_back(0x40);
  assert(deck.Mount(general) && deck.Play() && deck.UntilEdge()==50);deck.Advance(50);assert(deck.UntilEdge()==100);
  // TZX standard blocks share the ROM fast trap; turbo blocks use real pulses.
  auto cobra=Read("cobra.tap");tzx=TZXHeader();Standard(tzx,cobra);File(root+"game.tzx",tzx);
  Zip(root+"game.zip","COBRA.TZX",tzx);assert(machine.LoadROM(FindROM("48.rom"),error));assert(machine.Open(root+"game.zip",error));
  assert(machine.deck.FastLoadCompatible());machine.SetFastLoad(true);assert(machine.fastLoad);
  SpectrumMachine tap;assert(tap.LoadROM(FindROM("48.rom"),error));assert(tap.Open("cobra.tap",error));
  for(size_t n=0;n<tap.deck.Blocks().size();++n) {
    machine.CueTape(n);tap.CueTape(n);
    for(auto *m:{&machine,&tap}){m->cpu.regs.PC=0x056b;m->cpu.regs.altAF=0xff01;m->cpu.regs.IX=0x8000;m->cpu.regs.DE=m->deck.Blocks()[n].length-2;assert(m->TryFastLoad());}
    assert(machine.memory.CaptureState().ram==tap.memory.CaptureState().ram);
  }
  machine.SetFastLoad(false);machine.CueTape(0);assert(machine.PlayTape());std::vector<short> audio(11003);machine.ReadAudio(audio.data(),audio.size());
  before=Core(machine);assert(saves.Save(0,machine,error));SpectrumMachine restored;assert(saves.Load(0,restored,error));assert(Core(restored)==before);
  std::vector<short> a(4000),b(a.size());machine.ReadAudio(a.data(),a.size());restored.ReadAudio(b.data(),b.size());assert(a==b && Core(machine)==Core(restored));
  bad=tzx;bad.push_back(0x18);File(root+"bad.tzx",bad);before=Core(machine);assert(!machine.Open(root+"bad.tzx",error) && Core(machine)==before);
  for(const char *f:{"game.zip","game.tzx","bad.tzx","bad.sna","bad.z80","slot-0.gss"})unlink((root+f).c_str());assert(rmdir(dir)==0);
  puts("Formats: SNA48/128, Z80 v1/v2/v3 raw/RLE/48/128/+2, ZIP, TZX pulses/loops/stops/direct/fast load, saved sessions and transactional failures passed");
  return 0;
}
