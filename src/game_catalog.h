#ifndef GAME_CATALOG_H
#define GAME_CATALOG_H
#include <allegro5/allegro.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <atomic>
#include <array>
#include <cerrno>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "platform.h"
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
  char details[CURL_ERROR_SIZE] = {};
  curl_easy_setopt(curl.get(),CURLOPT_ERRORBUFFER,details);
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
#ifdef _WIN32
  // MSYS2's compiled-in certificate paths do not exist on a normal Windows
  // installation. Use Windows trust without weakening peer/hostname checks.
  curl_easy_setopt(curl.get(),CURLOPT_SSL_OPTIONS,long(CURLSSLOPT_NATIVE_CA));
  curl_easy_setopt(curl.get(),CURLOPT_CAINFO,static_cast<const char *>(nullptr));
  curl_easy_setopt(curl.get(),CURLOPT_CAPATH,static_cast<const char *>(nullptr));
  const std::string certificates=ROMDirectory(Platform::Executable())+"curl-ca-bundle.crt";
  std::unique_ptr<ALLEGRO_FILE,decltype(&al_fclose)> pem(al_fopen(certificates.c_str(),"rb"),al_fclose);
  if(pem) {
    const auto size=al_fsize(pem.get());
    if(size>0 && size<=4*1024*1024) {
      std::vector<char> bytes(static_cast<size_t>(size));
      if(al_fread(pem.get(),bytes.data(),bytes.size())==bytes.size()) {
        // Read through Allegro's UTF-8 file API, so accented Windows paths do
        // not depend on OpenSSL's narrow-path fopen implementation.
        curl_blob blob={bytes.data(),bytes.size(),CURL_BLOB_COPY};
        curl_easy_setopt(curl.get(),CURLOPT_CAINFO_BLOB,&blob);
      }
    }
  }
