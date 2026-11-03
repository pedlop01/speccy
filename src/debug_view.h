#ifndef DEBUG_VIEW_H
#define DEBUG_VIEW_H
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <cstdio>
#include "emulator.h"

class DebugView {
public:
  enum class Action {None,Pause,Step,Frame,Toggle};
  ~DebugView(){Close();}
  bool IsOpen() const{return display!=nullptr;}
  bool Show(ALLEGRO_EVENT_QUEUE *queue) {
    if(display){al_set_display_flag(display,ALLEGRO_MINIMIZED,false);return true;}
    int old=al_get_new_display_flags(),flags=ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE|ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    flags|=ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(flags);display=al_create_display(960,650);al_set_new_display_flags(old);if(!display)return false;
    al_set_window_title(display,"Gomas - Debugger");int bitmaps=al_get_new_bitmap_flags();al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);font=al_create_builtin_font();al_set_new_bitmap_flags(bitmaps);
    if(!font){al_destroy_display(display);display=nullptr;return false;}
    events=queue;al_register_event_source(events,al_get_display_event_source(display));return true;
  }
  void Close(){if(!display)return;al_unregister_event_source(events,al_get_display_event_source(display));al_destroy_font(font);al_destroy_display(display);display=nullptr;font=nullptr;}
  void RecordStep(unsigned cycles){lastCycles=cycles;frameStep=false;}
  void RecordFrame(){frameStep=true;}
  static unsigned char ReadByte(SpectrumMachine &machine,int bank,unsigned address){
    // Never read an I/O port here: reads from devices can consume data.
    return bank<0?machine.memory.Read(address&65535):machine.memory.ReadBank(unsigned(bank),address&16383);
  }
  bool Handle(const ALLEGRO_EVENT &event,SpectrumMachine &machine,Action &action) {
    action=Action::None;if(!display)return false;
    if(event.type==ALLEGRO_EVENT_DISPLAY_CLOSE && event.display.source==display){Close();return true;}
    if(event.type==ALLEGRO_EVENT_DISPLAY_RESIZE && event.display.source==display){al_acknowledge_resize(display);return true;}
    if((event.type==ALLEGRO_EVENT_DISPLAY_EXPOSE || event.type==ALLEGRO_EVENT_DISPLAY_SWITCH_IN) && event.display.source==display)return true;
    if((event.type==ALLEGRO_EVENT_KEY_DOWN || event.type==ALLEGRO_EVENT_KEY_UP || event.type==ALLEGRO_EVENT_KEY_CHAR) && event.keyboard.display==display){
      if(event.type!=ALLEGRO_EVENT_KEY_DOWN)return true;
      int key=event.keyboard.keycode;
      if(key==ALLEGRO_KEY_ESCAPE){if(editing){editing=false;addressText.clear();}else Close();}
      else if(key==ALLEGRO_KEY_F12)action=Action::Toggle;
      else if(key==ALLEGRO_KEY_F5)action=Action::Pause;
      else if(key==ALLEGRO_KEY_F10)action=Action::Step;
      else if(key==ALLEGRO_KEY_F4)action=Action::Frame;
      else if(editing){
        if(key==ALLEGRO_KEY_ENTER)Go();
        else if(key==ALLEGRO_KEY_BACKSPACE && !addressText.empty())addressText.pop_back();
        else if(addressText.size()<4){char value=0;if(key>=ALLEGRO_KEY_0 && key<=ALLEGRO_KEY_9)value='0'+key-ALLEGRO_KEY_0;else if(key>=ALLEGRO_KEY_A && key<=ALLEGRO_KEY_F)value='A'+key-ALLEGRO_KEY_A;if(value)addressText+=value;}
      }else if(key==ALLEGRO_KEY_UP)Scroll(-16);else if(key==ALLEGRO_KEY_DOWN)Scroll(16);
      else if(key==ALLEGRO_KEY_PGUP)Scroll(-256);else if(key==ALLEGRO_KEY_PGDN)Scroll(256);
      else if(key==ALLEGRO_KEY_HOME){bank=-1;follow=true;}
      return true;
    }
    if(event.type==ALLEGRO_EVENT_MOUSE_AXES && event.mouse.display==display){if(event.mouse.dz)Scroll(-event.mouse.dz*48);return true;}
    if(event.type==ALLEGRO_EVENT_MOUSE_BUTTON_UP && event.mouse.display==display)return true;
    if(event.type!=ALLEGRO_EVENT_MOUSE_BUTTON_DOWN || event.mouse.display!=display)return false;
    if(event.mouse.button!=1)return true;
    float x=event.mouse.x/Scale(),y=event.mouse.y/Scale();
    if(y>=44 && y<76){editing=false;
      if(x>=20 && x<140)action=Action::Pause;
      else if(x>=152 && x<297)action=Action::Step;
      else if(x>=309 && x<454)action=Action::Frame;
      else if(x>=466 && x<601){bank=-1;follow=!follow;}
      else if(x>=613 && x<718){bank=-1;follow=false;address=machine.cpu.regs.SP&0xfff0;}
      else if(x>=730 && x<940){follow=false;++bank;if(bank>=int(machine.Configuration().memory.ramBanks))bank=-1;address&=Mask();}
    } else if(y>=278 && y<305){if(x>=126 && x<201){editing=true;addressText.clear();badAddress=false;}else if(x>=210 && x<275)Go();}
    return true;
  }
  void Draw(SpectrumMachine &machine,bool paused) {
    if(!display)return;if(bank>=int(machine.Configuration().memory.ramBanks)){bank=-1;address&=65535;}
    if(follow){bank=-1;address=machine.cpu.regs.PC&0xfff0;}
    const auto &r=machine.cpu.regs;auto u=machine.ula.CaptureState();
    al_set_target_backbuffer(display);al_clear_to_color(al_map_rgb(15,19,26));ALLEGRO_TRANSFORM t;al_identity_transform(&t);al_scale_transform(&t,Scale(),Scale());al_use_transform(&t);
    auto white=al_map_rgb(232,239,247),muted=al_map_rgb(153,171,193),cyan=al_map_rgb(81,197,231),purple=al_map_rgb(202,151,228);
    al_draw_text(font,white,20,16,0,"DEBUGGER");al_draw_text(font,paused?cyan:muted,116,16,0,paused?"PAUSED":"RUNNING");al_draw_text(font,muted,940,16,ALLEGRO_ALIGN_RIGHT,machine.systemROM.name.c_str());
    auto button=[&](int x,int w,const std::string &text,bool active){al_draw_filled_rounded_rectangle(x,44,x+w,76,4,4,active?al_map_rgb(49,83,109):al_map_rgb(30,43,60));al_draw_text(font,white,x+w/2,56,ALLEGRO_ALIGN_CENTER,text.c_str());};
    button(20,120,paused?"Resume (F5)":"Pause (F5)",false);button(152,145,"Step CPU (F10)",true);button(309,145,"Next frame (F4)",false);
    button(466,135,follow?"Follow PC: ON":"Follow PC: OFF",follow);button(613,105,"Go to SP",false);button(730,210,bank<0?"View: CPU map":"View: RAM bank "+std::to_string(bank),false);
    al_draw_filled_rounded_rectangle(20,96,620,257,5,5,al_map_rgb(23,31,43));al_draw_filled_rounded_rectangle(640,96,940,257,5,5,al_map_rgb(23,31,43));
    char text[160];al_draw_text(font,muted,32,106,0,"Z80 REGISTERS");
    snprintf(text,sizeof(text),"AF %04X    BC %04X    DE %04X    HL %04X",r.AF,r.BC,r.DE,r.HL);al_draw_text(font,white,32,130,0,text);
    snprintf(text,sizeof(text),"AF'%04X    BC'%04X    DE'%04X    HL'%04X",r.altAF,r.altBC,r.altDE,r.altHL);al_draw_text(font,white,32,152,0,text);
    snprintf(text,sizeof(text),"IX %04X    IY %04X    SP %04X    PC %04X",r.IX,r.IY,r.SP,r.PC);al_draw_text(font,white,32,174,0,text);
    snprintf(text,sizeof(text),"I  %02X  R %02X    IM %u    IFF1 %u  IFF2 %u",r.I,r.R,r.IM,r.iff1A,r.iff1B);al_draw_text(font,white,32,196,0,text);
    std::string flags;const char *names[]={"S","Z","5","H","3","P/V","N","C"};for(unsigned bit=0;bit<8;++bit){if(bit)flags+=' ';flags+=names[bit];flags+='=';flags+=(r.F&(128>>bit))?'1':'0';}
    al_draw_text(font,muted,32,222,0,flags.c_str());
    snprintf(text,sizeof(text),"Frame %llu  ULA %lu / %u",static_cast<unsigned long long>(machine.FrameCounter()),u.frameTStates,u.timing.TStatesPerFrame());al_draw_text(font,white,652,108,0,text);
    snprintf(text,sizeof(text),"Line %lu +%lu  CPU %.2f MHz",u.line,u.lineTStates,machine.Configuration().timing.cpuClockHz*machine.CPUMultiplier()/1000000.);al_draw_text(font,muted,652,130,0,text);
    snprintf(text,sizeof(text),"7FFD %02X  1FFD %02X  Screen RAM %u",machine.memory.PagingLatch(),machine.memory.PagingLatchPlus3(),machine.memory.ScreenBank());al_draw_text(font,muted,652,152,0,text);
    if(frameStep)snprintf(text,sizeof(text),"Last step: next frame");else snprintf(text,sizeof(text),"Last step: %u CPU T-states",lastCycles);al_draw_text(font,muted,652,174,0,text);
    snprintf(text,sizeof(text),"Tape %zu/%zu  %s",machine.deck.CurrentBlock(),machine.deck.Blocks().size(),machine.deck.IsPlaying()?"Playing":"Stopped / paused");al_draw_text(font,muted,652,196,0,text);
    snprintf(text,sizeof(text),"AY %s  Disk motor %s",machine.ula.ay.Enabled()?"on":"off",machine.io.floppy.state.motor?"on":"off");al_draw_text(font,muted,652,218,0,text);
    al_draw_text(font,muted,20,288,0,bank<0?"Address (hex)":"Offset (hex)");al_draw_filled_rectangle(126,278,201,305,al_map_rgb(23,31,43));
    al_draw_rectangle(126,278,201,305,badAddress?al_map_rgb(230,110,110):editing?cyan:muted,1);
    snprintf(text,sizeof(text),"%04X",address);al_draw_text(font,white,134,288,0,editing?addressText.c_str():text);
    al_draw_filled_rounded_rectangle(210,278,275,305,3,3,al_map_rgb(49,83,109));al_draw_text(font,white,242,288,ALLEGRO_ALIGN_CENTER,"Go");
    al_draw_text(font,muted,292,288,0,bank<0?"CPU address space (ROM + mapped RAM)":"Physical bank offsets 0000-3FFF");
    al_draw_text(font,muted,20,317,0,"ADDR  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   ASCII");
    for(unsigned row=0;row<16;++row){unsigned base=(address+row*16)&Mask();float y=340+row*16;snprintf(text,sizeof(text),"%04X",base);al_draw_text(font,muted,20,y,0,text);
      for(unsigned col=0;col<16;++col){unsigned addr=(base+col)&Mask();auto value=ReadByte(machine,bank,addr);auto color=bank<0 && addr==r.PC?cyan:bank<0 && addr==r.SP?purple:white;
        snprintf(text,sizeof(text),"%02X",value);al_draw_text(font,color,68+col*24,y,0,text);char ascii[]={value>=32 && value<127?char(value):'.',0};al_draw_text(font,muted,468+col*8,y,0,ascii);}
    }
    al_draw_text(font,muted,652,282,0,"STACK (CPU MAP)");
    for(unsigned row=0;row<10;++row){unsigned at=(r.SP+row*2)&65535;unsigned value=ReadByte(machine,-1,at)|(ReadByte(machine,-1,at+1)<<8);snprintf(text,sizeof(text),"%04X  %04X",at,value);al_draw_text(font,row?white:purple,652,308+row*18,0,text);}
    al_draw_text(font,muted,652,500,0,"ACTIVE MEMORY WINDOWS");
    for(unsigned slot=0;slot<4;++slot){if(!slot && !machine.memory.SpecialPaging())snprintf(text,sizeof(text),"0000-3FFF  ROM %u",machine.memory.ROMPage());else{
      unsigned mapped=slot?machine.memory.RAMBank(slot-1):((machine.memory.PagingLatchPlus3()&6)?4:0);snprintf(text,sizeof(text),"%04X-%04X  RAM %u",slot*16384,slot*16384+16383,mapped);}
      al_draw_text(font,white,652,520+slot*18,0,text);}
    al_draw_text(font,muted,20,612,0,"Wheel / arrows / PgUp / PgDn: memory. Home: follow PC. Cyan: PC. Purple: SP.");
    al_draw_text(font,muted,20,634,0,"F5 pause/resume  F10 instruction  F4 frame  F12/Esc close. Stepping keeps loader traps active.");
    al_flip_display();al_identity_transform(&t);al_use_transform(&t);
  }
private:
  ALLEGRO_DISPLAY *display=nullptr;ALLEGRO_FONT *font=nullptr;ALLEGRO_EVENT_QUEUE *events=nullptr;
  unsigned address=0,lastCycles=0;int bank=-1;bool follow=true,editing=false,badAddress=false,frameStep=false;std::string addressText;
  unsigned Mask() const{return bank<0?65535:16383;}
  float Scale() const{return std::max(0.01f,std::min(al_get_display_width(display)/960.f,al_get_display_height(display)/650.f));}
  void Scroll(int bytes){follow=false;address=(address+bytes)&Mask();}
  void Go(){if(!editing)return;if(addressText.empty()){editing=false;return;}unsigned value=std::stoul(addressText,nullptr,16);if(value>Mask()){badAddress=true;return;}address=value;follow=false;editing=false;badAddress=false;}
};
#endif
