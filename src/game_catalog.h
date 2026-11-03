#ifndef GAME_CATALOG_H
#define GAME_CATALOG_H
#include <allegro5/allegro.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>
#include "emulator.h"

namespace GameCatalog {
using Json=nlohmann::json;
using Bytes=std::vector<unsigned char>;
struct Progress {std::atomic<bool> cancel{false};std::atomic<size_t> received{0},total{0};};
using Fetcher=std::function<Bytes(const std::string &,size_t,Progress &)>;
inline std::string Lower(std::string s){for(auto &c:s)if(c>='A' && c<='Z')c+=32;return s;}
inline std::string Text(const Json &object,const char *key) {
  auto it=object.find(key);if(it==object.end() || it->is_null())return "";
  std::string s=it->is_string()?it->get<std::string>():it->is_number_integer()?it->dump():"";
  if(s.size()>4096)s.resize(4096);for(auto &c:s)if(static_cast<unsigned char>(c)<32)c=' ';
  const std::pair<const char *,const char *> entities[]={{"&amp;","&"},{"&quot;","\""},{"&#039;","'"},{"&apos;","'"},{"&lt;","<"},{"&gt;",">"}};
  for(const auto &entity:entities){size_t p=0;while((p=s.find(entity.first,p))!=std::string::npos){s.replace(p,strlen(entity.first),entity.second);p+=strlen(entity.second);}}
  return s;
}
inline std::uint64_t Id(const Json &j,const char *key) {
  auto it=j.find(key);if(it==j.end() || !it->is_number_integer())throw std::runtime_error("Catalog response has an invalid identifier");
  auto value=it->get<std::int64_t>();if(value<=0 || value>9007199254740991LL)throw std::runtime_error("Invalid catalog identifier");return value;
}
inline std::string Escape(const std::string &value){static const char hex[]="0123456789ABCDEF";std::string out;
  for(unsigned char c:value)if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c=='-' || c=='_' || c=='.' || c=='~')out+=char(c);
    else{out+='%';out+=hex[c>>4];out+=hex[c&15];}return out;}
