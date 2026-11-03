#include <cassert>
#include <cstdio>
#include <atomic>
#include "../catalog_load.h"
using namespace GameCatalog;
static Json GameRow(unsigned id,const std::string &title){return Json{{"id",id},{"title",title},{"year",1986},{"legalStatus","allowed"}};}
static Json Reply(const char *key,Json rows,unsigned total){return Json{{"responseStatus","success"},{"totalAmount",total},{"responseData",{{key,rows}}}};}
static Bytes Data(const Json &j){auto text=j.dump();return {text.begin(),text.end()};}
static void Reject(const std::function<void()> &action){bool rejected=false;try{action();}catch(const std::exception &){rejected=true;}assert(rejected);}
static Bytes Zip(const Bytes &tap){Bytes buffer(65536);size_t size=0;auto *archive=archive_write_new();assert(archive_write_set_format_zip(archive)==ARCHIVE_OK);assert(archive_write_open_memory(archive,buffer.data(),buffer.size(),&size)==ARCHIVE_OK);
  auto *entry=archive_entry_new();archive_entry_set_pathname(entry,"nested/game.tap");archive_entry_set_filetype(entry,AE_IFREG);archive_entry_set_perm(entry,0644);archive_entry_set_size(entry,tap.size());assert(archive_write_header(archive,entry)==ARCHIVE_OK);assert(archive_write_data(archive,tap.data(),tap.size())==ssize_t(tap.size()));archive_entry_free(entry);archive_write_close(archive);archive_write_free(archive);buffer.resize(size);return buffer;}
