#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "../game_catalog.h"
#include "catalog_art_fixture.h"
using namespace GameCatalog;
static Bytes Data(const Json &j){auto s=j.dump();return {s.begin(),s.end()};}
static Bytes XML(const std::string &text){return {text.begin(),text.end()};}
static Json Art(unsigned id,const std::string &title,unsigned zxdb){return {{"id",id},{"title",title},{"year",1986},{"legalStatus","allowed"},{"importIds",{{"zxdb",std::to_string(zxdb)}}}};}
static Json Info(unsigned id,const std::string &title){return {{"_id",std::to_string(id)},{"_source",{{"title",title},{"machineType","ZX-Spectrum 48K"},{"originalYearOfRelease",1986},{"availability","Available"}}}};}
static Json Reply(const char *key,const Json &rows,unsigned total){return {{"responseStatus","success"},{"totalAmount",total},{"responseData",{{key,rows}}}};}
static void Reject(std::function<void()> action){bool rejected=false;try{action();}catch(const std::exception &){rejected=true;}assert(rejected);}
int main(int argc,char **argv) {
  assert(al_init());char folder[]="/tmp/gomas-sources-XXXXXX";assert(mkdtemp(folder));std::string root=folder;Bytes tap={3,0,255,128,127};unsigned requests=0,mirrorRequests=0;bool down=false;
  std::string feed="<rss><channel><item><title>Homebrew &amp; fun [Free]</title><plainTitle>Homebrew &amp; fun</plainTitle><link>https://creator.itch.io/homebrew</link><imageurl>https://img.itch.zone/cover.png</imageurl><price>$0.00</price></item></channel></rss>";
  Json detail=Info(42,"Alpha");detail["_source"]["screens"]={{{"url","/zxscreens/0000042/Alpha.png"}}};
  detail["_source"]["releases"]={{{"releaseSeq",0},{"yearOfRelease",1986},{"files",{
    {{"path","/zxdb/sinclair/entries/0000042/Alpha.tap"},{"size",tap.size()},{"origin","Original release (O)"}},
    {{"path","/pub/sinclair/games/a/Alpha.tap"},{"size",tap.size()}}}}}};
  Fetcher fake=[&](const std::string &url,size_t,Progress &progress)->Bytes {
    ++requests;if(progress.cancel)throw std::runtime_error("Cancelled");
    if(url.find("itch.io/games/")!=std::string::npos)return XML(feed);
    if(url.find("api.zxinfo.dk")!=std::string::npos && down)throw std::runtime_error("ZXInfo offline");
    if(url.find("/entries/")!=std::string::npos)return Data(detail);
    if(url.find("/v5/search?")!=std::string::npos)return XML("Hello World! catch all");
    bool paging=url.find("paging")!=std::string::npos;
    if(url.find("export:zxProd")!=std::string::npos || url.find("/v5/search")!=std::string::npos) {
      bool art=url.find("zxart.ee")!=std::string::npos;unsigned start=0;
      if(paging){auto p=url.find(art?"start:":"offset=");start=std::stoul(url.substr(p+(art?6:7)));if(!art)start*=30;}
      Json rows=Json::array();unsigned count=paging?63:1;
      for(unsigned i=start;i<std::min(count,start+30);++i){char title[32];snprintf(title,sizeof(title),"A%03u",i);
        rows.push_back(art?Art(paging?1000+i:i+1,paging?title:"Alpha",paging?100+i:42):Info(paging?100+i:42,paging?title:"Alpha"));}
      return art?Data(Reply("zxProd",rows,count)):Data(Json{{"hits",{{"total",{{"value",count},{"relation","eq"}}},{"hits",rows}}}});
    }
    if(url.find("export:zxRelease")!=std::string::npos)return Data(Reply("zxRelease",Json::array({{{"id",11},{"releaseType","original"},{"hardwareRequired",{"zx48"}},{"releaseStructure",Json::array({{{"id",1},{"fileName","Alpha.tap"},{"type","tap"},{"size",tap.size()},{"md5",MD5(tap)}}})}}}),1));
    if(url.find("worldofspectrum.net")!=std::string::npos)throw std::runtime_error("Mirror unavailable");
    if(url.find("archive.org")!=std::string::npos)++mirrorRequests;
    if(url.find("Alpha.png")!=std::string::npos || url.find("cover.png")!=std::string::npos)return PreviewPNG();
    return tap;
  };
  Library lib(root,fake);Progress p;
  auto art=lib.Search("Alpha",0,false,p).games[0];assert(art.id==1 && art.zxdb==42);
  auto original=lib.Versions(art,p,Source::ZXArt)[0];auto oldPath=lib.Install(original,false,p);
  assert(oldPath==root+"/zxart/1/11-1/Alpha.tap");
  assert(!lib.Search("",0,false,p,Source::ZXInfo).games.empty());
  auto info=lib.Search("Alpha",0,false,p,Source::ZXInfo).games[0];assert(info.id==ZXInfoID+42 && lib.Installed(info));
  assert(lib.InstalledGames().count(info.id));
  auto variants=lib.Versions(info,p,Source::All);assert(variants.size()==3);
  auto downloads=lib.Versions(info,p,Source::ZXInfo);assert(downloads.size()==2);
  auto mirrored=*std::find_if(downloads.begin(),downloads.end(),[](const File &f){return !f.mirrors.empty();});
  auto installed=lib.Install(mirrored,false,p);assert(mirrorRequests==1 && lib.Installed(mirrored));
  auto all=lib.Search("Alpha",0,false,p,Source::All);assert(all.games.size()==1 && all.games[0].zxdb==42);
  assert(lib.Cached("",0,false,true).games.size()==1); // One installed game across providers.
  lib.SetSource(Source::ZXInfo);lib.SetOffline(true);Library offline(root,fake);unsigned before=requests;
  assert(offline.SelectedSource()==Source::ZXInfo && offline.Offline());
  assert(offline.Install(original,true,p)==oldPath && offline.Install(mirrored,true,p)==installed && requests==before);
  assert(offline.CachedFiles(info).size()==3 && offline.Cached("",0,false,true,Source::Itch).games.size()==1);
  auto pictures=offline.Artwork(info,false,p);assert(!pictures.empty());
  lib.Uninstall(original);assert(lib.Installed(info));lib.Uninstall(mirrored);assert(!lib.Installed(info));
  auto itch=lib.Search("Homebrew",0,false,p,Source::Itch);assert(itch.games.size()==1 && itch.games[0].title=="Homebrew & fun" && itch.games[0].price=="$0.00");
  assert(lib.Versions(itch.games[0],p).empty() && ItchPage(itch.games[0].pageURL));
  auto first=lib.Search("paging",0,false,p,Source::All);auto second=lib.Search("paging",1,false,p,Source::All);auto third=lib.Search("paging",2,false,p,Source::All);
  assert(first.games.size()==30 && first.games[0].title=="A000");
  assert(second.games.size()==30 && second.games[0].title=="A030");assert(third.games.size()==3 && third.games[0].title=="A060" && third.total==63 && !third.estimated);
  down=true;auto partial=lib.Search("Alpha",0,false,p,Source::All);assert(!partial.games.empty() && partial.warning.find("ZXInfo")!=std::string::npos);
  p.cancel=true;Reject([&](){lib.Search("",0,false,p,Source::All);});p.cancel=false;
  assert(!CatalogURL("https://localhost/private") && !CatalogURL("https://zxart.ee.evil/a") && !ItchPage("https://itch.io.evil/a"));
  Reject([&](){ParseItchFeed(XML("<!DOCTYPE rss [<!ENTITY a SYSTEM 'file:///etc/passwd'>]><rss><channel><item><title>&a;</title></item></channel></rss>"));});
  Reject([&](){ParseItchFeed(XML("<rss><item>"));});
  auto denied=detail;denied["_source"]["availability"]="Distribution denied";assert(ZXInfoFiles(info,denied).empty());
  Reject([&](){FetchJSON([](const std::string &,size_t,Progress &){return XML("Hello World!");},"https://api.zxinfo.dk/v5/search/",p);});
  if(argc>1 && std::string(argv[1])=="--live") {
    Library live(root+"/live");auto browsing=live.Search("",0,false,p,Source::ZXInfo);assert(!browsing.games.empty());
    printf("Live ZXInfo empty search: %zu results\n",browsing.games.size());
    auto page=live.Search("Cobra",0,false,p,Source::ZXInfo);assert(!page.games.empty());
    auto match=std::find_if(page.games.begin(),page.games.end(),[](const Game &g){return g.zxdb==996;});assert(match!=page.games.end());
    auto files=live.Versions(*match,p,Source::ZXInfo);assert(!files.empty());
    auto direct=std::find_if(files.begin(),files.end(),[](const File &f){return f.origin=="Spectrum Computing";});assert(direct!=files.end());
    auto path=live.Install(*direct,false,p);assert(al_filename_exists(path.c_str()));
    auto previews=live.Artwork(*match,false,p);assert(!previews.empty());
    auto discoveries=live.Search("",0,false,p,Source::Itch);assert(!discoveries.games.empty());
    auto combined=live.Search("Cobra",0,false,p,Source::All);assert(!combined.games.empty());
    assert(std::count_if(combined.games.begin(),combined.games.end(),[](const Game &g){return g.zxdb==996;})==1);
    printf("Live All: %zu results, %zu total%s\n",combined.games.size(),combined.total,combined.estimated?" (estimated)":"");
    printf("Live ZXInfo: %s, %zu files, installed %s, %zu artwork; itch.io: %zu featured games\n",match->title.c_str(),files.size(),path.c_str(),previews.size(),discoveries.total);
  }
  // Every file here was generated in this private test directory.
  std::string cleanup="rm -rf '"+root+"'";assert(std::system(cleanup.c_str())==0);
  puts("Sources: IDs/dedup, ordered federation, unified installs, legacy paths, mirrors, offline cache, RSS bounds, cancellation and source failures passed");
}
