#define main FormatsFixtureMain
#include "formats_test.cpp"
#undef main

static Bytes Deflate(const Bytes &v){Bytes out(compressBound(v.size()));uLongf n=out.size();assert(compress(out.data(),&n,v.data(),v.size())==Z_OK);out.resize(n);return out;}
static void Chunk(Bytes &out,const char *tag,const Bytes &body){out.insert(out.end(),tag,tag+4);D(out,body.size());out.insert(out.end(),body.begin(),body.end());}
static Bytes SZXFile(MachineModel model,bool compressed) {
  auto image=Snapshots::Z80(Z80File(3,model,compressed));Bytes out={'Z','X','S','T',1,5,static_cast<unsigned char>(model==MachineModel::Spectrum48?1:model==MachineModel::Plus2?3:2),0};
  auto &z=image.registers;Bytes cpu;for(unsigned value:{z.AF,z.BC,z.DE,z.HL,z.altAF,z.altBC,z.altDE,z.altHL,z.IX,z.IY,z.SP,z.PC})W(cpu,value);
  cpu.insert(cpu.end(),{z.I,z.R,z.iff1A,z.iff1B,z.IM});D(cpu,0);cpu.insert(cpu.end(),{0,0});W(cpu,0);Chunk(out,"Z80R",cpu);
  Bytes ula(8);ula[0]=3;ula[1]=image.paging;ula[3]=3;Chunk(out,"SPCR",ula);
  for(unsigned bank=0;bank<(model==MachineModel::Spectrum48?3u:8u);++bank){Bytes page(image.ram.begin()+bank*16384,image.ram.begin()+(bank+1)*16384);if(compressed)page=Deflate(page);
    Bytes body;W(body,compressed?1:0);body.push_back(model==MachineModel::Spectrum48?(bank==0?5:bank==1?2:0):bank);body.insert(body.end(),page.begin(),page.end());Chunk(out,"RAMP",body);}
  if(model!=MachineModel::Spectrum48){Bytes ay(18);ay[1]=8;ay[10]=15;Chunk(out,"AY\0\0",ay);}
  return out;
}
static Disks::Image DiskFixture() {
  Disks::Image image;image.cylinders=40;image.heads=1;image.tracks.resize(40);
  for(unsigned c=0;c<40;++c)for(unsigned r=1;r<=9;++r)image.tracks[c].sectors.push_back({static_cast<unsigned char>(c),0,static_cast<unsigned char>(r),2,0,0,Bytes(512,static_cast<unsigned char>(c+r))});
  auto &boot=image.tracks[0].sectors[0].data;std::fill(boot.begin(),boot.end(),0);Bytes parameters={0,0,40,9,2,1,3,2,0x2a,0x52};std::copy(parameters.begin(),parameters.end(),boot.begin());
  Bytes code={0x3e,0xa5,0x32,0,0x80,0x18,0xfe};std::copy(code.begin(),code.end(),boot.begin()+16);
  unsigned sum=0;for(auto b:boot)sum+=b;boot[511]=3-sum;return image;
}
static void Command(Disks::Controller &fdc,std::initializer_list<unsigned char> bytes){for(auto b:bytes){fdc.Advance(1000);fdc.Write(b);}}
static Bytes Result(Disks::Controller &fdc){Bytes out;while(fdc.state.phase==Disks::Controller::Phase::Result){fdc.Advance(1000);out.push_back(fdc.Read());assert(out.size()<=7);}return out;}
int main() {
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);char directory[]="/tmp/gomas-media-XXXXXX";assert(mkdtemp(directory));std::string root=std::string(directory)+"/",error;
  SpectrumMachine m;assert(m.LoadROM(FindROM("48.rom"),error));SaveStates::Store saves(directory);
  for(auto model:{MachineModel::Spectrum48,MachineModel::Spectrum128,MachineModel::Plus2})for(bool compressed:{false,true}) {
    auto szx=SZXFile(model,compressed);File(root+"game.szx",szx);assert(m.Open(root+"game.szx",error));assert(m.Configuration().model==model && m.cpu.regs.PC==0x8000 && m.cpu.regs.AF==0x1245);
    assert(m.bus.Read(0x8000)==0xc3);assert(saves.Save(0,m,error));SpectrumMachine restored;assert(saves.Load(0,restored,error));assert(Core(restored)==Core(m));
    Zip(root+"game.zip","game.szx",szx);assert(m.Open(root+"game.zip",error));
  }
  // Imported HALT is independent of whether PC points at a HALT opcode.
  auto halted=SZXFile(MachineModel::Spectrum48,true);halted[50]=2;File(root+"game.szx",halted);assert(m.Open(root+"game.szx",error));
  m.Frame();assert(m.cpu.regs.PC==0x8000 && m.CaptureState().snapshotHalted);
  assert(saves.Save(0,m,error));SpectrumMachine haltRestored;assert(saves.Load(0,haltRestored,error) && haltRestored.CaptureState().snapshotHalted);
  m.cpu.regs.iff1A=m.cpu.regs.iff1B=1;m.Frame();assert(m.cpu.regs.PC==0x38 && !m.CaptureState().snapshotHalted);
  // Existing GSS v1 files remain readable after adding disk and SLT state.
  assert(m.LoadROM(FindROM("48.rom"),error));auto record=SaveStates::Capture(m);SaveStates::Writer payload;
  payload.u64(record.created);payload.text(record.label);payload.text(record.tapePath);payload.u8(unsigned(record.rom.model));payload.text(record.rom.source);
  payload.u32(record.rom.pages.size());for(const auto &page:record.rom.pages)payload.blob(page);payload.blob(record.tape);payload.blob(record.thumbnail);SaveStates::WriteCore(payload,record.core,1);
  SaveStates::Writer legacy;const unsigned char magic[]={'G','O','M','A','S','S','T',0};legacy.raw(magic,8);legacy.u32(1);legacy.u32(payload.bytes.size());legacy.u64(ROMFingerprint(payload.bytes.data(),payload.bytes.size()));legacy.raw(payload.bytes.data(),payload.bytes.size());
  auto old=SaveStates::Decode(legacy.bytes);assert(old.core.registers.PC==m.cpu.regs.PC && old.core.memory.ram==m.memory.CaptureState().ram);
  auto szx=SZXFile(MachineModel::Spectrum128,true);Bytes tape={3,0,255,128,127},body;W(body,0);W(body,3);D(body,tape.size());auto compressed=Deflate(tape);D(body,compressed.size());body.resize(body.size()+16);body[12]='t';body[13]='a';body[14]='p';body.insert(body.end(),compressed.begin(),compressed.end());Chunk(szx,"TAPE",body);File(root+"game.szx",szx);assert(m.Open(root+"game.szx",error) && m.tape==tape);
  // Linked tapes resolve relative to either the SZX file or its ZIP member.
  auto linked=SZXFile(MachineModel::Spectrum128,true);Bytes link;W(link,0);W(link,0);D(link,0);D(link,0);link.resize(28);link[12]='t';link[13]='a';link[14]='p';
  const char filename[]="linked.tap";link.insert(link.end(),filename,filename+sizeof(filename));Chunk(linked,"TAPE",link);
  File(root+"linked.tap",tape);File(root+"game.szx",linked);assert(m.Open(root+"game.szx",error) && m.tape==tape);
  {
    auto *zip=archive_write_new();assert(archive_write_set_format_zip(zip)==ARCHIVE_OK);assert(archive_write_open_filename(zip,(root+"game.zip").c_str())==ARCHIVE_OK);
    for(const auto &item:std::vector<std::pair<std::string,Bytes>>{{"folder/game.szx",linked},{"folder/linked.tap",tape}}){auto *entry=archive_entry_new();archive_entry_set_pathname(entry,item.first.c_str());archive_entry_set_filetype(entry,AE_IFREG);archive_entry_set_perm(entry,0644);archive_entry_set_size(entry,item.second.size());
      assert(archive_write_header(zip,entry)==ARCHIVE_OK);assert(archive_write_data(zip,item.second.data(),item.second.size())==static_cast<la_ssize_t>(item.second.size()));archive_entry_free(entry);}
    assert(archive_write_close(zip)==ARCHIVE_OK);archive_write_free(zip);
  }
  assert(m.Open(root+"game.zip",error) && m.tape==tape);
  auto slt=Z80File(2,MachineModel::Spectrum48,true);slt.insert(slt.end(),{0,0,0,'S','L','T'});Bytes level={1,2,0xed,0xed,0xed,255};auto packed=Pack(level);
  W(slt,1);W(slt,0x12);D(slt,packed.size());W(slt,0);W(slt,0);D(slt,0);slt.insert(slt.end(),packed.begin(),packed.end());File(root+"game.slt",slt);assert(m.Open(root+"game.slt",error));
  m.cpu.regs.PC=0x8000;m.cpu.regs.HL=0xa000;m.bus.Write(0x8000,0xed);m.bus.Write(0x8001,0xfb);assert(m.TryLevelLoad());
  for(size_t i=0;i<level.size();++i)assert(m.bus.Read(0xa000+i)==level[i]);assert(m.cpu.regs.HL==0xa000 && m.cpu.regs.PC==0x8002);
  assert(saves.Save(0,m,error));SpectrumMachine restored;assert(saves.Load(0,restored,error));assert(Core(restored)==Core(m));
  Zip(root+"game.zip","game.slt",slt);assert(m.Open(root+"game.zip",error));
  // A deferred TZX branch resumes the chosen route and survives saved games.
  auto tzx=TZXHeader();tzx.push_back(0x28);W(tzx,13);tzx.push_back(2);W(tzx,1);tzx.push_back(3);tzx.insert(tzx.end(),{'O','n','e'});W(tzx,3);tzx.push_back(3);tzx.insert(tzx.end(),{'T','w','o'});
  Tone(tzx,100,2);tzx.push_back(0x23);W(tzx,2);Tone(tzx,200,2);Tone(tzx,300,2);
  File(root+"game.tzx",tzx);assert(m.Open(root+"game.tzx",error) && m.deck.NeedsChoice());assert(!m.PlayTape());auto undecided=m.CaptureState();
  assert(m.ChooseTape(1) && !m.deck.NeedsChoice() && m.deck.UntilEdge()==200);assert(saves.Save(0,m,error));assert(saves.Load(0,restored,error));assert(Core(restored)==Core(m));
  assert(m.RestoreState(undecided) && m.deck.NeedsChoice());assert(m.ChooseTape(0) && m.deck.UntilEdge()==100);assert(m.CueTape(0) && m.deck.NeedsChoice());
  // All +3 all-RAM maps and four ROM pages, including lock/reset.
  assert(m.LoadROM(FindROM("plus3-0.rom"),error));for(unsigned bank=0;bank<8;++bank){m.io.Write(0x7ffd,bank);m.bus.Write(0xc000,0x80+bank);}
  const unsigned maps[4][4]={{0,1,2,3},{4,5,6,7},{4,5,6,3},{4,7,6,3}};
  for(unsigned map=0;map<4;++map){m.io.Write(0x1ffd,1+map*2);for(unsigned slot=0;slot<4;++slot)assert(m.bus.Read(slot*16384)==0x80+maps[map][slot]);}
  for(unsigned rom=0;rom<4;++rom){m.io.Write(0x1ffd,(rom&2)<<1);m.io.Write(0x7ffd,(rom&1)<<4);assert(m.memory.ROMPage()==rom && m.bus.Read(0)==m.systemROM.pages[rom][0]);}
  m.io.Write(0x7ffd,32);m.io.Write(0x1ffd,7);assert(m.memory.PagingLatchPlus3()==4);m.Reset();assert(m.memory.PagingLatchPlus3()==0);
  auto disk=DiskFixture();auto bytes=disk.Encode();assert(Disks::Image::Parse(bytes).tracks[0].sectors[0].data==disk.tracks[0].sectors[0].data);
  auto diskSnapshot=SZXFile(MachineModel::Spectrum128,true);diskSnapshot[6]=5;Bytes diskChunk;W(diskChunk,3);diskChunk.push_back(0);D(diskChunk,bytes.size());auto packedDisk=Deflate(bytes);diskChunk.insert(diskChunk.end(),packedDisk.begin(),packedDisk.end());Chunk(diskSnapshot,"DSK\0",diskChunk);Chunk(diskSnapshot,"+3\0\0",{1,1});
  File(root+"game.szx",diskSnapshot);assert(m.Open(root+"game.szx",error) && m.Configuration().model==MachineModel::Plus3 && m.io.floppy.state.drives[0].image && m.io.floppy.state.motor);
  File(root+"game.dsk",bytes);assert(m.Open(root+"game.dsk",error));assert(m.Configuration().model==MachineModel::Plus3 && m.io.floppy.state.drives[0].image);
  auto &fdc=m.io.floppy;fdc.Motor(true);
  Command(fdc,{0x46,0,0,0,1,2,1,0x2a,0xff});Bytes sector;for(unsigned n=0;n<512;++n){fdc.Advance(114);assert((fdc.Status()&0xc0)==0xc0);sector.push_back(fdc.Read());}
  assert(sector==disk.tracks[0].sectors[0].data && Result(fdc).size()==7);
  Command(fdc,{0x45,0,0,0,2,2,2,0x2a,0xff});for(unsigned n=0;n<512;++n){fdc.Advance(114);fdc.Write(n&255);}assert(Result(fdc).size()==7);
  assert(fdc.state.drives[0].dirty && fdc.state.drives[0].image->tracks[0].sectors[1].data[255]==255);
  fdc.state.drives[0].writeProtected=true;Command(fdc,{0x45,0,0,0,2,2,2,0x2a,0xff});auto result=Result(fdc);assert(result.size()==7 && (result[1]&2));fdc.state.drives[0].writeProtected=false;
  Command(fdc,{0x46,0,0,0,2,2,2,0x2a,0xff});for(unsigned n=0;n<23;++n){fdc.Advance(114);assert(fdc.Read()==n);}
  assert(saves.Save(0,m,error));assert(saves.Load(0,restored,error));assert(Core(restored)==Core(m));for(unsigned n=23;n<512;++n){m.io.floppy.Advance(114);restored.io.floppy.Advance(114);assert(m.io.floppy.Read()==restored.io.floppy.Read());}
  assert(m.SaveDisk(0,root+"saved.dsk",error));assert(Disks::Image::Parse(Read(root+"saved.dsk")).tracks[0].sectors[1].data[255]==255);
  // Seek, sense-interrupt and format run through command/data/result phases.
  while(!fdc.state.interrupts.empty()){Command(fdc,{8});Result(fdc);}
  Command(fdc,{15,2,2});fdc.Advance(100000);Command(fdc,{8});result=Result(fdc);assert(result.size()==2 && result[1]==2 && (result[0]&3)==2 && fdc.state.cylinder[0]==2);
  Command(fdc,{7,0});fdc.Advance(100000);Command(fdc,{8});result=Result(fdc);assert(result.size()==2 && result[1]==0);
  Command(fdc,{13,0,2,2,0x2a,0xe5});for(unsigned char value:{0,0,1,2,0,0,2,2}){fdc.Advance(114);fdc.Write(value);}assert(Result(fdc).size()==7);
  assert(fdc.state.drives[0].image->tracks[0].sectors.size()==2 && fdc.state.drives[0].image->tracks[0].sectors[0].data[0]==0xe5);
  assert(restored.io.floppy.state.drives[0].image->tracks[0].sectors.size()==9); // Copy-on-write preserves older states.
  auto preserved=Core(m);auto bad=bytes;bad.resize(100);File(root+"bad.dsk",bad);assert(!m.Open(root+"bad.dsk",error) && Core(m)==preserved);unlink((root+"bad.dsk").c_str());
  Zip(root+"game.zip","disk.dsk",bytes);assert(m.Open(root+"game.zip",error));assert(m.io.floppy.state.drives[0].image);
  // Real +3 ROM reaches its menu and boots a synthetic, checksummed +3 disk.
  for(unsigned frame=0;frame<450;++frame)m.Frame();
  m.ula.PressKey(2,9,true);for(unsigned frame=0;frame<10;++frame)m.Frame();m.ula.PressKey(2,9,false);
  bool booted=false;for(unsigned frame=0;frame<2000 && !booted;++frame){m.Frame();booted=m.bus.Read(0x8000)==0xa5;}
  if(!booted)fprintf(stderr,"+3 boot PC=%04x ROM=%u paging=%02x/%02x FDC phase=%u cmd=%zu motor=%d cylinder=%u\n",m.cpu.regs.PC,m.memory.ROMPage(),m.memory.PagingLatch(),m.memory.PagingLatchPlus3(),unsigned(m.io.floppy.state.phase),m.io.floppy.state.command.size(),m.io.floppy.state.motor,m.io.floppy.state.cylinder[0]);
  assert(booted);
  for(const char *name:{"game.szx","linked.tap","game.slt","game.tzx","game.dsk","game.zip","saved.dsk","slot-0.gss"})unlink((root+name).c_str());assert(rmdir(directory)==0);
  puts("New media: SZX/SLT/ZIP, TZX branch restore, +3 paging, DSK reads/writes/protection/saved transfers and real +3 ROM disk boot passed");
}
