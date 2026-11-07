#ifndef CATALOG_VIEW_H
#define CATALOG_VIEW_H
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>
#include <chrono>
#include <future>
#include "game_catalog.h"
#include "catalog_art.h"

class CatalogView {
public:
  struct Selection {std::string path;MachineModel model=MachineModel::Spectrum48;std::string type;};
  Selection Show(ALLEGRO_DISPLAY *parent,GameCatalog::Library &library) {
    using namespace GameCatalog;
    if(!al_init_image_addon())throw std::runtime_error("Cannot initialize catalog image support");
    struct Target{ALLEGRO_DISPLAY *parent;~Target(){al_set_target_backbuffer(parent);}}target{parent};
    int old=al_get_new_display_flags(),flags=ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE|ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    flags|=ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(flags);
    std::unique_ptr<ALLEGRO_DISPLAY,decltype(&al_destroy_display)> display(al_create_display(1080,730),al_destroy_display);al_set_new_display_flags(old);
    if(!display)throw std::runtime_error("Cannot create game catalog window");al_set_window_title(display.get(),"Gomas - Game catalog");
    std::unique_ptr<ALLEGRO_FONT,decltype(&al_destroy_font)> font(al_create_builtin_font(),al_destroy_font);
    std::unique_ptr<ALLEGRO_EVENT_QUEUE,decltype(&al_destroy_event_queue)> events(al_create_event_queue(),al_destroy_event_queue);
    if(!font || !events)throw std::runtime_error("Cannot allocate game catalog resources");
    al_register_event_source(events.get(),al_get_display_event_source(display.get()));al_register_event_source(events.get(),al_get_display_event_source(parent));
    al_register_event_source(events.get(),al_get_keyboard_event_source());al_register_event_source(events.get(),al_get_mouse_event_source());
    enum Kind{Search,Versions,Artwork,Install};
    struct Request {Kind kind=Search;unsigned generation=0,page=0;std::string query;bool descending=false,offline=false,installed=false;Game game;GameCatalog::File file;};
    struct Result {Request request;Page page;std::vector<GameCatalog::File> files;std::string path,error;std::vector<std::string> artwork;std::set<std::uint64_t> installedGames;std::set<std::string> installedFiles;bool cached=false;};
    Progress progress;std::future<Result> job;bool busy=false,pending=false,closing=false,dirty=true;
    bool offline=library.Offline(),installedOnly=false,descending=false,searchFocus=true,fileFocus=false;
    std::string query,message;Page page;std::vector<GameCatalog::File> files;
    using Bitmap=std::unique_ptr<ALLEGRO_BITMAP,decltype(&al_destroy_bitmap)>;
    std::vector<Bitmap> artwork;unsigned selectedArt=0;
    const unsigned visibleFiles=6;
    int selected=-1,selectedFile=-1;unsigned fileFirst=0,gameFirst=0,generation=0;Request next,current;
    std::set<std::uint64_t> installedGames=library.InstalledGames();std::set<std::string> installedFiles=library.InstalledFiles();
    double due=0,lastDraw=0,lastClick=-1;int clickedGame=-1;
    auto request=[&](Request r,double delay){r.generation=++generation;next=std::move(r);pending=true;due=al_get_time()+delay;if(busy)progress.cancel=true;dirty=true;};
    auto search=[&](double delay){Request r;r.kind=Search;r.query=query;r.page=0;r.offline=offline;r.installed=installedOnly;r.descending=descending;selected=selectedFile=-1;files.clear();artwork.clear();selectedArt=0;fileFirst=gameFirst=0;request(r,delay);};
    auto loadVersions=[&](unsigned index){if(index>=page.games.size())return;selected=index;if(index<gameFirst)gameFirst=index;if(index>=gameFirst+19)gameFirst=index-18;selectedFile=-1;files.clear();artwork.clear();selectedArt=0;fileFirst=0;Request r;r.kind=Versions;r.game=page.games[index];r.offline=offline || installedOnly;request(r,0);};
    auto activate=[&](){if(selected<0 || selectedFile<0 || size_t(selectedFile)>=files.size() || !files[selectedFile].supported)return;Request r;r.kind=Install;r.game=page.games[selected];r.file=files[selectedFile];r.offline=offline || installedOnly;request(r,0);};
    auto changePage=[&](int direction){if((busy && current.kind==Install) || pending)return;int number=int(page.page)+direction;if(number<0 || size_t(number)*PageSize>=page.total)return;
      Request r;r.kind=Search;r.query=query;r.page=number;r.offline=offline;r.installed=installedOnly;r.descending=descending;selected=selectedFile=-1;files.clear();artwork.clear();selectedArt=0;gameFirst=0;request(r,0);};
    auto scale=[&](){return std::max(0.01f,std::min(al_get_display_width(display.get())/1080.f,al_get_display_height(display.get())/730.f));};
    auto cut=[](const std::string &s,size_t limit){size_t n=std::min(limit,s.size());while(n<s.size() && n && (static_cast<unsigned char>(s[n])&0xc0)==0x80)--n;return s.substr(0,n);};
    search(0);
    Selection chosen;
    while(true) {
      double now=al_get_time();
      if(busy && job.wait_for(std::chrono::milliseconds(0))==std::future_status::ready) {
        Result result=job.get();busy=false;
        if(!closing && result.request.generation==generation) {
          installedGames=std::move(result.installedGames);installedFiles=std::move(result.installedFiles);message=result.error;
          if(result.request.kind==Search){page=std::move(result.page);gameFirst=0;if(result.cached && message.empty())message="Offline catalog: games cached from previous visits.";}
          else if(result.request.kind==Versions){files=std::move(result.files);if(result.request.offline)files.erase(std::remove_if(files.begin(),files.end(),[&](const GameCatalog::File &file){return !installedFiles.count(library.Key(file));}),files.end());fileFirst=0;selectedFile=files.empty()?-1:0;if(files.empty() && message.empty())message="No compatible download is available for this game.";
            Request r;r.kind=Artwork;r.game=result.request.game;r.offline=result.request.offline;request(r,0);
          }
          else if(result.request.kind==Artwork){
            artwork.clear();selectedArt=0;
            for(const auto &path:result.artwork){Bitmap bitmap(LoadCatalogArtwork(path),al_destroy_bitmap);if(bitmap)artwork.push_back(std::move(bitmap));}
          }
          else if(!result.path.empty()){chosen.path=result.path;chosen.model=result.request.file.model;chosen.type=result.request.file.type;closing=true;}
          dirty=true;
        }else if(!closing && !pending){message="Cancelled";dirty=true;}
      }
      if(closing && !busy){try{library.SetOffline(offline);}catch(const std::exception &e){if(chosen.path.empty())message=e.what();}return chosen;}
      if(!closing && !busy && pending && now>=due) {
        current=next;pending=false;busy=true;progress.cancel=false;progress.received=0;progress.total=0;Request r=current;
        message=r.kind==Search?(r.offline || r.installed?"Loading local library...":"Searching ZXArt..."):r.kind==Versions?"Loading versions...":r.kind==Artwork?"Loading artwork...":"Checking / downloading game...";
        job=std::async(std::launch::async,[&library,&progress,r](){Result result;result.request=r;
          try {
            if(r.kind==Search){if(r.offline || r.installed){result.page=library.Cached(r.query,r.page,r.descending,r.installed);result.cached=true;}else result.page=library.Search(r.query,r.page,r.descending,progress);}
            else if(r.kind==Versions){result.files=r.offline?library.CachedFiles(r.game):library.Versions(r.game,progress);if(r.offline)result.cached=true;}
            else if(r.kind==Artwork)result.artwork=library.Artwork(r.game,r.offline,progress);
            else result.path=library.Install(r.file,r.offline,progress);
          }catch(const std::exception &e){result.error=e.what();if(r.kind==Search && !progress.cancel){result.page=library.Cached(r.query,r.page,r.descending,r.installed);if(result.page.games.empty() && result.page.total)result.page=library.Cached(r.query,0,r.descending,r.installed);result.cached=true;result.error="Network unavailable. Showing cached games. "+result.error;}
            if(r.kind==Versions && !progress.cancel)result.files=library.CachedFiles(r.game);}
          result.installedGames=library.InstalledGames();result.installedFiles=library.InstalledFiles();return result;
        });dirty=true;
      }

      if((dirty || busy || pending) && now-lastDraw>=0.03) {
        al_set_target_backbuffer(display.get());al_clear_to_color(al_map_rgb(15,19,26));ALLEGRO_TRANSFORM t;al_identity_transform(&t);al_scale_transform(&t,scale(),scale());al_use_transform(&t);
        auto white=al_map_rgb(232,239,247),muted=al_map_rgb(153,171,193),green=al_map_rgb(82,208,179),accent=al_map_rgb(81,197,231);
        al_draw_text(font.get(),white,20,16,0,"GAME CATALOG");al_draw_text(font.get(),muted,1060,16,ALLEGRO_ALIGN_RIGHT,"Source: ZXArt.ee");
        auto button=[&](float x,float y,float w,const std::string &label,bool active,bool enabled=true){al_draw_filled_rounded_rectangle(x,y,x+w,y+30,4,4,al_map_rgb(enabled?(active?49:30):23,enabled?(active?83:43):31,enabled?(active?109:60):43));al_draw_text(font.get(),enabled?white:muted,x+w/2,y+11,ALLEGRO_ALIGN_CENTER,label.c_str());};
        al_draw_filled_rounded_rectangle(20,44,650,78,4,4,al_map_rgb(23,31,43));al_draw_rounded_rectangle(20,44,650,78,4,4,searchFocus?accent:muted,1);
        std::string searchText=query.empty()?"Search games...":query;if(searchFocus && int(now*2)%2 && !query.empty())searchText+="|";
        al_draw_text(font.get(),query.empty()?muted:white,32,57,0,cut(searchText,74).c_str());
        button(666,46,108,installedOnly?"Installed":"All games",installedOnly);button(786,46,104,offline?"Offline: ON":"Offline: OFF",offline);button(902,46,72,descending?"Z-A":"A-Z",false);button(986,46,74,"Refresh",false,!busy);
        al_draw_text(font.get(),muted,20,94,0,(std::to_string(page.total)+" games"+(offline || installedOnly?" - local library":" - online catalog")).c_str());
        al_draw_text(font.get(),muted,605,94,ALLEGRO_ALIGN_RIGHT,"Click a game to see versions");
        for(unsigned row=0;row<19 && gameFirst+row<page.games.size();++row){unsigned index=gameFirst+row;float y=116+row*26;const auto &game=page.games[index];bool installed=installedGames.count(game.id);
          al_draw_filled_rounded_rectangle(20,y,620,y+24,3,3,al_map_rgb(int(index)==selected?34:23,int(index)==selected?52:31,int(index)==selected?68:43));
          al_draw_text(font.get(),white,30,y+8,0,cut(game.title,installed?54:63).c_str());if(installed)al_draw_text(font.get(),green,610,y+8,ALLEGRO_ALIGN_RIGHT,"Installed");
        }
        if(page.games.empty() && !busy)al_draw_text(font.get(),muted,35,140,0,offline || installedOnly?"No cached games match this search.":"No games match this search.");
        al_draw_filled_rounded_rectangle(640,94,1060,623,5,5,al_map_rgb(23,31,43));
        if(selected>=0 && size_t(selected)<page.games.size()) {
          const auto &game=page.games[selected];al_draw_text(font.get(),white,656,110,0,cut(game.title,48).c_str());al_draw_text(font.get(),muted,656,132,0,(game.year+"  |  ZXArt #"+std::to_string(game.id)).c_str());
          al_draw_filled_rounded_rectangle(650,151,1050,355,4,4,al_map_rgb(12,17,24));
          if(!artwork.empty()) {
            auto *image=artwork[selectedArt].get();float iw=al_get_bitmap_width(image),ih=al_get_bitmap_height(image);
            float fit=std::min(390.f/iw,194.f/ih);
            al_draw_scaled_bitmap(image,0,0,iw,ih,850-iw*fit/2,253-ih*fit/2,iw*fit,ih*fit,0);
          }else al_draw_text(font.get(),muted,850,248,ALLEGRO_ALIGN_CENTER,
              busy && (current.kind==Versions || current.kind==Artwork)?"Loading preview...":"No artwork available offline / on ZXArt");
          if(artwork.size()>1) {
            button(650,360,58,"<",false);button(992,360,58,">",false);
            al_draw_textf(font.get(),muted,850,371,ALLEGRO_ALIGN_CENTER,"Image %u / %u",selectedArt+1,unsigned(artwork.size()));
          }
          al_draw_textf(font.get(),muted,656,399,0,"Versions / formats (%u)%s",unsigned(files.size()),files.size()>visibleFiles?" - scroll for more":"");
          for(unsigned row=0;row<visibleFiles && fileFirst+row<files.size();++row){unsigned index=fileFirst+row;float y=418+row*25;const auto &file=files[index];bool installed=installedFiles.count(library.Key(file));
            al_draw_filled_rounded_rectangle(650,y,1050,y+24,3,3,al_map_rgb(int(index)==selectedFile?34:18,int(index)==selectedFile?52:26,int(index)==selectedFile?68:36));
            al_draw_text(font.get(),file.supported?white:muted,658,y+3,0,cut(file.name,46).c_str());
            std::string model=file.model==MachineModel::Spectrum48?"48K":file.model==MachineModel::Plus3?"+3":file.model==MachineModel::Plus2?"+2":"128K";
            std::string info=file.type+" | "+model+" | "+file.releaseType+(installed?" | Installed":"")+(!file.supported?" | Unsupported hardware":"");
            al_draw_text(font.get(),installed?green:muted,658,y+14,0,cut(info,47).c_str());
          }
          if(files.empty() && !busy)al_draw_text(font.get(),muted,656,440,0,"No compatible versions cached / available.");
          if(selectedFile>=0 && size_t(selectedFile)<files.size()){auto &file=files[selectedFile];al_draw_text(font.get(),muted,656,575,0,(std::to_string(file.size/1024)+" KB  -  "+file.year).c_str());}
        } else al_draw_text(font.get(),muted,656,116,0,"Select a game on the left.");
        button(20,629,110,"Previous",false,page.page>0 && !busy);button(145,629,110,"Next",false,(size_t(page.page)+1)*PageSize<page.total && !busy);
        al_draw_text(font.get(),muted,275,640,0,("Page "+std::to_string(page.page+1)+" / "+std::to_string(std::max<size_t>(1,(page.total+PageSize-1)/PageSize))).c_str());
        bool allowed=selectedFile>=0 && size_t(selectedFile)<files.size() && files[selectedFile].supported && (!busy || current.kind==Artwork) && !pending;
        button(640,636,260,selectedFile>=0 && size_t(selectedFile)<files.size() && installedFiles.count(library.Key(files[selectedFile]))?"Load installed game":"Install & load",true,allowed);
        button(916,636,144,busy?"Cancel task":"Close",false);
        std::string status=closing?"Cancelling network request...":message;
        if(busy && current.kind==Install){size_t received=progress.received,total=progress.total;status+=" "+std::to_string(received/1024)+" KB";if(total)status+=" / "+std::to_string(total/1024)+" KB";}
        al_draw_text(font.get(),muted,20,684,0,cut(status,130).c_str());al_draw_text(font.get(),muted,20,707,0,"Enter: search / load. Tab: change pane. Installed games remain available offline.");
        al_flip_display();al_identity_transform(&t);al_use_transform(&t);dirty=false;lastDraw=now;
      }
      ALLEGRO_EVENT event;if(!al_wait_for_event_timed(events.get(),&event,0.03))continue;
      if(event.type==ALLEGRO_EVENT_DISPLAY_CLOSE){closing=true;pending=false;progress.cancel=true;dirty=true;}
      if(closing)continue;
      if(event.type==ALLEGRO_EVENT_DISPLAY_RESIZE && event.display.source==display.get()){al_acknowledge_resize(display.get());dirty=true;}
      if(event.type==ALLEGRO_EVENT_DISPLAY_EXPOSE)dirty=true;
      if(event.type==ALLEGRO_EVENT_KEY_DOWN && event.keyboard.display==display.get()) {
        if(event.keyboard.keycode==ALLEGRO_KEY_ESCAPE){closing=true;pending=false;progress.cancel=true;dirty=true;continue;}
        if(event.keyboard.keycode==ALLEGRO_KEY_ENTER){if(searchFocus){if(pending)due=now;else search(0);}else if((!busy || current.kind==Artwork) && !pending)activate();continue;}
        if(event.keyboard.keycode==ALLEGRO_KEY_TAB){if(searchFocus){searchFocus=false;fileFocus=false;}else if(!fileFocus)fileFocus=true;else{searchFocus=true;fileFocus=false;}dirty=true;}
        if(!searchFocus && !fileFocus && event.keyboard.keycode==ALLEGRO_KEY_DOWN && selected>=0 && size_t(selected+1)<page.games.size())loadVersions(selected+1);
        if(!searchFocus && !fileFocus && event.keyboard.keycode==ALLEGRO_KEY_UP && selected>0)loadVersions(selected-1);
      }
      if(event.type==ALLEGRO_EVENT_KEY_DOWN && event.keyboard.display==display.get() && !searchFocus && fileFocus) {
        if(event.keyboard.keycode==ALLEGRO_KEY_DOWN && selectedFile+1<int(files.size()))++selectedFile;
        if(event.keyboard.keycode==ALLEGRO_KEY_UP && selectedFile>0)--selectedFile;
        if(selectedFile>=0){if(unsigned(selectedFile)<fileFirst)fileFirst=selectedFile;if(unsigned(selectedFile)>=fileFirst+visibleFiles)fileFirst=selectedFile-visibleFiles+1;}dirty=true;
      }
      if(event.type==ALLEGRO_EVENT_KEY_CHAR && event.keyboard.display==display.get() && searchFocus) {
        bool changed=false;
        if(event.keyboard.keycode==ALLEGRO_KEY_BACKSPACE && !query.empty()){size_t p=query.size()-1;while(p && (static_cast<unsigned char>(query[p])&0xc0)==0x80)--p;query.resize(p);changed=true;}
        else if(event.keyboard.unichar>=32 && event.keyboard.unichar!=127 && query.size()<196 && !(event.keyboard.modifiers&(ALLEGRO_KEYMOD_CTRL|ALLEGRO_KEYMOD_ALT))){
          auto *text=al_ustr_new(query.c_str());al_ustr_append_chr(text,event.keyboard.unichar);query=al_cstr(text);al_ustr_free(text);changed=true;}
        if(changed)search(0.3);
      }
      if(event.type==ALLEGRO_EVENT_MOUSE_AXES && event.mouse.display==display.get() && event.mouse.dz) {
        float x=event.mouse.x/scale();if(x>=640)fileFirst=std::max(0,std::min(std::max(0,int(files.size())-int(visibleFiles)),int(fileFirst)-event.mouse.dz*3));
        else gameFirst=std::max(0,std::min(std::max(0,int(page.games.size())-19),int(gameFirst)-event.mouse.dz*3));dirty=true;
      }
      if(event.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && event.mouse.display==display.get() && event.mouse.button==1) {
        float x=event.mouse.x/scale(),y=event.mouse.y/scale();searchFocus=x>=20 && x<650 && y>=44 && y<78;dirty=true;
        if(y>=46 && y<78){if(x>=666 && x<774){installedOnly=!installedOnly;search(0);}else if(x>=786 && x<890){offline=!offline;search(0);}else if(x>=902 && x<974){descending=!descending;search(0);}else if(x>=986 && !busy)search(0);}
        else if(x>=20 && x<620 && y>=116 && y<610){unsigned index=gameFirst+unsigned((y-116)/26);if(index<page.games.size()){fileFocus=false;
          bool doubleClick=clickedGame==int(index) && now-lastClick<0.4;clickedGame=index;lastClick=now;
          if(selected==int(index) && doubleClick && (!busy || current.kind==Artwork) && !pending)activate();else loadVersions(index);}}
        else if(x>=650 && x<1050 && y>=360 && y<390 && artwork.size()>1) {
          if(x<708)selectedArt=(selectedArt+artwork.size()-1)%artwork.size();
          else if(x>=992)selectedArt=(selectedArt+1)%artwork.size();
        }
        else if(x>=650 && x<1050 && y>=418 && y<568){unsigned index=fileFirst+unsigned((y-418)/25);if(index<files.size()){selectedFile=index;searchFocus=false;fileFocus=true;}}
        else if(y>=629 && y<666){if(x>=20 && x<130)changePage(-1);else if(x>=145 && x<255)changePage(1);else if(x>=640 && x<900 && (!busy || current.kind==Artwork) && !pending)activate();
          else if(x>=916 && x<1060){if(busy){++generation;progress.cancel=true;pending=false;message="Cancelling...";}else{closing=true;pending=false;}}}
      }
    }
  }
};
#endif
