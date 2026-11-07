#ifndef MONITOR_REFRESH_H
#define MONITOR_REFRESH_H
#include <allegro5/allegro.h>
#include <memory>
// Platform headers stay in the implementation to avoid leaking X11 macros.
double MonitorRefresh(ALLEGRO_DISPLAY *display);
class MonitorVSync {
public:
  MonitorVSync();
  ~MonitorVSync();
  bool Configure(ALLEGRO_DISPLAY *display,unsigned interval);
  void Restore();
private:
  class Implementation;
  std::unique_ptr<Implementation> implementation;
};
#endif