inline Bytes Http(const std::string &url,size_t limit,Progress &progress) {
  static const bool initialized=curl_global_init(CURL_GLOBAL_DEFAULT)==CURLE_OK;if(!initialized)throw std::runtime_error("Cannot initialize HTTPS");
  std::unique_ptr<CURL,decltype(&curl_easy_cleanup)> curl(curl_easy_init(),curl_easy_cleanup);if(!curl)throw std::runtime_error("Cannot create HTTPS request");
  struct Transfer{Bytes bytes;size_t limit;Progress *progress;bool oversized=false;};Transfer transfer;transfer.limit=limit;transfer.progress=&progress;
  auto write=+[](char *data,size_t size,size_t count,void *user)->size_t{
    auto &t=*static_cast<Transfer *>(user);if(size && count>SIZE_MAX/size)return 0;size_t n=size*count;
    if(t.progress->cancel)return 0;if(n>t.limit-t.bytes.size()){t.oversized=true;return 0;}
    try{t.bytes.insert(t.bytes.end(),data,data+n);}catch(...){return 0;}return n;
  };
  auto update=+[](void *user,curl_off_t total,curl_off_t now,curl_off_t,curl_off_t)->int{
    auto &p=*static_cast<Progress *>(user);p.received=now>0?size_t(now):0;p.total=total>0?size_t(total):0;return p.cancel?1:0;
  };
  curl_easy_setopt(curl.get(),CURLOPT_URL,url.c_str());curl_easy_setopt(curl.get(),CURLOPT_PROTOCOLS_STR,"https");curl_easy_setopt(curl.get(),CURLOPT_REDIR_PROTOCOLS_STR,"https");
  curl_easy_setopt(curl.get(),CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(curl.get(),CURLOPT_MAXREDIRS,4L);curl_easy_setopt(curl.get(),CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT,40L);
  curl_easy_setopt(curl.get(),CURLOPT_LOW_SPEED_LIMIT,128L);curl_easy_setopt(curl.get(),CURLOPT_LOW_SPEED_TIME,15L);curl_easy_setopt(curl.get(),CURLOPT_NOSIGNAL,1L);
  curl_easy_setopt(curl.get(),CURLOPT_USERAGENT,"Gomas/1.0 (ZX Spectrum emulator; ZXArt catalog)");curl_easy_setopt(curl.get(),CURLOPT_ACCEPT_ENCODING,"");
  curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(curl.get(),CURLOPT_WRITEDATA,&transfer);curl_easy_setopt(curl.get(),CURLOPT_XFERINFOFUNCTION,update);curl_easy_setopt(curl.get(),CURLOPT_XFERINFODATA,&progress);curl_easy_setopt(curl.get(),CURLOPT_NOPROGRESS,0L);
  CURLcode result=curl_easy_perform(curl.get());long status=0;curl_easy_getinfo(curl.get(),CURLINFO_RESPONSE_CODE,&status);
  if(progress.cancel)throw std::runtime_error("Cancelled");if(transfer.oversized)throw std::runtime_error("Download exceeds the supported size limit");
  if(result!=CURLE_OK)throw std::runtime_error(std::string("Network error: ")+curl_easy_strerror(result));
  if(status!=200)throw std::runtime_error("ZXArt returned HTTP "+std::to_string(status));return transfer.bytes;
}
inline std::string MD5(const Bytes &bytes){unsigned char digest[EVP_MAX_MD_SIZE];unsigned size=0;
  if(!EVP_Digest(bytes.data(),bytes.size(),digest,&size,EVP_md5(),nullptr))throw std::runtime_error("Cannot verify download checksum");
  static const char hex[]="0123456789abcdef";std::string out;for(unsigned n=0;n<size;++n){out+=hex[digest[n]>>4];out+=hex[digest[n]&15];}return out;}
inline Bytes Read(const std::string &path,size_t limit){std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("Cached file is missing");auto size=file.tellg();
  if(size<0 || size>std::streamoff(limit))throw std::runtime_error("Cached file is too large");Bytes bytes(static_cast<size_t>(size));file.seekg(0);if(!bytes.empty())file.read(reinterpret_cast<char *>(bytes.data()),bytes.size());if(!file)throw std::runtime_error("Cannot read cached file");return bytes;}
inline void AtomicWrite(const std::string &path,const Bytes &bytes){std::string pattern=path+".part-XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
  int fd=mkstemp(name.data());if(fd<0)throw std::runtime_error("Cannot create game cache file");std::string temporary=name.data();FILE *file=fdopen(fd,"wb");
  if(!file){close(fd);remove(temporary.c_str());throw std::runtime_error("Cannot write game cache file");}
  bool ok=fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();ok=(fflush(file)==0)&&ok;ok=(fsync(fileno(file))==0)&&ok;ok=(fclose(file)==0)&&ok;
  if(!ok || rename(temporary.c_str(),path.c_str())){remove(temporary.c_str());throw std::runtime_error("Cannot replace game cache file");}}
inline std::string Directory(const std::string &executable){if(const char *path=getenv("GOMAS_GAMES_DIR"))return path;
  auto dir=ROMDirectory(executable);if(al_filename_exists((dir+"../roms").c_str()) && access((dir+"..").c_str(),W_OK)==0)return dir+"../games";
  std::unique_ptr<ALLEGRO_PATH,decltype(&al_destroy_path)> path(al_get_standard_path(ALLEGRO_USER_DATA_PATH),al_destroy_path);
  if(!path)return "games";al_append_path_component(path.get(),"games");return al_path_cstr(path.get(),ALLEGRO_NATIVE_PATH_SEP);}
struct Game {std::uint64_t id=0;std::string title,year,status;};
struct File {std::uint64_t game=0,release=0,id=0;std::string name,type,year,publisher,releaseType,md5;size_t size=0;MachineModel model=MachineModel::Spectrum48;bool supported=true;};
struct Page {std::vector<Game> games;size_t total=0;unsigned page=0;};
constexpr unsigned PageSize=30;
inline Json Encode(const Game &g){return Json{{"id",g.id},{"title",g.title},{"year",g.year},{"status",g.status}};}
inline Json Encode(const File &f){return Json{{"game",f.game},{"release",f.release},{"id",f.id},{"name",f.name},{"type",f.type},{"year",f.year},{"publisher",f.publisher},{"releaseType",f.releaseType},{"md5",f.md5},{"size",f.size},{"model",unsigned(f.model)},{"supported",f.supported}};}
inline Game DecodeGame(const Json &j){Game g;g.id=Id(j,"id");g.title=Text(j,"title");g.year=Text(j,"year");g.status=Text(j,"status");if(g.title.empty())throw std::runtime_error("Catalog game has no title");return g;}
inline bool Format(const std::string &type){return type=="tap" || type=="tzx" || type=="sna" || type=="z80" || type=="szx" || type=="slt" || type=="dsk" || type=="zip";}
inline File DecodeFile(const Json &j){File f;f.game=Id(j,"game");f.release=Id(j,"release");f.id=Id(j,"id");f.name=Text(j,"name");f.type=Lower(Text(j,"type"));f.year=Text(j,"year");f.publisher=Text(j,"publisher");f.releaseType=Text(j,"releaseType");f.md5=Lower(Text(j,"md5"));
  f.size=j.value("size",size_t(0));unsigned model=j.value("model",0u);f.supported=j.value("supported",true);
  if(!Format(f.type) || model>unsigned(MachineModel::Plus3) || f.size>TapeFiles::MaxBytes || f.name.empty())throw std::runtime_error("Invalid cached game version");f.model=static_cast<MachineModel>(model);return f;}
inline std::string SafeName(const File &file){std::string name=file.name.substr(file.name.find_last_of("/\\")+1);auto dot=name.find_last_of('.');if(dot!=std::string::npos)name.resize(dot);std::string out;
  for(unsigned char c:name)if((c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') || c>=128 || c==' ' || c=='-' || c=='_')out+=char(c);else out+='_';
  if(out.empty())out="game";size_t n=std::min<size_t>(96,out.size());while(n<out.size() && n && (static_cast<unsigned char>(out[n])&0xc0)==0x80)--n;out.resize(n);return out+"."+file.type;
}
inline void Validate(const Bytes &bytes,const std::string &type) {
  if(bytes.empty() || bytes.size()>TapeFiles::MaxBytes)throw std::runtime_error("Invalid downloaded file size");
  if(type=="tap" || type=="tzx"){if(!ValidateTape(bytes) || (type=="tzx" && !TZX::Signature(bytes)))throw std::runtime_error("Downloaded tape is invalid");}
  else if(type=="sna")Snapshots::SNA(bytes);else if(type=="z80")Snapshots::Z80(bytes);else if(type=="szx")Snapshots::SZX(bytes);else if(type=="slt")Snapshots::SLT(bytes);else if(type=="dsk")Disks::Image::Parse(bytes);
  else if(type!="zip")throw std::runtime_error("Unsupported game format");
}
class Library {
public:
  explicit Library(std::string root,Fetcher fetch=Http):root(std::move(root)),fetch(std::move(fetch)){Load();}
  bool Offline() const{return offline;}
  void SetOffline(bool enabled){offline=enabled;Save();}
  std::string Root() const{return root;}
  std::string Path(const File &f) const{return root+"/zxart/"+std::to_string(f.game)+"/"+std::to_string(f.release)+"-"+std::to_string(f.id)+"/"+SafeName(f);}
  std::string Key(const File &f) const{return std::to_string(f.release)+"-"+std::to_string(f.id);}
  bool Installed(const File &f) const{auto found=installed.find(Key(f));return found!=installed.end() && al_filename_exists(Path(found->second).c_str());}
  std::set<std::uint64_t> InstalledGames() const{std::set<std::uint64_t> out;for(const auto &entry:installed)if(al_filename_exists(Path(entry.second).c_str()))out.insert(entry.second.game);return out;}
  std::set<std::string> InstalledFiles() const{std::set<std::string> out;for(const auto &entry:installed)if(al_filename_exists(Path(entry.second).c_str()))out.insert(entry.first);return out;}
  bool Installed(const Game &g) const{for(const auto &item:installed)if(item.second.game==g.id && al_filename_exists(Path(item.second).c_str()))return true;return false;}
  Page Cached(const std::string &query,unsigned page,bool descending,bool onlyInstalled) const {
    Page out;out.page=page;auto needle=Lower(query);
    for(const auto &item:games)if(Lower(item.second.title).find(needle)!=std::string::npos && (!onlyInstalled || Installed(item.second)))out.games.push_back(item.second);
    std::sort(out.games.begin(),out.games.end(),[&](const Game &a,const Game &b){return descending?Lower(a.title)>Lower(b.title):Lower(a.title)<Lower(b.title);});out.total=out.games.size();
    size_t start=size_t(page)*PageSize;if(start>=out.games.size())out.games.clear();else{auto end=std::min(out.games.size(),start+PageSize);out.games=std::vector<Game>(out.games.begin()+start,out.games.begin()+end);}return out;
  }
  std::vector<File> CachedFiles(const Game &game) const {
    auto found=files.find(game.id);std::vector<File> out=found==files.end()?std::vector<File>():found->second;
    for(auto &file:out){auto entry=installed.find(Key(file));if(entry!=installed.end() && Installed(entry->second))file=entry->second;}
    for(const auto &entry:installed)if(entry.second.game==game.id && std::none_of(out.begin(),out.end(),[&](const File &f){return Key(f)==entry.first;}))out.push_back(entry.second);return out;
  }
  Page Search(const std::string &query,unsigned page,bool descending,Progress &progress) {
    if(query.size()>200 || page>100000)throw std::runtime_error("Search is too large");
    std::string term=query;for(auto &c:term)if(c==';' || c=='/' || c==':' || c=='=')c=' ';
    auto response=Get("export:zxProd/language:eng/start:"+std::to_string(page*PageSize)+"/limit:"+std::to_string(PageSize)+"/order:title,"+(descending?"desc":"asc")+"/filter:zxProdCategory=92177;zxProdStatus=allowed,allowedzxart,unknown,recovered"+(query.empty()?"":";zxProdAjaxSearch="+Escape(term))+"/",progress);
    Page out;out.page=page;out.total=response.value("totalAmount",size_t(0));const auto &rows=response.at("responseData").at("zxProd");if(!rows.is_array() || rows.size()>PageSize)throw std::runtime_error("Invalid catalog page");
    for(const auto &row:rows){Game g;g.id=Id(row,"id");g.title=Text(row,"title");g.year=Text(row,"year");g.status=Text(row,"legalStatus");if(g.title.empty())continue;out.games.push_back(g);games[g.id]=g;}
    Save();return out;
  }
  std::vector<File> Versions(const Game &game,Progress &progress) {
    std::vector<File> out;std::set<std::string> seen;
    for(unsigned start=0;start<2000;start+=40) {
      auto response=Get("export:zxRelease/language:eng/start:"+std::to_string(start)+"/limit:40/filter:zxProdId="+std::to_string(game.id)+"/",progress);
      const auto &rows=response.at("responseData").at("zxRelease");if(!rows.is_array() || rows.size()>40)throw std::runtime_error("Invalid game version response");
      for(const auto &release:rows) {
        File base;base.game=game.id;base.release=Id(release,"id");base.year=Text(release,"year");base.releaseType=Text(release,"releaseType");
        bool machine=false,unsupported=false;
        if(release.contains("hardwareRequired") && release["hardwareRequired"].is_array())for(const auto &h:release["hardwareRequired"])if(h.is_string()){
          auto value=Lower(h.get<std::string>());if(value=="zx48"){base.model=MachineModel::Spectrum48;machine=true;}else if(value=="zx128"){base.model=MachineModel::Spectrum128;machine=true;}
          else if(value=="zxplus2"){base.model=MachineModel::Plus2;machine=true;}else if(value=="zxplus3"){base.model=MachineModel::Plus3;machine=true;}
          else if(value.find("pentagon")!=std::string::npos || value.find("tsconf")!=std::string::npos || value.find("next")!=std::string::npos || value=="zx81" || value.find("scorpion")!=std::string::npos)unsupported=true;
        }
        base.supported=machine || !unsupported;
        std::function<void(const Json &,unsigned)> walk=[&](const Json &entries,unsigned depth){
          if(depth>20)throw std::runtime_error("Catalog file nesting is too deep");if(!entries.is_array())return;
          for(const auto &entry:entries){auto type=Lower(Text(entry,"type"));if(Format(type)){
            File f=base;f.id=Id(entry,"id");f.name=Text(entry,"fileName");f.type=type;f.md5=Lower(Text(entry,"md5"));f.size=entry.value("size",size_t(0));
            auto name=Lower(f.name);if(type=="dsk")f.model=MachineModel::Plus3;else if(name.find("48k")!=std::string::npos)f.model=MachineModel::Spectrum48;else if(name.find("128k")!=std::string::npos)f.model=MachineModel::Spectrum128;
            if(f.size<=TapeFiles::MaxBytes && !f.name.empty() && seen.insert(Key(f)).second)out.push_back(std::move(f));
          }if(entry.contains("items"))walk(entry["items"],depth+1);}
        };
        if(release.contains("releaseStructure"))walk(release["releaseStructure"],0);
      }
      if(start+rows.size()>=response.value("totalAmount",size_t(0)) || rows.empty())break;
      if(progress.cancel)throw std::runtime_error("Cancelled");
    }
    std::stable_sort(out.begin(),out.end(),[](const File &a,const File &b){auto rank=[](const File &f){return (f.supported?0:100)+(f.releaseType=="original"?0:20)+(f.type=="tzx"?0:f.type=="tap"?1:f.type=="sna"?2:f.type=="z80"?3:f.type=="zip"?10:5);};return rank(a)<rank(b);});
    games[game.id]=game;files[game.id]=out;Save();return out;
  }
  std::string Install(const File &file,bool localOnly,Progress &progress) {
    if(progress.cancel)throw std::runtime_error("Cancelled");
    if(!file.supported)throw std::runtime_error("This version requires hardware not supported by Gomas");
    auto path=Path(file);
    if(Installed(file))try{auto existing=Path(installed.at(Key(file)));auto bytes=Read(existing,TapeFiles::MaxBytes);Check(file,bytes);if(progress.cancel)throw std::runtime_error("Cancelled");return existing;}catch(const std::exception &){if(localOnly)throw std::runtime_error("Installed file is missing or damaged; go online to download it again");}
    if(localOnly)throw std::runtime_error("This version is not installed. Switch off Offline to download it");
    if(progress.cancel)throw std::runtime_error("Cancelled");
    auto bytes=fetch("https://zxart.ee/zxfile/id:"+std::to_string(file.release)+"/fileId:"+std::to_string(file.id)+"/"+Escape(file.name),TapeFiles::MaxBytes,progress);Check(file,bytes);
    if(progress.cancel)throw std::runtime_error("Cancelled");
    if(!al_make_directory(ROMDirectory(path).c_str()))throw std::runtime_error("Cannot create games directory");
    if(file.type=="zip") {
      std::string pattern=path+".part-XXXXXX.zip";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);int fd=mkstemps(name.data(),4);
      if(fd<0)throw std::runtime_error("Cannot stage ZIP download");close(fd);std::string temporary=name.data();
      try{AtomicWrite(temporary,bytes);Bytes media;std::string name,error;if(!TapeFiles::ReadMedia(temporary,media,name,error))throw std::runtime_error(error);
        auto dot=name.find_last_of('.');Validate(media,dot==std::string::npos?"":Lower(name.substr(dot+1)));
        if(progress.cancel)throw std::runtime_error("Cancelled");if(rename(temporary.c_str(),path.c_str()))throw std::runtime_error("Cannot install ZIP download");
      }catch(...){remove(temporary.c_str());throw;}
    } else AtomicWrite(path,bytes);
    installed[Key(file)]=file;Save();if(progress.cancel)throw std::runtime_error("Cancelled");return path;
  }
private:
  std::string root;Fetcher fetch;bool offline=false;std::map<std::uint64_t,Game> games;std::map<std::uint64_t,std::vector<File>> files;std::map<std::string,File> installed;
  Json Get(const std::string &path,Progress &progress){if(progress.cancel)throw std::runtime_error("Cancelled");auto bytes=fetch("https://zxart.ee/api/"+path,8*1024*1024,progress);auto response=Json::parse(bytes);
    if(!response.is_object() || response.value("responseStatus",std::string())!="success")throw std::runtime_error("ZXArt catalog request failed");return response;}
  static void Check(const File &file,const Bytes &bytes){if(file.size && bytes.size()!=file.size)throw std::runtime_error("Download size does not match ZXArt metadata");if(!file.md5.empty() && MD5(bytes)!=file.md5)throw std::runtime_error("Download checksum does not match ZXArt metadata");Validate(bytes,file.type);}
  void Load(){try{auto bytes=Read(root+"/catalog.json",32*1024*1024);auto data=Json::parse(bytes);if(data.value("version",0)!=1)return;offline=data.value("offline",false);
    if(data.contains("games") && data["games"].is_array() && data["games"].size()<=100000)for(const auto &item:data["games"]){auto game=DecodeGame(item);games[game.id]=game;}
    if(data.contains("files") && data["files"].is_array() && data["files"].size()<=200000)for(const auto &item:data["files"]){auto file=DecodeFile(item);files[file.game].push_back(file);}
    if(data.contains("installed") && data["installed"].is_array() && data["installed"].size()<=100000)for(const auto &item:data["installed"]){auto file=DecodeFile(item);installed[Key(file)]=file;}
  }catch(const std::exception &){/* A missing/broken catalog must not prevent local file loading. */}}
  void Save(){if(!al_make_directory(root.c_str()))throw std::runtime_error("Cannot create games directory");Json data={{"version",1},{"offline",offline},{"games",Json::array()},{"files",Json::array()},{"installed",Json::array()}};
    for(const auto &g:games)data["games"].push_back(Encode(g.second));for(const auto &group:files)for(const auto &f:group.second)data["files"].push_back(Encode(f));for(const auto &f:installed)data["installed"].push_back(Encode(f.second));
    auto text=data.dump();AtomicWrite(root+"/catalog.json",Bytes(text.begin(),text.end()));}
};
}
#endif
