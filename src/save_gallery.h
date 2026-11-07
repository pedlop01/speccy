#ifndef SAVE_GALLERY_H
#define SAVE_GALLERY_H
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <array>
#include "save_state.h"

// Modal gallery: machine/audio are stopped by the caller; only this display's
// input is accepted. Every thumbnail belongs to this display's GL context.
class SaveGallery {
public:
  enum class Result { Cancelled, Saved, Loaded, Failed };
  Result Show(ALLEGRO_DISPLAY *parent,SpectrumMachine &machine,SaveStates::Store &store,
      bool saving,std::string &error) {
    struct Target {ALLEGRO_DISPLAY *parent;~Target(){al_set_target_backbuffer(parent);}} target{parent};
    int flags=al_get_new_display_flags();
    int galleryFlags=ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE|ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    galleryFlags|=ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(galleryFlags);
    std::unique_ptr<ALLEGRO_DISPLAY,decltype(&al_destroy_display)> display(al_create_display(960,700),al_destroy_display);
    al_set_new_display_flags(flags);
    if(!display) {error="Cannot create saved-game gallery";return Result::Failed;}
    al_set_window_title(display.get(),saving?"Gomas - Save game":"Gomas - Load game");
    int x,y;al_get_window_position(parent,&x,&y);al_set_window_position(display.get(),x+20,y+20);
    std::unique_ptr<ALLEGRO_EVENT_QUEUE,decltype(&al_destroy_event_queue)> events(al_create_event_queue(),al_destroy_event_queue);
    int bitmapFlags=al_get_new_bitmap_flags();al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    std::unique_ptr<ALLEGRO_FONT,decltype(&al_destroy_font)> font(al_create_builtin_font(),al_destroy_font);
    al_set_new_bitmap_flags(bitmapFlags);
    if(!events || !font) {error="Cannot allocate gallery resources";return Result::Failed;}
    al_register_event_source(events.get(),al_get_display_event_source(display.get()));
    al_register_event_source(events.get(),al_get_display_event_source(parent));
    al_register_event_source(events.get(),al_get_keyboard_event_source());
    al_register_event_source(events.get(),al_get_mouse_event_source());
    using Bitmap=std::unique_ptr<ALLEGRO_BITMAP,decltype(&al_destroy_bitmap)>;
    struct Slot {
      bool exists=false,valid=false;
      std::string label,date,error;
      Bitmap image{nullptr,al_destroy_bitmap};
    };
    std::array<Slot,SaveStates::SlotCount> slots;
    auto thumbnail=[&](const std::vector<unsigned char> &rgb)->Bitmap {
      int old=al_get_new_bitmap_flags();al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      Bitmap image(al_create_bitmap(SaveStates::ThumbWidth,SaveStates::ThumbHeight),al_destroy_bitmap);
      al_set_new_bitmap_flags(old);if(!image) return image;
      auto *lock=al_lock_bitmap(image.get(),ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_WRITEONLY);
      if(!lock) {image.reset();return image;}
      for(unsigned y=0;y<SaveStates::ThumbHeight;++y) {
        auto *row=reinterpret_cast<unsigned char *>(lock->data)+static_cast<std::ptrdiff_t>(y)*lock->pitch;
        for(unsigned x=0;x<SaveStates::ThumbWidth;++x) {
          for(unsigned c=0;c<3;++c) row[x*4+c]=rgb[(y*SaveStates::ThumbWidth+x)*3+c];
          row[x*4+3]=255;
        }
      }
      al_unlock_bitmap(image.get());return image;
    };
    for(unsigned i=0;i<SaveStates::SlotCount;++i) {
      auto &slot=slots[i];slot.exists=store.Exists(i);if(!slot.exists) continue;
      SaveStates::Record saved;
      if(!store.Read(i,saved,slot.error)) continue;
      slot.valid=true;slot.label=saved.label;slot.image=thumbnail(saved.thumbnail);
      std::time_t stamp=static_cast<std::time_t>(saved.created);std::tm local{};char text[40];
      if(saved.created<=std::uint64_t(std::numeric_limits<std::time_t>::max()) && Platform::LocalTime(&stamp,&local) &&
          std::strftime(text,sizeof(text),"%Y-%m-%d %H:%M",&local)) slot.date=text;
      else slot.date="Unknown date";
    }
    Bitmap current(nullptr,al_destroy_bitmap);
    std::string currentLabel;
    if(saving) {
      try {auto preview=SaveStates::Capture(machine);current=thumbnail(preview.thumbnail);currentLabel=preview.label;}
      catch(const std::exception &e) {error=e.what();return Result::Failed;}
    }
    unsigned selected=saving?1:0;
    if(saving) {for(unsigned i=1;i<SaveStates::SlotCount;++i) if(!slots[i].exists) {selected=i;break;}}
    else {for(unsigned i=0;i<SaveStates::SlotCount;++i) if(slots[i].valid) {selected=i;break;}}
    std::string message;
    bool dirty=true;
    int lastClickSlot=-1;
    double lastClickTime=0;
    float lastClickX=0,lastClickY=0;
    auto allowed=[&](){return saving?selected!=SaveStates::QuickSlot:slots[selected].valid;};
    auto act=[&]()->Result {
      if(!allowed()) return Result::Cancelled;
      bool ok=saving?store.Save(selected,machine,message):store.Load(selected,machine,message);
      if(ok) return saving?Result::Saved:Result::Loaded;
      dirty=true;return Result::Cancelled;
    };
    auto viewport=[&](){return std::max(0.01f,std::min(al_get_display_width(display.get())/960.0f,al_get_display_height(display.get())/700.0f));};
    while(true) {
      if(dirty) {
        al_set_target_backbuffer(display.get());al_clear_to_color(al_map_rgb(15,19,26));
        float scale=viewport();ALLEGRO_TRANSFORM transform;al_identity_transform(&transform);al_scale_transform(&transform,scale,scale);
        al_use_transform(&transform);
        auto white=al_map_rgb(232,239,247),muted=al_map_rgb(153,171,193),accent=al_map_rgb(81,197,231);
        al_draw_text(font.get(),white,20,20,0,saving?"SAVE GAME":"LOAD GAME");
        std::string subtitle=saving?"Choose a slot for "+currentLabel:"Choose a saved game. The QUICK slot can also be loaded here.";
        al_draw_text(font.get(),muted,20,42,0,subtitle.substr(0,110).c_str());
        for(unsigned i=0;i<SaveStates::SlotCount;++i) {
          float left=20+(i%4)*232,top=76+(i/4)*186;
          const auto &slot=slots[i];bool reserved=saving && i==SaveStates::QuickSlot;
          al_draw_filled_rounded_rectangle(left,top,left+220,top+174,5,5,
              al_map_rgb(i==selected?34:23,i==selected?52:31,i==selected?68:43));
          al_draw_rounded_rectangle(left+1,top+1,left+219,top+173,5,5,
              i==selected?accent:i==0?al_map_rgb(75,107,176):al_map_rgb(51,65,83),i==selected?2:1);
          std::string title=i==0?"QUICK - F2 / F3":"SLOT "+std::to_string(i);
          al_draw_text(font.get(),reserved?muted:white,left+10,top+7,0,title.c_str());
          ALLEGRO_BITMAP *image=slot.image.get();
          if(saving && i==selected && !slot.exists && !reserved) image=current.get();
          if(image) al_draw_bitmap(image,left+30,top+22,0);
          else {
            al_draw_filled_rectangle(left+30,top+22,left+190,top+142,al_map_rgb(12,17,24));
            al_draw_text(font.get(),muted,left+110,top+77,ALLEGRO_ALIGN_CENTER,
                !slot.exists?"Empty slot":slot.valid?"No preview":"Unreadable save");
          }
          std::string label=slot.valid?slot.label:reserved?"Reserved for quick save":slot.exists?"Cannot load this slot":
              saving && i==selected?"New save":"Empty";
          al_draw_text(font.get(),slot.exists&&!slot.valid?al_map_rgb(234,128,120):muted,left+10,top+149,0,label.substr(0,25).c_str());
          al_draw_text(font.get(),muted,left+10,top+161,0,slot.date.c_str());
        }
        auto button=[&](float left,float right,const std::string &label,bool enabled) {
          al_draw_filled_rounded_rectangle(left,645,right,681,4,4,enabled?al_map_rgb(49,83,109):al_map_rgb(30,39,51));
          al_draw_text(font.get(),enabled?white:muted,(left+right)/2,659,ALLEGRO_ALIGN_CENTER,label.c_str());
        };
        button(594,718,"Cancel (Esc)",true);
        button(738,936,saving?(slots[selected].exists?"Replace slot ":"Save to slot ")+std::to_string(selected):"Load selected",allowed());
        std::string status=message.empty()?slots[selected].error:message;
        if(status.empty()) status=saving?"Double-click to save or replace. The QUICK slot is reserved.":"Double-click a slot to load; arrows select, Enter loads.";
        al_draw_text(font.get(),message.empty()?muted:al_map_rgb(240,145,125),20,650,0,status.substr(0,69).c_str());
        if(status.size()>69) al_draw_text(font.get(),muted,20,663,0,status.substr(69,69).c_str());
        al_flip_display();al_identity_transform(&transform);al_use_transform(&transform);dirty=false;
      }
      ALLEGRO_EVENT event;al_wait_for_event(events.get(),&event);
      if(event.type==ALLEGRO_EVENT_DISPLAY_CLOSE && (event.display.source==display.get() || event.display.source==parent)) return Result::Cancelled;
      if(event.type==ALLEGRO_EVENT_DISPLAY_RESIZE && event.display.source==display.get()) {al_acknowledge_resize(display.get());dirty=true;}
      if(event.type==ALLEGRO_EVENT_DISPLAY_EXPOSE && event.display.source==display.get()) dirty=true;
      if(event.type==ALLEGRO_EVENT_KEY_DOWN && event.keyboard.display==display.get()) {
        lastClickSlot=-1;
        unsigned previous=selected;
        switch(event.keyboard.keycode) {
          case ALLEGRO_KEY_ESCAPE:return Result::Cancelled;
          case ALLEGRO_KEY_LEFT:if(selected) --selected;break;
          case ALLEGRO_KEY_RIGHT:if(selected+1<SaveStates::SlotCount) ++selected;break;
          case ALLEGRO_KEY_UP:if(selected>=4) selected-=4;break;
          case ALLEGRO_KEY_DOWN:if(selected+4<SaveStates::SlotCount) selected+=4;break;
          case ALLEGRO_KEY_ENTER:{auto result=act();if(result!=Result::Cancelled)return result;break;}
        }
        if(previous!=selected) message.clear();dirty=true;
      }
      if(event.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && event.mouse.display==display.get() && event.mouse.button==1) {
        float x=event.mouse.x/viewport(),y=event.mouse.y/viewport();
        int previousClickSlot=lastClickSlot;lastClickSlot=-1;
        if(y>=645 && y<681) {
          if(x>=594 && x<718) return Result::Cancelled;
          if(x>=738 && x<936) {auto result=act();if(result!=Result::Cancelled)return result;}
        } else if(x>=20 && y>=76) {
          int col=int((x-20)/232),row=int((y-76)/186);
          if(col<4 && row<3 && x<20+col*232+220 && y<76+row*186+174) {
            selected=row*4+col;message.clear();
            float dx=x-lastClickX,dy=y-lastClickY;
            bool doubleClick=previousClickSlot==int(selected) && event.mouse.timestamp>=lastClickTime &&
                event.mouse.timestamp-lastClickTime<=0.4 && dx*dx+dy*dy<=36;
            if(doubleClick) {auto result=act();if(result!=Result::Cancelled)return result;}
            else {lastClickSlot=selected;lastClickTime=event.mouse.timestamp;lastClickX=x;lastClickY=y;}
          }
        }
        dirty=true;
      }
    }
  }
};
#endif