#endif
  curl_easy_setopt(curl.get(),CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(curl.get(),CURLOPT_MAXREDIRS,4L);curl_easy_setopt(curl.get(),CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(curl.get(),CURLOPT_TIMEOUT,40L);
  curl_easy_setopt(curl.get(),CURLOPT_LOW_SPEED_LIMIT,128L);curl_easy_setopt(curl.get(),CURLOPT_LOW_SPEED_TIME,15L);curl_easy_setopt(curl.get(),CURLOPT_NOSIGNAL,1L);
  curl_easy_setopt(curl.get(),CURLOPT_USERAGENT,"Gomas/1.0 (ZX Spectrum emulator; multi-source game catalog)");curl_easy_setopt(curl.get(),CURLOPT_ACCEPT_ENCODING,"");
  curl_easy_setopt(curl.get(),CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(curl.get(),CURLOPT_WRITEDATA,&transfer);curl_easy_setopt(curl.get(),CURLOPT_XFERINFOFUNCTION,update);curl_easy_setopt(curl.get(),CURLOPT_XFERINFODATA,&progress);curl_easy_setopt(curl.get(),CURLOPT_NOPROGRESS,0L);
  CURLcode result=curl_easy_perform(curl.get());long status=0;curl_easy_getinfo(curl.get(),CURLINFO_RESPONSE_CODE,&status);
  if(progress.cancel)throw std::runtime_error("Cancelled");if(transfer.oversized)throw std::runtime_error("Download exceeds the supported size limit");
  if(result!=CURLE_OK)throw std::runtime_error(std::string("Network error: ")+(details[0]?details:curl_easy_strerror(result)));
  if(status!=200)throw std::runtime_error("Catalog server returned HTTP "+std::to_string(status));return transfer.bytes;
}
inline std::string MD5(const Bytes &bytes){unsigned char digest[EVP_MAX_MD_SIZE];unsigned size=0;
  if(!EVP_Digest(bytes.data(),bytes.size(),digest,&size,EVP_md5(),nullptr))throw std::runtime_error("Cannot verify download checksum");
  static const char hex[]="0123456789abcdef";std::string out;for(unsigned n=0;n<size;++n){out+=hex[digest[n]>>4];out+=hex[digest[n]&15];}return out;}
inline Bytes Read(const std::string &path,size_t limit){std::ifstream file(path,std::ios::binary|std::ios::ate);if(!file)throw std::runtime_error("Cached file is missing");auto size=file.tellg();
  if(size<0 || size>std::streamoff(limit))throw std::runtime_error("Cached file is too large");Bytes bytes(static_cast<size_t>(size));file.seekg(0);if(!bytes.empty())file.read(reinterpret_cast<char *>(bytes.data()),bytes.size());if(!file)throw std::runtime_error("Cannot read cached file");return bytes;}
inline void AtomicWrite(const std::string &path,const Bytes &bytes){std::string pattern=path+".part-XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
  int fd=Platform::Temporary(name.data());if(fd<0)throw std::runtime_error("Cannot create game cache file");std::string temporary=name.data();FILE *file=Platform::File(fd,"wb");
  if(!file){Platform::Close(fd);remove(temporary.c_str());throw std::runtime_error("Cannot write game cache file");}
  bool ok=fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();ok=(fflush(file)==0)&&ok;ok=(Platform::Sync(Platform::Descriptor(file))==0)&&ok;ok=(fclose(file)==0)&&ok;
  if(!ok || Platform::Replace(temporary,path)){remove(temporary.c_str());throw std::runtime_error("Cannot replace game cache file");}}
inline std::string Directory(const std::string &executable){if(const char *path=getenv("GOMAS_GAMES_DIR"))return path;
  auto dir=ROMDirectory(executable);if(al_filename_exists((dir+"../roms").c_str()) && Platform::Writable(dir+".."))return dir+"../games";
  std::unique_ptr<ALLEGRO_PATH,decltype(&al_destroy_path)> path(al_get_standard_path(ALLEGRO_USER_DATA_PATH),al_destroy_path);
  if(!path)return "games";al_append_path_component(path.get(),"games");return al_path_cstr(path.get(),ALLEGRO_NATIVE_PATH_SEP);}
enum class Source { All, ZXArt, ZXInfo, Itch };
inline const char *SourceName(Source source){return source==Source::All?"All sources":source==Source::ZXArt?"ZXArt":source==Source::ZXInfo?"ZXInfo / ZXDB":"itch.io discovery";}
inline std::string Provider(Source source){return source==Source::ZXInfo?"zxinfo":source==Source::Itch?"itch":"zxart";}
struct Game {std::uint64_t id=0;std::string title,year,status;std::vector<std::string> images;std::string provider="zxart",pageURL,price;std::uint64_t zxdb=0;};
struct File {std::uint64_t game=0,release=0,id=0;std::string name,type,year,publisher,releaseType,md5;size_t size=0;MachineModel model=MachineModel::Spectrum48;bool supported=true;std::string provider="zxart",url,origin;std::vector<std::string> mirrors;};
struct Page {std::vector<Game> games;size_t total=0;unsigned page=0;bool estimated=false;std::string warning;};
constexpr unsigned PageSize=30;
inline Json Encode(const Game &g){return Json{{"id",g.id},{"title",g.title},{"year",g.year},{"status",g.status},{"images",g.images},{"provider",g.provider},{"zxdb",g.zxdb},{"pageURL",g.pageURL},{"price",g.price}};}
inline Json Encode(const File &f){return Json{{"game",f.game},{"release",f.release},{"id",f.id},{"name",f.name},{"type",f.type},{"year",f.year},{"publisher",f.publisher},{"releaseType",f.releaseType},{"md5",f.md5},{"size",f.size},{"model",unsigned(f.model)},{"supported",f.supported},{"provider",f.provider},{"url",f.url},{"origin",f.origin},{"mirrors",f.mirrors}};}
inline std::string URLHost(const std::string &url){if(url.find("https://")!=0 || url.size()>4096)return "";auto end=url.find('/',8);auto host=Lower(url.substr(8,end==std::string::npos?end:end-8));if(host.find_first_of("@:?#\\")!=std::string::npos)return "";return host;}
inline bool ItchPage(const std::string &url){auto host=URLHost(url);return host.size()>8 && host.substr(host.size()-8)==".itch.io";}
inline bool CatalogURL(const std::string &url){auto host=URLHost(url);return host=="zxart.ee" || host=="api.zxinfo.dk" || host=="zxinfo.dk" || host=="spectrumcomputing.co.uk" || host=="worldofspectrum.net" || host=="www.worldofspectrum.net" || host=="worldofspectrum.org" || host=="archive.org";}
inline std::uint64_t NumericID(const Json &object,const char *key){auto value=Text(object,key);if(value.empty() || value.size()>9 || value.find_first_not_of("0123456789")!=std::string::npos)return 0;return std::strtoull(value.c_str(),nullptr,10);}
inline std::uint64_t URLId(const std::string &url){Bytes bytes(url.begin(),url.end());return std::strtoull(MD5(bytes).substr(0,12).c_str(),nullptr,16)+1;}
inline bool ArtworkURL(const std::string &url) {
  return url.size()<4096 && (CatalogURL(url) || URLHost(url)=="img.itch.zone");
}
inline void ReadImages(Game &game,const Json &j,const char *key) {
  auto it=j.find(key);if(it==j.end() || !it->is_array())return;
  for(const auto &item:*it)if(item.is_string() && game.images.size()<4){auto url=item.get<std::string>();
    if(ArtworkURL(url) && std::find(game.images.begin(),game.images.end(),url)==game.images.end())game.images.push_back(url);}
}
inline Game DecodeGame(const Json &j){Game g;g.id=Id(j,"id");g.title=Text(j,"title");g.year=Text(j,"year");g.status=Text(j,"status");g.provider=j.value("provider",std::string("zxart"));if(g.provider!="zxart" && g.provider!="zxinfo" && g.provider!="itch")throw std::runtime_error("Invalid catalog provider");g.zxdb=j.value("zxdb",std::uint64_t(0));g.pageURL=Text(j,"pageURL");g.price=Text(j,"price");if(!g.pageURL.empty() && !CatalogURL(g.pageURL) && !ItchPage(g.pageURL))g.pageURL.clear();ReadImages(g,j,"images");if(g.title.empty())throw std::runtime_error("Catalog game has no title");return g;}
// Reject oversized canvases before handing image data to a decoder.
inline std::string ArtworkType(const Bytes &bytes) {
  unsigned w=0,h=0;std::string type;
  if(bytes.size()>=24 && !memcmp(bytes.data(),"\x89PNG\r\n\x1a\n",8) && !memcmp(bytes.data()+12,"IHDR",4)) {
    auto big=[&](unsigned n){return unsigned(bytes[n])<<24 | unsigned(bytes[n+1])<<16 | unsigned(bytes[n+2])<<8 | bytes[n+3];};
    w=big(16);h=big(20);type="png";
  }else if(bytes.size()>=13 && (!memcmp(bytes.data(),"GIF87a",6) || !memcmp(bytes.data(),"GIF89a",6))) {
    w=bytes[6]|unsigned(bytes[7])<<8;h=bytes[8]|unsigned(bytes[9])<<8;type="gif";
  }
  if(type.empty() && bytes.size()>=4 && bytes[0]==0xff && bytes[1]==0xd8) {
    size_t pos=2;
    while(pos+4<=bytes.size()) {
      if(bytes[pos++]!=0xff)break;
      while(pos<bytes.size() && bytes[pos]==0xff)++pos;
      if(pos>=bytes.size())break;unsigned marker=bytes[pos++];
      if(marker==0xd9 || marker==0xda)break;
      if(marker==1 || (marker>=0xd0 && marker<=0xd8))continue;
      if(pos+2>bytes.size())break;size_t length=unsigned(bytes[pos])*256+bytes[pos+1];
      if(length<2 || length>bytes.size()-pos)break;
      bool sof=marker>=0xc0 && marker<=0xcf && marker!=0xc4 && marker!=0xc8 && marker!=0xcc;
      if(sof && length>=7){h=unsigned(bytes[pos+3])*256+bytes[pos+4];w=unsigned(bytes[pos+5])*256+bytes[pos+6];type="jpg";break;}
      pos+=length;
    }
  }
  if(type.empty() && bytes.size()>=30 && !memcmp(bytes.data(),"RIFF",4) && !memcmp(bytes.data()+8,"WEBP",4)) {
    if(!memcmp(bytes.data()+12,"VP8X",4)){w=1+bytes[24]+(unsigned(bytes[25])<<8)+(unsigned(bytes[26])<<16);h=1+bytes[27]+(unsigned(bytes[28])<<8)+(unsigned(bytes[29])<<16);type="webp";}
    else if(!memcmp(bytes.data()+12,"VP8 ",4) && bytes[23]==0x9d && bytes[24]==1 && bytes[25]==0x2a){w=(bytes[26]|unsigned(bytes[27])<<8)&0x3fff;h=(bytes[28]|unsigned(bytes[29])<<8)&0x3fff;type="webp";}
    else if(!memcmp(bytes.data()+12,"VP8L",4) && bytes[20]==0x2f){w=1+bytes[21]+((unsigned(bytes[22])&0x3f)<<8);h=1+(bytes[22]>>6)+(unsigned(bytes[23])<<2)+((unsigned(bytes[24])&15)<<10);type="webp";}
  }
  if(!w || !h || w>2048 || h>2048 || bytes.size()>4*1024*1024)throw std::runtime_error("Unsupported or oversized catalog artwork");
  return type;
}

inline bool Format(const std::string &type){return type=="tap" || type=="tzx" || type=="sna" || type=="z80" || type=="szx" || type=="slt" || type=="dsk" || type=="zip";}
inline File DecodeFile(const Json &j){File f;f.game=Id(j,"game");f.release=Id(j,"release");f.id=Id(j,"id");f.name=Text(j,"name");f.type=Lower(Text(j,"type"));f.year=Text(j,"year");f.publisher=Text(j,"publisher");f.releaseType=Text(j,"releaseType");f.md5=Lower(Text(j,"md5"));
  f.provider=j.value("provider",std::string("zxart"));f.url=Text(j,"url");f.origin=Text(j,"origin");
  if(f.provider!="zxart" && f.provider!="zxinfo")throw std::runtime_error("Invalid download provider");
  if(f.provider=="zxinfo" && !CatalogURL(f.url))throw std::runtime_error("Invalid download URL");
  if(j.contains("mirrors") && j["mirrors"].is_array())for(const auto &url:j["mirrors"])if(url.is_string() && CatalogURL(url.get<std::string>()) && f.mirrors.size()<3)f.mirrors.push_back(url.get<std::string>());
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
}
#include "catalog_sources.h"
namespace GameCatalog {
class Library {
public:
  explicit Library(std::string root,Fetcher fetch=Http):root(std::move(root)),fetch(std::move(fetch)){Load();}
  Source SelectedSource() const{return selectedSource;}
  void SetSource(Source source){selectedSource=source;Save();}
  bool Offline() const{return offline;}
  void SetOffline(bool enabled){offline=enabled;Save();}
  std::string Root() const{return root;}
  std::string Path(const File &f) const{return root+"/"+f.provider+"/"+std::to_string(f.game)+"/"+std::to_string(f.release)+"-"+std::to_string(f.id)+"/"+SafeName(f);}
  std::string Key(const File &f) const{return (f.provider=="zxart"?"":f.provider+":")+std::to_string(f.release)+"-"+std::to_string(f.id);}
  bool Installed(const File &f) const{auto found=installed.find(Key(f));return found!=installed.end() && al_filename_exists(Path(found->second).c_str());}
  std::set<std::uint64_t> InstalledGames() const{std::set<std::uint64_t> out;for(const auto &game:games)if(Installed(game.second))out.insert(game.first);for(const auto &entry:installed)if(al_filename_exists(Path(entry.second).c_str()))out.insert(entry.second.game);return out;}
  std::set<std::string> InstalledFiles() const{std::set<std::string> out;for(const auto &entry:installed)if(al_filename_exists(Path(entry.second).c_str()))out.insert(entry.first);return out;}
  bool Installed(const Game &g) const{for(const auto &item:installed)if(SameGame(g,item.second.game) && al_filename_exists(Path(item.second).c_str()))return true;return false;}
  Page Cached(const std::string &query,unsigned page,bool descending,bool onlyInstalled,Source source=Source::All) const {
    Page out;out.page=page;auto needle=Lower(query);
    for(const auto &item:games)if((onlyInstalled || source==Source::All || item.second.provider==Provider(source)) && Lower(item.second.title).find(needle)!=std::string::npos && (!onlyInstalled || Installed(item.second)))out.games.push_back(item.second);
    if(source==Source::All || onlyInstalled)out.games=MergeGames(out.games);
    std::sort(out.games.begin(),out.games.end(),[&](const Game &a,const Game &b){return descending?Lower(a.title)>Lower(b.title):Lower(a.title)<Lower(b.title);});out.total=out.games.size();
    size_t start=size_t(page)*PageSize;if(start>=out.games.size())out.games.clear();else{auto end=std::min(out.games.size(),start+PageSize);out.games=std::vector<Game>(out.games.begin()+start,out.games.begin()+end);}return out;
  }
  std::vector<File> CachedFiles(const Game &game,Source source=Source::All) const {
    std::map<std::string,File> combined;
    for(const auto &group:files)if(SameGame(game,group.first))for(const auto &file:group.second)
      if(source==Source::All || file.provider==Provider(source))combined[Key(file)]=file;
    for(const auto &entry:installed)if(SameGame(game,entry.second.game) && Installed(entry.second) &&
        (source==Source::All || entry.second.provider==Provider(source)))combined[entry.first]=entry.second;
    std::vector<File> out;for(const auto &entry:combined)out.push_back(entry.second);OrderFiles(out);return out;
  }
  Page Search(const std::string &query,unsigned page,bool descending,Progress &progress,Source source=Source::ZXArt) {
    if(progress.cancel)throw std::runtime_error("Cancelled");
    if(query.size()>200 || page>10000)throw std::runtime_error("Search is too large");
    if(source==Source::ZXArt)return SearchZXArt(query,page,descending,progress);
    if(source==Source::ZXInfo){auto result=FetchZXInfo(fetch,query,page,descending,progress);for(const auto &game:result.games)StoreGame(game);Save();return result;}
    if(source==Source::Itch) {
      if(!itchLoaded || al_get_time()-itchTime>300){itchFeed=ParseItchFeed(fetch("https://itch.io/games/tag-zx-spectrum.xml",4*1024*1024,progress));itchLoaded=true;itchTime=al_get_time();}
      const auto &found=itchFeed;
      for(const auto &game:found)StoreGame(game);Save();
      Page result;result.page=page;result.warning="itch.io discovery feed: a featured selection, not the complete store. Open a game page for downloads.";
      auto term=Lower(query);for(const auto &game:found)if(Lower(game.title).find(term)!=std::string::npos)result.games.push_back(game);
      SortGames(result.games,descending);result.total=result.games.size();Slice(result.games,page);return result;
    }
    // Grow sorted provider prefixes lazily. Merging full prefixes, rather than
    // independent page N results, preserves global A-Z pagination.
    const auto key=query+"\n"+(descending?"desc":"asc");
    if(federatedKey!=key || page==0){federatedKey=key;federated=Federated();}
    const size_t needed=(size_t(page)+1)*PageSize+1;std::vector<Game> combined;std::string warning;
    bool any=false;size_t totals=0;bool unfinished=false;
    for(unsigned n=0;n<3;++n) {
      auto provider=static_cast<Source>(n+1);auto &stream=federated.streams[n];
      try {
        while(!stream.done && stream.games.size()<needed) {
          if(progress.cancel)throw std::runtime_error("Cancelled");
          auto part=Search(query,stream.next,descending,progress,provider);++stream.next;
          stream.total=part.total;stream.games.insert(stream.games.end(),part.games.begin(),part.games.end());
          stream.done=size_t(stream.next)*PageSize>=part.total;
        }
      }catch(const std::exception &e){if(progress.cancel)throw;stream.done=true;stream.error=std::string(SourceName(provider))+": "+e.what();}
      if(!stream.error.empty())warning+=(warning.empty()?"":" | ")+stream.error;
      if(!stream.games.empty())any=true;
      totals+=stream.total;unfinished|=!stream.done;
      combined.insert(combined.end(),stream.games.begin(),stream.games.end());
    }
    if(!any && !warning.empty())throw std::runtime_error(warning);
    Page result;result.page=page;result.games=MergeGames(combined);SortGames(result.games,descending);
    result.total=unfinished?totals:result.games.size();result.estimated=unfinished;result.warning=warning;
    if(!unfinished && size_t(result.page)*PageSize>=result.total)result.page=result.total?unsigned((result.total-1)/PageSize):0;
    Slice(result.games,result.page);return result;
  }
  std::vector<File> Versions(const Game &game,Progress &progress,Source source=Source::All,std::string *warning=nullptr) {
    if(game.provider=="itch")return {};
    std::vector<File> out;std::string errors;bool attempted=false,success=false;
    Game art=game;bool haveArt=game.provider=="zxart";
    if(!haveArt && game.zxdb)for(const auto &known:games)if(known.second.provider=="zxart" && known.second.zxdb==game.zxdb){art=known.second;haveArt=true;break;}
    if(haveArt && (source==Source::All || source==Source::ZXArt)) {
      attempted=true;try{auto list=VersionsZXArt(art,progress);out.insert(out.end(),list.begin(),list.end());success=true;}
      catch(const std::exception &e){if(progress.cancel)throw;errors="ZXArt: "+std::string(e.what());}
    }
    if(game.zxdb && (source==Source::All || source==Source::ZXInfo)) {
      attempted=true;try {
        auto data=FetchJSON(fetch,"https://api.zxinfo.dk/v5/entries/"+std::to_string(game.zxdb)+"?mode=full",progress);
        Game info=ZXInfoGame(data);auto list=ZXInfoFiles(info,data);StoreGame(info);files[info.id]=list;Save();
        out.insert(out.end(),list.begin(),list.end());success=true;
      }catch(const std::exception &e){if(progress.cancel)throw;errors+=(errors.empty()?"":" | ")+std::string("ZXInfo: ")+e.what();}
    }
    if(warning)*warning=errors;
    if(attempted && !success){out=CachedFiles(game,source);if(out.empty())throw std::runtime_error(errors);}
    OrderFiles(out);return out;
  }
  Page SearchZXArt(const std::string &query,unsigned page,bool descending,Progress &progress) {
    if(query.size()>200 || page>100000)throw std::runtime_error("Search is too large");
    std::string term=query;for(auto &c:term)if(c==';' || c=='/' || c==':' || c=='=')c=' ';
    auto response=Get("export:zxProd/language:eng/start:"+std::to_string(page*PageSize)+"/limit:"+std::to_string(PageSize)+"/order:title,"+(descending?"desc":"asc")+"/filter:zxProdCategory=92177;zxProdStatus=allowed,allowedzxart,unknown,recovered"+(query.empty()?"":";zxProdAjaxSearch="+Escape(term))+"/",progress);
    Page out;out.page=page;out.total=response.value("totalAmount",size_t(0));const auto &rows=response.at("responseData").at("zxProd");if(!rows.is_array() || rows.size()>PageSize)throw std::runtime_error("Invalid catalog page");
    for(const auto &row:rows){Game g;g.id=Id(row,"id");g.title=Text(row,"title");g.year=Text(row,"year");g.status=Text(row,"legalStatus");if(row.contains("importIds") && row["importIds"].is_object())g.zxdb=NumericID(row["importIds"],"zxdb");ReadImages(g,row,"imagesUrls");if(g.title.empty())continue;out.games.push_back(g);StoreGame(g);}
    Save();return out;
  }
  std::vector<File> VersionsZXArt(const Game &game,Progress &progress) {
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
    StoreGame(game);files[game.id]=out;Save();return out;
  }
  std::vector<std::string> Artwork(const Game &game,bool localOnly,Progress &progress) {
    std::vector<std::string> paths;Game picture=game;
    for(const auto &known:games)if(SameGame(game,known.first))for(const auto &url:known.second.images)AddImage(picture,url);
    for(const auto &url:picture.images) {
      if(progress.cancel)throw std::runtime_error("Cancelled");
      if(!ArtworkURL(url))continue;
      Bytes key(url.begin(),url.end());auto base=root+"/artwork/"+MD5(key);
      bool found=false;
      for(const char *type:{"png","gif","jpg","webp"})try{auto path=base+"."+type;auto bytes=Read(path,4*1024*1024);
        if(ArtworkType(bytes)==type){paths.push_back(path);found=true;break;}
      }catch(const std::exception &){}
      if(found || localOnly)continue;
      try {
        auto bytes=fetch(url,4*1024*1024,progress);auto type=ArtworkType(bytes);
        if(progress.cancel)throw std::runtime_error("Cancelled");
        if(!al_make_directory((root+"/artwork").c_str()))throw std::runtime_error("Cannot create artwork cache");
        auto path=base+"."+type;AtomicWrite(path,bytes);paths.push_back(path);
      }catch(const std::exception &){if(progress.cancel)throw; /* Artwork never prevents game loading. */}
    }
    return paths;
  }
  void Uninstall(const File &file) {
    auto found=installed.find(Key(file));if(found==installed.end())return;
    // Only remove the file recorded by this library, never a caller's path.
    const File saved=found->second;const auto path=Path(saved);
    std::string temporary;
    if(al_filename_exists(path.c_str())) {
      std::string pattern=path+".uninstall-XXXXXX";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);
      int fd=Platform::Temporary(name.data());if(fd<0)throw std::runtime_error("Cannot uninstall game file");Platform::Close(fd);temporary=name.data();
      if(Platform::Replace(path,temporary)){remove(temporary.c_str());throw std::runtime_error("Cannot uninstall game file");}
    }
    installed.erase(found);
    try{Save();}catch(...){installed[Key(saved)]=saved;if(!temporary.empty())Platform::Replace(temporary,path);throw;}
    if(!temporary.empty() && remove(temporary.c_str()))throw std::runtime_error("Game uninstalled, but its temporary file could not be removed");
    Platform::RemoveDirectory(ROMDirectory(path).c_str());Platform::RemoveDirectory((root+"/"+saved.provider+"/"+std::to_string(saved.game)).c_str());
  }
  std::string Install(const File &file,bool localOnly,Progress &progress) {
    if(progress.cancel)throw std::runtime_error("Cancelled");
    if(!file.supported)throw std::runtime_error("This version requires hardware not supported by Gomas");
    auto path=Path(file);
    if(Installed(file))try{auto existing=Path(installed.at(Key(file)));auto bytes=Read(existing,TapeFiles::MaxBytes);Check(file,bytes);if(progress.cancel)throw std::runtime_error("Cancelled");return existing;}catch(const std::exception &){if(localOnly)throw std::runtime_error("Installed file is missing or damaged; go online to download it again");}
    if(localOnly)throw std::runtime_error("This version is not installed. Switch off Offline to download it");
    if(progress.cancel)throw std::runtime_error("Cancelled");
    std::vector<std::string> urls={file.provider=="zxart"?"https://zxart.ee/zxfile/id:"+std::to_string(file.release)+"/fileId:"+std::to_string(file.id)+"/"+Escape(file.name):file.url};
    urls.insert(urls.end(),file.mirrors.begin(),file.mirrors.end());Bytes bytes;std::string lastError;
    for(const auto &url:urls)try{if(!CatalogURL(url))throw std::runtime_error("Unsupported catalog download host");bytes=fetch(url,TapeFiles::MaxBytes,progress);Check(file,bytes);lastError.clear();break;}
      catch(const std::exception &e){if(progress.cancel)throw;lastError=e.what();bytes.clear();}
    if(!lastError.empty())throw std::runtime_error(lastError);
    if(progress.cancel)throw std::runtime_error("Cancelled");
    if(!al_make_directory(ROMDirectory(path).c_str()))throw std::runtime_error("Cannot create games directory");
    if(file.type=="zip") {
      std::string pattern=path+".part-XXXXXX.zip";std::vector<char> name(pattern.begin(),pattern.end());name.push_back(0);int fd=Platform::Temporary(name.data(),4);
      if(fd<0)throw std::runtime_error("Cannot stage ZIP download");Platform::Close(fd);std::string temporary=name.data();
      try{AtomicWrite(temporary,bytes);Bytes media;std::string name,error;if(!TapeFiles::ReadMedia(temporary,media,name,error))throw std::runtime_error(error);
        auto dot=name.find_last_of('.');Validate(media,dot==std::string::npos?"":Lower(name.substr(dot+1)));
        if(progress.cancel)throw std::runtime_error("Cancelled");if(Platform::Replace(temporary,path))throw std::runtime_error("Cannot install ZIP download");
      }catch(...){remove(temporary.c_str());throw;}
    } else AtomicWrite(path,bytes);
    installed[Key(file)]=file;Save();if(progress.cancel)throw std::runtime_error("Cancelled");return path;
  }
private:
  std::string root;Fetcher fetch;bool offline=false;Source selectedSource=Source::ZXArt;std::map<std::uint64_t,Game> games;std::map<std::uint64_t,std::vector<File>> files;std::map<std::string,File> installed;
  struct Stream {std::vector<Game> games;unsigned next=0;size_t total=0;bool done=false;std::string error;};
  struct Federated {std::array<Stream,3> streams;};
  Federated federated;std::string federatedKey;
  std::vector<Game> itchFeed;bool itchLoaded=false;double itchTime=0;
  static void SortGames(std::vector<Game> &list,bool descending){std::stable_sort(list.begin(),list.end(),[&](const Game &a,const Game &b){return descending?Lower(a.title)>Lower(b.title):Lower(a.title)<Lower(b.title);});}
  static void Slice(std::vector<Game> &list,unsigned page){size_t start=size_t(page)*PageSize;if(start>=list.size())list.clear();else{size_t end=std::min(list.size(),start+PageSize);list=std::vector<Game>(list.begin()+start,list.begin()+end);}}
  void StoreGame(Game game) {
    auto previous=games.find(game.id);
    if(previous!=games.end()) {
      if(!game.zxdb)game.zxdb=previous->second.zxdb;
      if(game.pageURL.empty())game.pageURL=previous->second.pageURL;
      for(const auto &url:previous->second.images)AddImage(game,url);
    }
    games[game.id]=std::move(game);
  }
  bool SameGame(const Game &game,std::uint64_t id) const {
    if(game.id==id)return true;
    auto found=games.find(id);return game.zxdb && found!=games.end() && found->second.zxdb==game.zxdb;
  }
  Json Get(const std::string &path,Progress &progress){if(progress.cancel)throw std::runtime_error("Cancelled");auto bytes=fetch("https://zxart.ee/api/"+path,8*1024*1024,progress);auto response=Json::parse(bytes);
    if(!response.is_object() || response.value("responseStatus",std::string())!="success")throw std::runtime_error("ZXArt catalog request failed");return response;}
  static void Check(const File &file,const Bytes &bytes){if(file.size && bytes.size()!=file.size)throw std::runtime_error("Download size does not match catalog metadata");if(!file.md5.empty() && MD5(bytes)!=file.md5)throw std::runtime_error("Download checksum does not match catalog metadata");Validate(bytes,file.type);}
  void Load(){try{auto bytes=Read(root+"/catalog.json",32*1024*1024);auto data=Json::parse(bytes);if(data.value("version",0)!=1)return;offline=data.value("offline",false);unsigned source=data.value("source",1u);if(source<=3)selectedSource=static_cast<Source>(source);
    if(data.contains("games") && data["games"].is_array() && data["games"].size()<=100000)for(const auto &item:data["games"]){auto game=DecodeGame(item);StoreGame(game);}
    if(data.contains("files") && data["files"].is_array() && data["files"].size()<=200000)for(const auto &item:data["files"]){auto file=DecodeFile(item);files[file.game].push_back(file);}
    if(data.contains("installed") && data["installed"].is_array() && data["installed"].size()<=100000)for(const auto &item:data["installed"]){auto file=DecodeFile(item);installed[Key(file)]=file;}
  }catch(const std::exception &){/* A missing/broken catalog must not prevent local file loading. */}}
  void Save(){if(!al_make_directory(root.c_str()))throw std::runtime_error("Cannot create games directory");Json data={{"version",1},{"offline",offline},{"source",unsigned(selectedSource)},{"games",Json::array()},{"files",Json::array()},{"installed",Json::array()}};
    for(const auto &g:games)data["games"].push_back(Encode(g.second));for(const auto &group:files)for(const auto &f:group.second)data["files"].push_back(Encode(f));for(const auto &f:installed)data["installed"].push_back(Encode(f.second));
    auto text=data.dump();AtomicWrite(root+"/catalog.json",Bytes(text.begin(),text.end()));}
};
}
#endif
