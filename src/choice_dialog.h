#ifndef CHOICE_DIALOG_H
#define CHOICE_DIALOG_H
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <memory>
#include <string>
#include <vector>

class ChoiceDialog {
public:
  static int Show(ALLEGRO_DISPLAY *parent,const std::vector<std::string> &options) {
    if(options.empty())return -1;
    struct Target{ALLEGRO_DISPLAY *parent;~Target(){al_set_target_backbuffer(parent);}}target{parent};
    int old=al_get_new_display_flags(),flags=ALLEGRO_WINDOWED|ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    flags|=ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(flags);
    std::unique_ptr<ALLEGRO_DISPLAY,decltype(&al_destroy_display)> display(al_create_display(640,400),al_destroy_display);al_set_new_display_flags(old);
    if(!display)return -1;al_set_window_title(display.get(),"Gomas - Choose TZX route");
    std::unique_ptr<ALLEGRO_FONT,decltype(&al_destroy_font)> font(al_create_builtin_font(),al_destroy_font);
    std::unique_ptr<ALLEGRO_EVENT_QUEUE,decltype(&al_destroy_event_queue)> events(al_create_event_queue(),al_destroy_event_queue);if(!font || !events)return -1;
    al_register_event_source(events.get(),al_get_display_event_source(display.get()));al_register_event_source(events.get(),al_get_display_event_source(parent));
    al_register_event_source(events.get(),al_get_keyboard_event_source());al_register_event_source(events.get(),al_get_mouse_event_source());
    unsigned selected=0,first=0;bool dirty=true;double click=-1;unsigned clickRow=0;
    while(true) {
      if(selected<first)first=selected;if(selected>=first+11)first=selected-10;
      if(dirty){al_set_target_backbuffer(display.get());al_clear_to_color(al_map_rgb(15,19,26));
        al_draw_text(font.get(),al_map_rgb(232,239,247),20,18,0,"CHOOSE A TAPE ROUTE");
        for(unsigned row=0;row<11 && first+row<options.size();++row){unsigned index=first+row;float y=55+row*25;
          al_draw_filled_rectangle(16,y,624,y+23,index==selected?al_map_rgb(34,52,68):al_map_rgb(23,31,43));
          al_draw_text(font.get(),al_map_rgb(232,239,247),25,y+7,0,options[index].substr(0,72).c_str());}
        al_draw_text(font.get(),al_map_rgb(153,171,193),20,350,0,"Arrows / wheel: browse. Enter: choose. Esc: cancel.");
        al_draw_filled_rectangle(470,368,624,395,al_map_rgb(49,83,109));al_draw_text(font.get(),al_map_rgb(232,239,247),547,377,ALLEGRO_ALIGN_CENTER,"Choose");
        al_flip_display();dirty=false;}
      ALLEGRO_EVENT event;al_wait_for_event(events.get(),&event);
      if(event.type==ALLEGRO_EVENT_DISPLAY_CLOSE)return -1;
      if(event.type==ALLEGRO_EVENT_DISPLAY_EXPOSE){dirty=true;continue;}
      if(event.type==ALLEGRO_EVENT_KEY_DOWN && event.keyboard.display==display.get()){
        if(event.keyboard.keycode==ALLEGRO_KEY_ESCAPE)return -1;
        if(event.keyboard.keycode==ALLEGRO_KEY_ENTER)return selected;
        if(event.keyboard.keycode==ALLEGRO_KEY_UP && selected)--selected;
        if(event.keyboard.keycode==ALLEGRO_KEY_DOWN && selected+1<options.size())++selected;dirty=true;
      }
      if(event.type==ALLEGRO_EVENT_MOUSE_AXES && event.mouse.display==display.get() && event.mouse.dz){
        selected=std::max(0,std::min(int(options.size())-1,int(selected)-event.mouse.dz));dirty=true;}
      if(event.type==ALLEGRO_EVENT_MOUSE_BUTTON_DOWN && event.mouse.display==display.get() && event.mouse.button==1){
        if(event.mouse.y>=368 && event.mouse.x>=470)return selected;
        if(event.mouse.x>=16 && event.mouse.x<624 && event.mouse.y>=55 && event.mouse.y<330){
          unsigned row=first+(event.mouse.y-55)/25;if(row<options.size()){
            selected=row;if(clickRow==row && event.mouse.timestamp-click>=0 && event.mouse.timestamp-click<0.4)return selected;
            click=event.mouse.timestamp;clickRow=row;dirty=true;}
        }
      }
    }
  }
};
#endif