int main(int argc,char **argv) {
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);char directory[]="/tmp/gomas-catalog-XXXXXX";assert(mkdtemp(directory));std::string root=directory;
  Bytes tap={3,0,255,128,127},zip=Zip(tap);std::atomic<unsigned> calls(0);bool broken=false,invalidZip=false,networkDown=false;std::string lastURL;
  Json games=Json::array({GameRow(1,"Alpha &amp; Beta"),GameRow(2,"Zeta")});
  Json release={{"id",11},{"year",1986},{"releaseType","original"},{"hardwareRequired",Json::array({"zx128","ay"})},
    {"releaseStructure",Json::array({{{"id",1},{"fileName","Bundle.zip"},{"type","zip"},{"size",zip.size()},{"md5",MD5(zip)},
      {"items",Json::array({{{"id",2},{"fileName","../../Alpha - 48k.tap"},{"type","tap"},{"size",tap.size()},{"md5",MD5(tap)}}})}}})}};
  Fetcher fake=[&](const std::string &url,size_t,Progress &p)->Bytes{++calls;lastURL=url;if(p.cancel)throw std::runtime_error("Cancelled");if(networkDown)throw std::runtime_error("offline");
    if(url.find("export:zxProd")!=std::string::npos)return Data(Reply("zxProd",url.find("start:30")!=std::string::npos?Json::array({GameRow(3,"Bravo")}):games,61));
    if(url.find("export:zxRelease")!=std::string::npos)return Data(Reply("zxRelease",Json::array({release}),1));
    if(url.find("fileId:1/")!=std::string::npos)return invalidZip?Bytes(zip.size(),0):zip;
    return broken?Bytes(tap.size(),0):tap;};
  Library library(root,fake);Progress p;auto page=library.Search("Alpha;id=1",0,false,p);assert(page.total==61 && page.games.size()==2 && page.games[0].title=="Alpha & Beta");
  assert(lastURL.find("Alpha%20id%201")!=std::string::npos);library.Search("",1,true,p);assert(lastURL.find("order:title,desc")!=std::string::npos);
  auto cached=library.Cached("",0,false,false);assert(cached.total==3 && cached.games[0].title=="Alpha & Beta" && cached.games[1].title=="Bravo");
  auto game=page.games[0];auto versions=library.Versions(game,p);assert(versions.size()==2 && versions[0].type=="tap" && versions[0].model==MachineModel::Spectrum48);
  auto file=versions[0];auto path=library.Install(file,false,p);assert(library.Installed(file) && library.Installed(game));assert(path==root+"/zxart/1/11-2/Alpha - 48k.tap");
  assert(lastURL.find("..%2F..%2FAlpha%20-%2048k.tap")!=std::string::npos);unsigned downloaded=calls;
  networkDown=true;assert(library.Install(file,true,p)==path && calls==downloaded);library.SetOffline(true);
  Library offline(root,fake);assert(offline.Offline() && offline.Cached("alpha",0,false,true).total==1);assert(offline.Install(offline.CachedFiles(game)[0],true,p)==path && calls==downloaded);
  // Missing/damaged local files fail offline; a bad replacement preserves the previous bytes.
  auto before=Read(path,100);broken=true;networkDown=false;auto updated=file;updated.md5=MD5(Bytes{3,0,255,1,254});Reject([&](){library.Install(updated,false,p);});assert(Read(path,100)==before);
  p.cancel=true;Reject([&](){library.Install(file,false,p);});p.cancel=false;assert(Read(path,100)==before);
  auto absent=file;absent.id=99;Reject([&](){offline.Install(absent,true,p);});assert(!al_filename_exists(offline.Path(absent).c_str()));
  auto zipFile=versions[1];assert(zipFile.type=="zip");broken=false;auto zipPath=library.Install(zipFile,false,p);assert(ValidateTape(tap) && Read(zipPath,65536)==zip);
  auto zipUpdate=zipFile;zipUpdate.md5=MD5(Bytes(zip.size(),0));invalidZip=true;Reject([&](){library.Install(zipUpdate,false,p);});assert(Read(zipPath,65536)==zip);
  // Auto-start goes through real ROM input and returns a complete, transactional session.
  SpectrumMachine machine;std::string error;assert(machine.LoadROM(FindROM("48.rom"),error));auto core=machine.CaptureState();
  assert(GameCatalog::Load(machine,path,MachineModel::Spectrum48,error));assert(machine.tape==tap && machine.FrameCounter()>=580);
  auto oldPC=machine.cpu.regs.PC;assert(!GameCatalog::Load(machine,root+"/missing.tap",MachineModel::Spectrum128,error) && machine.cpu.regs.PC==oldPC);
  for(auto model:{MachineModel::Spectrum48,MachineModel::Spectrum128,MachineModel::Plus2,MachineModel::Plus3}){
    assert(GameCatalog::Load(machine,"cobra.tap",model,error));
    for(unsigned frame=0;frame<1500 && machine.deck.CurrentBlock()!=machine.deck.Blocks().size();++frame)machine.Frame();
    if(machine.deck.CurrentBlock()!=machine.deck.Blocks().size())fprintf(stderr,"Catalog auto-load failed: model=%u PC=%04x block=%zu\n",unsigned(model),machine.cpu.regs.PC,machine.deck.CurrentBlock());
    assert(machine.deck.CurrentBlock()==machine.deck.Blocks().size());
  }
  // A live smoke check is opt-in; the regular suite never needs the network.
  if(argc>1 && std::string(argv[1])=="--live"){
    Library live(root+"/live");Progress progress;auto found=live.Search("Rick Dangerous 2",0,false,progress);assert(!found.games.empty());auto files=live.Versions(found.games[0],progress);assert(!files.empty());
    const auto &download=*std::find_if(files.begin(),files.end(),[](const GameCatalog::File &f){return f.supported && (f.type=="tap" || f.type=="tzx");});
    auto downloaded=live.Install(download,false,progress);printf("Live ZXArt: %s, %zu variants, downloaded %s\n",found.games[0].title.c_str(),files.size(),downloaded.c_str());
    remove(downloaded.c_str());rmdir(ROMDirectory(downloaded).c_str());remove((root+"/live/catalog.json").c_str());rmdir((root+"/live/zxart/"+std::to_string(download.game)).c_str());rmdir((root+"/live/zxart").c_str());rmdir((root+"/live").c_str());
  }
  remove(path.c_str());remove(zipPath.c_str());rmdir(ROMDirectory(path).c_str());rmdir(ROMDirectory(zipPath).c_str());remove((root+"/catalog.json").c_str());rmdir((root+"/zxart/1").c_str());rmdir((root+"/zxart").c_str());assert(rmdir(root.c_str())==0);
  puts("Catalog: pagination/search/sort, versions, safe cache paths, checksums, cancellation, offline reuse, ZIP validation and transactional loading passed");
}
