#ifndef CATALOG_SOURCES_H
#define CATALOG_SOURCES_H
// Included by game_catalog.h after the shared catalog types.
#include <expat.h>
#include <cstring>
#include <map>
#include <type_traits>
namespace GameCatalog {
constexpr std::uint64_t ZXInfoID=std::uint64_t(1)<<48,ItchID=std::uint64_t(1)<<49;
inline std::string EncodePath(const std::string &path){std::string out;for(unsigned char c:path){if(c=='/')out+='/';else out+=Escape(std::string(1,char(c)));}return out;}
inline std::string ArchiveURL(const std::string &path){return "https://archive.org/download/World_of_Spectrum_June_2017_Mirror/World%20of%20Spectrum%20June%202017%20Mirror.zip/World%20of%20Spectrum%20June%202017%20Mirror/"+Escape(path.substr(5));}
inline std::string ZXResource(const std::string &path) {
  if(path.find("/pub/")==0)return "https://worldofspectrum.net"+EncodePath(path);
  if(path.find("/zxdb/")==0)return "https://spectrumcomputing.co.uk"+EncodePath(path);
  if(path.find("/zxscreens/")==0)return "https://zxinfo.dk/media"+EncodePath(path);
  if(CatalogURL(path))return path;
  return "";
}
inline bool SpectrumSupported(const std::string &machine) {
  auto text=Lower(machine);return text.find("zx-spectrum")!=std::string::npos && text.find("next")==std::string::npos && text.find("pentagon")==std::string::npos;
}
inline MachineModel SpectrumModel(const std::string &machine){auto text=Lower(machine);return text.find("+3")!=std::string::npos?MachineModel::Plus3:text.find("+2")!=std::string::npos?MachineModel::Plus2:text.find("128")!=std::string::npos?MachineModel::Spectrum128:MachineModel::Spectrum48;}
inline void AddImage(Game &game,const std::string &url){if(game.images.size()<4 && ArtworkURL(url) && std::find(game.images.begin(),game.images.end(),url)==game.images.end())game.images.push_back(url);}
inline Game ZXInfoGame(const Json &hit) {
  const auto &row=hit.at("_source");Game game;game.zxdb=NumericID(hit,"_id");if(!game.zxdb)throw std::runtime_error("Invalid ZXInfo identifier");
  game.id=ZXInfoID+game.zxdb;game.provider="zxinfo";game.title=Text(row,"title");game.year=Text(row,"originalYearOfRelease");game.status=Text(row,"availability");
  game.pageURL="https://zxinfo.dk/details/"+std::to_string(game.zxdb);
  if(row.contains("screens") && row["screens"].is_array())for(const auto &screen:row["screens"])AddImage(game,ZXResource(Text(screen,"url")));
  if(row.contains("additionalDownloads") && row["additionalDownloads"].is_array())for(const auto &item:row["additionalDownloads"])
    if(Text(item,"type").find("Inlay")==0)AddImage(game,ZXResource(Text(item,"path")));
  if(game.title.empty())throw std::runtime_error("ZXInfo game has no title");return game;
}
inline Json FetchJSON(const Fetcher &fetch,const std::string &url,Progress &progress) {
  if(progress.cancel)throw std::runtime_error("Cancelled");auto bytes=fetch(url,8*1024*1024,progress);try{return Json::parse(bytes);}catch(const Json::parse_error &){throw std::runtime_error("Catalog server returned an invalid response (expected JSON). Please retry later.");}
}
inline Page FetchZXInfo(const Fetcher &fetch,const std::string &query,unsigned page,bool descending,Progress &progress) {
  auto data=FetchJSON(fetch,"https://api.zxinfo.dk/v5/search"+(query.empty()?"/":"/titles/"+Escape(query))+"?size=30&offset="+std::to_string(page)+"&sort="+(descending?"title_desc":"title_asc")+"&mode=compact&contenttype=SOFTWARE&machinetype=ZXSPECTRUM&genretype=GAMES",progress);
  const auto &hits=data.at("hits");const auto &rows=hits.at("hits");if(!rows.is_array() || rows.size()>PageSize)throw std::runtime_error("Invalid ZXInfo page");
  Page out;out.page=page;const auto &total=hits.at("total");out.total=total.is_object()?total.value("value",size_t(0)):total.get<size_t>();
  out.estimated=total.is_object() && total.value("relation",std::string("eq"))!="eq";
  for(const auto &hit:rows)if(SpectrumSupported(Text(hit.at("_source"),"machineType")))out.games.push_back(ZXInfoGame(hit));return out;
}
inline std::vector<File> ZXInfoFiles(const Game &game,const Json &detail) {
  const auto &row=detail.at("_source");std::vector<File> out;
  auto availability=Lower(Text(row,"availability"));
  if(availability.find("denied")!=std::string::npos || availability.find("restricted")!=std::string::npos)return out;
  if(!row.contains("releases") || !row["releases"].is_array())return out;
  for(const auto &release:row["releases"]) {
    if(!release.contains("files") || !release["files"].is_array())continue;
    for(const auto &entry:release["files"]) {
      std::string path=Text(entry,"path"),url=ZXResource(path);if(url.empty())continue;
      auto dot=path.find_last_of('.');if(dot==std::string::npos)continue;auto type=Lower(path.substr(dot+1));if(!Format(type))continue;
      File file;file.provider="zxinfo";file.game=game.id;file.release=NumericID(release,"releaseSeq")+1;file.id=URLId(url);file.url=url;
      file.name=path.substr(path.find_last_of('/')+1);file.type=type;file.size=entry.value("size",size_t(0));if(file.size>TapeFiles::MaxBytes)continue;
      file.year=Text(release,"yearOfRelease");file.releaseType=Text(entry,"origin");if(file.releaseType.empty())file.releaseType="release";
      file.model=type=="dsk"?MachineModel::Plus3:SpectrumModel(Text(row,"machineType"));file.supported=SpectrumSupported(Text(row,"machineType"));
      file.origin=path.find("/pub/")==0?"World of Spectrum":URLHost(url)=="spectrumcomputing.co.uk"?"Spectrum Computing":"ZXInfo";
      if(path.find("/pub/")==0)file.mirrors.push_back(ArchiveURL(path));out.push_back(file);
    }
  }
  return out;
}
inline std::vector<Game> MergeGames(const std::vector<Game> &input) {
  std::vector<Game> out;std::map<std::string,size_t> seen;
  for(const auto &game:input) {
    std::string key=game.zxdb?"zxdb:"+std::to_string(game.zxdb):game.provider+":"+std::to_string(game.id);
    auto found=seen.find(key);if(found==seen.end()){seen[key]=out.size();out.push_back(game);}
    else for(const auto &image:game.images)AddImage(out[found->second],image);
  }
  return out;
}
inline void OrderFiles(std::vector<File> &files) {
  std::stable_sort(files.begin(),files.end(),[](const File &a,const File &b){
    auto rank=[](const File &file){return (file.supported?0:1000)+(Lower(file.releaseType).find("original")==0?0:20)+(file.type=="tzx"?0:file.type=="tap"?1:file.type=="sna"?2:file.type=="z80"?3:file.type=="zip"?10:5);};
    return rank(a)<rank(b);
  });
}
// itch.io's documented RSS feed provides discovery, not authenticated downloads.
// Expat handles escaping/CDATA; DTDs are rejected and input/entry counts bounded.
inline std::vector<Game> ParseItchFeed(const Bytes &bytes) {
  if(bytes.size()>4*1024*1024)throw std::runtime_error("itch.io feed exceeds size limit");
  struct Feed {XML_Parser parser=nullptr;std::vector<Game> games;Game game;std::string field,text;bool item=false,rejected=false;unsigned depth=0,fieldDepth=0;};
  Feed state;std::unique_ptr<std::remove_pointer<XML_Parser>::type,decltype(&XML_ParserFree)> parser(XML_ParserCreate(nullptr),XML_ParserFree);
  if(!parser)throw std::runtime_error("Cannot allocate feed parser");state.parser=parser.get();XML_SetUserData(parser.get(),&state);
  XML_SetStartDoctypeDeclHandler(parser.get(),+[](void *user,const XML_Char *,const XML_Char *,const XML_Char *,int){auto &s=*static_cast<Feed *>(user);s.rejected=true;XML_StopParser(s.parser,XML_FALSE);});
  XML_SetElementHandler(parser.get(),+[](void *user,const char *name,const char **){auto &s=*static_cast<Feed *>(user);
    if(++s.depth>32){s.rejected=true;XML_StopParser(s.parser,XML_FALSE);return;}
    if(!strcmp(name,"item")){s.item=true;s.game=Game();s.game.provider="itch";}
    else if(s.item && (!strcmp(name,"title") || !strcmp(name,"plainTitle") || !strcmp(name,"link") || !strcmp(name,"imageurl") || !strcmp(name,"price"))){s.field=name;s.text.clear();s.fieldDepth=s.depth;}
  },+[](void *user,const char *name){auto &s=*static_cast<Feed *>(user);
    if(s.item && !s.field.empty() && s.depth==s.fieldDepth){
      if(s.field=="plainTitle" || (s.field=="title" && s.game.title.empty()))s.game.title=s.text;
      else if(s.field=="link")s.game.pageURL=s.text;
      else if(s.field=="imageurl")AddImage(s.game,s.text);
      else if(s.field=="price")s.game.price=s.text;
      s.field.clear();s.text.clear();
    }
    if(!strcmp(name,"item")){if(!s.game.title.empty() && ItchPage(s.game.pageURL) && s.games.size()<200){s.game.id=ItchID+URLId(s.game.pageURL);s.games.push_back(s.game);}s.item=false;}
    if(s.depth)--s.depth;
  });
  XML_SetCharacterDataHandler(parser.get(),+[](void *user,const char *text,int length){auto &s=*static_cast<Feed *>(user);if(!s.field.empty() && s.text.size()+size_t(length)<=4096)s.text.append(text,length);});
  if(XML_Parse(parser.get(),reinterpret_cast<const char *>(bytes.data()),int(bytes.size()),XML_TRUE)==XML_STATUS_ERROR || state.rejected)throw std::runtime_error("Invalid itch.io feed");
  return state.games;
}
}
#endif
