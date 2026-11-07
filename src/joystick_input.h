#ifndef JOYSTICK_INPUT_H
#define JOYSTICK_INPUT_H
#include <allegro5/allegro.h>
#include <array>
#include <algorithm>
#include <string>
#include "ula.h"

enum class JoystickMode { Disabled, Kempston, Sinclair1, Sinclair2 };

class JoystickInput {
public:
  enum { Right=1, Left=2, Down=4, Up=8, Fire=16 };
  JoystickMode mode=JoystickMode::Disabled;
  bool keyboard=false;
  std::array<bool,ALLEGRO_KEY_MAX> keys{};
  bool KeyboardJoystick() const {return keyboard && mode!=JoystickMode::Disabled;}
  static unsigned Directions(float x,float y) {
    return (x>0.25f?Right:x< -0.25f?Left:0) | (y>0.25f?Down:y< -0.25f?Up:0);
  }
  void Gamepad(unsigned state) {
    state&=31;
    if(waitNeutral){if(!state)waitNeutral=false;pad=0;}else pad=state;
  }
  void SuspendGamepad(){pad=0;waitNeutral=true;}
  void Release() {keys.fill(false);pad=0;waitNeutral=true;}
  unsigned State() const {
    if(mode==JoystickMode::Disabled)return 0;
    unsigned state=pad;
    if(KeyboardJoystick())state|=(keys[ALLEGRO_KEY_RIGHT]?Right:0) | (keys[ALLEGRO_KEY_LEFT]?Left:0) |
        (keys[ALLEGRO_KEY_DOWN]?Down:0) | (keys[ALLEGRO_KEY_UP]?Up:0) | (keys[ALLEGRO_KEY_RCTRL]?Fire:0);
    // Conflicting sources must not produce impossible opposite directions.
    if((state&(Left|Right))==(Left|Right))state&=~(Left|Right);
    if((state&(Up|Down))==(Up|Down))state&=~(Up|Down);
    return state;
  }
  void ApplyKeyboard(ULA &ula) const {
    unsigned digits=0,state=State();
    if(mode==JoystickMode::Sinclair1) {
      if(state&Left)digits|=1<<6;if(state&Right)digits|=1<<7;
      if(state&Down)digits|=1<<8;if(state&Up)digits|=1<<9;if(state&Fire)digits|=1;
    }else if(mode==JoystickMode::Sinclair2) {
      if(state&Left)digits|=1<<1;if(state&Right)digits|=1<<2;
      if(state&Down)digits|=1<<3;if(state&Up)digits|=1<<4;if(state&Fire)digits|=1<<5;
    }
    bool left=!KeyboardJoystick() && keys[ALLEGRO_KEY_LEFT],right=!KeyboardJoystick() && keys[ALLEGRO_KEY_RIGHT];
    bool up=!KeyboardJoystick() && keys[ALLEGRO_KEY_UP],down=!KeyboardJoystick() && keys[ALLEGRO_KEY_DOWN];
    if(left)digits|=1<<5;if(down)digits|=1<<6;if(up)digits|=1<<7;if(right)digits|=1<<8;
    for(unsigned n=0;n<10;++n)ula.PressKey(0,n?n-1:9,keys[ALLEGRO_KEY_0+n] || (digits&(1<<n)));
    ula.PressKey(3,0,keys[ALLEGRO_KEY_LSHIFT] || left || right || up || down);
  }
private:
  unsigned pad=0;
  bool waitNeutral=true;
};

// Stable Allegro 5 API: works with both the Linux backend and the Windows SDK.
class JoystickHost {
public:
  struct Stick {
    unsigned axes;int flags;bool ignored;
    Stick(unsigned axes=0,int flags=0,bool ignored=false):axes(axes),flags(flags),ignored(ignored){}
  };
  static unsigned Movement(const ALLEGRO_JOYSTICK_STATE &state,const Stick *sticks,size_t count) {
    unsigned result=0;bool primary=false;
    count=std::min(count,sizeof(state.stick)/sizeof(state.stick[0]));
    for(size_t n=0;n<count;++n) {
      if(sticks[n].axes<2 || sticks[n].ignored)continue;
      // DIGITAL is a capability bit, not an exclusive stick type. DirectInput
      // marks ordinary analogue sticks DIGITAL | ANALOGUE as well.
      bool digitalOnly=(sticks[n].flags&ALLEGRO_JOYFLAG_DIGITAL) && !(sticks[n].flags&ALLEGRO_JOYFLAG_ANALOGUE);
      if(digitalOnly || !primary)result|=JoystickInput::Directions(state.stick[n].axis[0],state.stick[n].axis[1]);
      if(!digitalOnly)primary=true;
    }
    return result;
  }
  static std::string Lower(const char *name) {
    std::string result=name?name:"";for(auto &c:result)if(c>='A' && c<='Z')c+=32;return result;
  }
  static unsigned DpadButton(const char *name) {
    auto text=Lower(name);
    if(text.find("dpad")==std::string::npos && text.find("d-pad")==std::string::npos)return 0;
    if(text.find("right")!=std::string::npos)return JoystickInput::Right;
    if(text.find("left")!=std::string::npos)return JoystickInput::Left;
    if(text.find("down")!=std::string::npos)return JoystickInput::Down;
    if(text.find("up")!=std::string::npos)return JoystickInput::Up;
    return 0;
  }
  bool gamepad=true;
  int controller=-1; // Automatic, or one of the first four connected controllers.
  JoystickHost(){installed=al_install_joystick();}
  ~JoystickHost(){Drop();if(installed)al_uninstall_joystick();}
  bool Available() const {return installed;}
  void Reconfigure(){Drop();if(installed)al_reconfigure_joysticks();}
  unsigned Poll() {
    if(!installed || !gamepad){Drop();return 0;}
    int number=controller<0?0:controller;
    if(number>=al_get_num_joysticks()){Drop();return 0;}
    if(!device || number!=selected || !al_get_joystick_active(device)) {
      Drop();device=al_get_joystick(number);selected=number;
      if(device)Describe();
    }
    if(!device)return 0;
    ALLEGRO_JOYSTICK_STATE state{};al_get_joystick_state(device,&state);
    unsigned result=Movement(state,sticks.data(),stickCount);
    for(unsigned button=0;button<buttonCount;++button)if(state.button[button]) {
      result|=buttons[button];
      if(button<2 && !buttons[button])result|=JoystickInput::Fire;
    }
    return result;
  }
private:
  bool installed=false;
  ALLEGRO_JOYSTICK *device=nullptr;
  int selected=-1;
  std::array<Stick,16> sticks{};
  std::array<unsigned,32> buttons{};
  unsigned stickCount=0,buttonCount=0;
  void Describe() {
    stickCount=std::min<unsigned>(sticks.size(),al_get_joystick_num_sticks(device));
    for(unsigned n=0;n<stickCount;++n) {
      auto name=Lower(al_get_joystick_stick_name(device,n));
      bool ignored=name.find("trigger")!=std::string::npos || name.find("throttle")!=std::string::npos || name.find("slider")!=std::string::npos;
      sticks[n]=Stick(al_get_joystick_num_axes(device,n),al_get_joystick_stick_flags(device,n),ignored);
    }
    buttonCount=std::min<unsigned>(buttons.size(),al_get_joystick_num_buttons(device));
    for(unsigned n=0;n<buttonCount;++n)buttons[n]=DpadButton(al_get_joystick_button_name(device,n));
  }
  void Drop(){if(device)al_release_joystick(device);device=nullptr;selected=-1;stickCount=buttonCount=0;}
};
#endif
