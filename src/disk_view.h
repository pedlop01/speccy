#ifndef DISK_VIEW_H
#define DISK_VIEW_H
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include "emulator.h"

class DiskView {
public:
  ~DiskView(){Close();}
  bool Show(ALLEGRO_EVENT_QUEUE *queue) {
    if(display){al_set_display_flag(display,ALLEGRO_MINIMIZED,false);return true;}
    int old=al_get_new_display_flags(),flags=ALLEGRO_WINDOWED|ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    flags|=ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(flags);display=al_create_display(700,350);al_set_new_display_flags(old);if(!display)return false;
    al_set_window_title(display,"Gomas - Disk drives");font=al_create_builtin_font();if(!font){al_destroy_display(display);display=nullptr;return false;}
    events=queue;al_register_event_source(events,al_get_display_event_source(display));return true;
  }
  void Close(){if(!display)return;al_unregister_event_source(events,al_get_display_event_source(display));al_destroy_font(font);al_destroy_display(display);display=nullptr;font=nullptr;}
  bool Handle(const ALLEGRO_EVENT &event,int &action) {
    action=0;if(!display)return false;
    if(event.type==ALLEGRO_EVENT_DISPLAY_CLOSE && event.display.source==display){Close();return true;}
    if((event.type==ALLEGRO_EVENT_KEY_DOWN || event.type==ALLEGRO_EVENT_KEY_UP) && event.keyboard.display==display){if(event.type==ALLEGRO_EVENT_KEY_DOWN && event.keyboard.keycode==ALLEGRO_KEY_ESCAPE)Close();return true;}
    if((event.type==ALLEGRO_EVENT_MOUSE_AXES || event.type==ALLEGRO_EVENT_MOUSE_BUTTON_UP) && event.mouse.display==display)return true;
    if(event.type!=ALLEGRO_EVENT_MOUSE_BUTTON_DOWN || event.mouse.display!=display)return false;
    if(event.mouse.button==1)for(unsigned unit=0;unit<2;++unit){int top=40+unit*140;if(event.mouse.y>=top+82 && event.mouse.y<top+112 && event.mouse.x>=18){
      int button=(event.mouse.x-18)/164;if(button>=0 && button<4 && event.mouse.x<18+button*164+150)action=491+unit*4+button;}}
    return true;
  }
  void Draw(const SpectrumMachine &machine) {
    if(!display)return;al_set_target_backbuffer(display);al_clear_to_color(al_map_rgb(15,19,26));
    auto white=al_map_rgb(232,239,247),muted=al_map_rgb(153,171,193);const auto &fdc=machine.io.floppy.state;
    al_draw_text(font,white,18,15,0,machine.Configuration().model==MachineModel::Plus3?"SPECTRUM +3 DISK DRIVES":"Insert a disk to switch to Spectrum +3");
    for(unsigned unit=0;unit<2;++unit){const auto &drive=fdc.drives[unit];int top=40+unit*140;
      al_draw_filled_rounded_rectangle(12,top,688,top+126,5,5,al_map_rgb(23,31,43));
      std::string label=std::string("Drive ")+char('A'+unit)+": ";label+=drive.image?drive.path.substr(drive.path.find_last_of("/\\")+1):"Empty";
      al_draw_text(font,white,22,top+12,0,label.substr(0,80).c_str());
      std::string status=drive.image?std::to_string(drive.image->cylinders)+" tracks, "+std::to_string(drive.image->heads)+" side(s), cylinder "+std::to_string(fdc.cylinder[unit]):"No disk inserted";
      if(drive.dirty)status+="  [unsaved changes]";al_draw_text(font,muted,22,top+35,0,status.c_str());
      std::string activity=fdc.motor?"Motor running":"Motor stopped";
      if(fdc.unit==unit && (fdc.phase==Disks::Controller::Phase::Read || fdc.phase==Disks::Controller::Phase::Write || fdc.phase==Disks::Controller::Phase::Format))activity+=fdc.phase==Disks::Controller::Phase::Read?" - Reading":" - Writing";
      al_draw_text(font,muted,22,top+55,0,activity.c_str());
      const char *labels[]={"Insert...","Eject","Save as...",drive.writeProtected?"Write protected":"Writable"};
      for(int b=0;b<4;++b){int x=18+b*164;al_draw_filled_rounded_rectangle(x,top+82,x+150,top+112,3,3,al_map_rgb(49,83,109));al_draw_text(font,white,x+75,top+93,ALLEGRO_ALIGN_CENTER,labels[b]);}
    }
    al_draw_text(font,muted,18,331,0,"Changes stay in memory until Save as. ZIP originals are left intact.");al_flip_display();
  }
private:
  ALLEGRO_DISPLAY *display=nullptr;ALLEGRO_FONT *font=nullptr;ALLEGRO_EVENT_QUEUE *events=nullptr;
};
#endif
