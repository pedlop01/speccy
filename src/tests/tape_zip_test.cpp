#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <unistd.h>
#include "../save_state.h"

struct File {std::string name;std::vector<unsigned char> bytes;};
static void Zip(const std::string &path,const std::vector<File> &files,bool compressed=true) {
  archive *writer=archive_write_new();assert(writer);
  assert(archive_write_set_format_zip(writer)==ARCHIVE_OK);
  assert(archive_write_set_options(writer,compressed?"zip:compression=deflate":"zip:compression=store")==ARCHIVE_OK);
  assert(archive_write_open_filename(writer,path.c_str())==ARCHIVE_OK);
  for(const auto &file:files) {
    archive_entry *entry=archive_entry_new();archive_entry_set_pathname(entry,file.name.c_str());
    archive_entry_set_filetype(entry,AE_IFREG);archive_entry_set_perm(entry,0644);archive_entry_set_size(entry,file.bytes.size());
    assert(archive_write_header(writer,entry)==ARCHIVE_OK);
    assert(archive_write_data(writer,file.bytes.data(),file.bytes.size())==static_cast<la_ssize_t>(file.bytes.size()));
    archive_entry_free(entry);
  }
  assert(archive_write_close(writer)==ARCHIVE_OK);assert(archive_write_free(writer)==ARCHIVE_OK);
}
static std::vector<unsigned char> Bytes(const std::string &path) {
  std::ifstream file(path,std::ios::binary);assert(file);return {(std::istreambuf_iterator<char>(file)),{}};
}
static std::vector<unsigned char> Core(const SpectrumMachine &m) {
  SaveStates::Writer w;SaveStates::WriteCore(w,m.CaptureState());return w.bytes;
}
int main() {
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  char directory[]="/tmp/gomas-zip-XXXXXX";assert(mkdtemp(directory));
  std::string path=std::string(directory)+"/game.ZIP",error;
  const std::vector<unsigned char> tap={3,0,255,128,127},other={3,0,255,55,200};
  std::vector<unsigned char> result={42};
  for(bool compressed:{false,true}) {
    // Directories / sidecar files and traversal names never become filesystem writes.
    Zip(path,{{"readme.txt",{1,2,3}},{"../../game.TAP",tap},{"second.tap",other}},compressed);
    assert(TapeFiles::Read(path,result,error) && result==tap);
  }
  SpectrumMachine machine;assert(machine.LoadROM(FindROM("48.rom"),error));
  auto cobra=Bytes("cobra.tap");Zip(path,{{"games/cobra.tap",cobra}});
  assert(machine.Open(path,error));assert(machine.tape==cobra && machine.tapePath==path);
  // The core receives the same tape for both loading methods.
  for(bool fast:{true,false}) {
    SpectrumMachine plain,zipped;assert(plain.LoadROM(FindROM("48.rom"),error));assert(zipped.LoadROM(FindROM("48.rom"),error));
    assert(plain.Open("cobra.tap",error));assert(zipped.Open(path,error));
    plain.SetFastLoad(fast);zipped.SetFastLoad(fast);
    if(!fast) {assert(plain.PlayTape());assert(zipped.PlayTape());}
    std::vector<short> a(48000),b(a.size());plain.ReadAudio(a.data(),a.size());zipped.ReadAudio(b.data(),b.size());
    assert(a==b && Core(plain)==Core(zipped));
  }
  SaveStates::Store saves(directory);assert(saves.Save(0,machine,error));unlink(path.c_str());
  SpectrumMachine restored;assert(saves.Load(0,restored,error));assert(restored.tape==cobra && restored.tapePath==path);
  auto before=Core(machine);auto oldTape=machine.tape;auto oldPath=machine.tapePath;
  auto reject=[&]() {
    result={42};assert(!TapeFiles::Read(path,result,error));assert(!error.empty() && result==std::vector<unsigned char>{42});
    assert(!machine.Open(path,error));assert(Core(machine)==before && machine.tape==oldTape && machine.tapePath==oldPath);
  };
  reject(); // Missing ZIP.
  Zip(path,{{"readme.txt",{1}}});reject(); // No TAP.
  Zip(path,{{"broken.tap",{3,0,1}}});reject();
  Zip(path,{{"empty.tap",{}}});reject();
  Zip(path,{{"huge.tap",std::vector<unsigned char>(TapeFiles::MaxBytes+1)}});reject();
  // Corrupt stored data without changing TAP framing: CRC must catch it.
  Zip(path,{{"crc.tap",tap}},false);auto corrupt=Bytes(path);
  auto payload=std::search(corrupt.begin(),corrupt.end(),tap.begin(),tap.end());assert(payload!=corrupt.end());payload[4]^=1;
  {std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(corrupt.data()),corrupt.size());}reject();
  corrupt.resize(12);{std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(corrupt.data()),corrupt.size());}reject();
  unlink(path.c_str());unlink(saves.Path(0).c_str());assert(rmdir(directory)==0);
  puts("ZIP tapes: stored/deflated, nested/uppercase TAP, first entry, fast/pulse equivalence, saved games, limits/CRC/truncation and preserved session passed");
}
