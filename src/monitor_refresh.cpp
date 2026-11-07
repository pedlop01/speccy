#include "monitor_refresh.h"
#include <allegro5/allegro.h>
#include "platform.h"
#include <allegro5/allegro_opengl.h>
#ifdef _WIN32
#include <allegro5/allegro_windows.h>
#else
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#undef None
#endif

// Query the monitor containing the window, not the highest supported mode.
double MonitorRefresh(ALLEGRO_DISPLAY *display) {
#ifdef _WIN32
  auto window=al_get_win_window_handle(display);
  MONITORINFOEXW info={};info.cbSize=sizeof(info);
  DEVMODEW mode={};mode.dmSize=sizeof(mode);
  if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),reinterpret_cast<MONITORINFO *>(&info)) &&
      EnumDisplaySettingsExW(info.szDevice,ENUM_CURRENT_SETTINGS,&mode,0) && mode.dmDisplayFrequency>1)
    return mode.dmDisplayFrequency;
#else
  struct Connection {Display *value=XOpenDisplay(nullptr);~Connection(){if(value)XCloseDisplay(value);}} connection;
  if(!connection.value)return 0;
  int x,y;al_get_window_position(display,&x,&y);x+=al_get_display_width(display)/2;y+=al_get_display_height(display)/2;
  auto *resources=XRRGetScreenResourcesCurrent(connection.value,DefaultRootWindow(connection.value));
  if(!resources)return 0;double refresh=0;
  for(int n=0;n<resources->ncrtc;++n) {
    auto *crtc=XRRGetCrtcInfo(connection.value,resources,resources->crtcs[n]);if(!crtc)continue;
    if(crtc->mode && x>=crtc->x && y>=crtc->y && x<crtc->x+int(crtc->width) && y<crtc->y+int(crtc->height)) {
      for(int m=0;m<resources->nmode;++m){const auto &mode=resources->modes[m];
        if(mode.id==crtc->mode && mode.hTotal && mode.vTotal){refresh=double(mode.dotClock)/mode.hTotal/mode.vTotal;
          if(mode.modeFlags&RR_Interlace)refresh*=2;if(mode.modeFlags&RR_DoubleScan)refresh/=2;break;}}
    }
    XRRFreeCrtcInfo(crtc);if(refresh)break;
  }
  XRRFreeScreenResources(resources);if(refresh)return refresh;
#endif
  return al_get_display_refresh_rate(display);
}
// Set an integer swap interval at runtime where the graphics backend exposes
// it. Preserve the previous interval so disabling this option restores it.
class MonitorVSync::Implementation {
public:
  ~Implementation(){Restore();}
  bool Configure(ALLEGRO_DISPLAY *display,unsigned interval) {
    Restore();al_set_target_backbuffer(display);owner=display;if(!(al_get_display_flags(display)&ALLEGRO_OPENGL))return false;
#ifdef _WIN32
    using Set=BOOL (WINAPI *)(int);using Get=int (WINAPI *)();
    auto set=reinterpret_cast<Set>(al_get_opengl_proc_address("wglSwapIntervalEXT"));
    auto get=reinterpret_cast<Get>(al_get_opengl_proc_address("wglGetSwapIntervalEXT"));
    if(!set || !get)return false;oldInterval=get();setter=set;
    if(!setter(interval)){setter=nullptr;return false;}return true;
#else
    if(!al_have_opengl_extension("GLX_EXT_swap_control"))return false;
    using GetDisplay=void *(*)();using GetDrawable=unsigned long (*)();
    using Query=void (*)(void *,unsigned long,int,unsigned *);
    auto connection=reinterpret_cast<GetDisplay>(al_get_opengl_proc_address("glXGetCurrentDisplay"));
    auto drawable=reinterpret_cast<GetDrawable>(al_get_opengl_proc_address("glXGetCurrentDrawable"));
    auto query=reinterpret_cast<Query>(al_get_opengl_proc_address("glXQueryDrawable"));
    setter=reinterpret_cast<Set>(al_get_opengl_proc_address("glXSwapIntervalEXT"));
    if(!connection || !drawable || !query || !setter){setter=nullptr;return false;}
    nativeDisplay=connection();nativeDrawable=drawable();unsigned value=0;
    if(!nativeDisplay || !nativeDrawable){setter=nullptr;return false;}
    query(nativeDisplay,nativeDrawable,0x20F1,&value);oldInterval=value; // GLX_SWAP_INTERVAL_EXT
    setter(nativeDisplay,nativeDrawable,interval);return true;
#endif
  }
  void Restore() {
    if(!setter)return;
    if(owner)al_set_target_backbuffer(owner);
#ifdef _WIN32
    setter(oldInterval);
#else
    setter(nativeDisplay,nativeDrawable,oldInterval);
#endif
    setter=nullptr;
  }
private:
  ALLEGRO_DISPLAY *owner=nullptr;
  unsigned oldInterval=0;
#ifdef _WIN32
  using Set=BOOL (WINAPI *)(int);Set setter=nullptr;
#else
  using Set=void (*)(void *,unsigned long,int);Set setter=nullptr;
  void *nativeDisplay=nullptr;unsigned long nativeDrawable=0;
#endif
};

MonitorVSync::MonitorVSync():implementation(new Implementation) {}
MonitorVSync::~MonitorVSync() {}
bool MonitorVSync::Configure(ALLEGRO_DISPLAY *display,unsigned interval){return implementation->Configure(display,interval);}
void MonitorVSync::Restore(){implementation->Restore();}
