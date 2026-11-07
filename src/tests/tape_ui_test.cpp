// Optional X11 integration test: real pointer events through the application loop.
#include <allegro5/allegro_native_dialog.h>
#include <atomic>
#include "catalog_art_fixture.h"
static std::atomic<unsigned> menuFlagWrites(0);
static std::atomic<int> observedMenuFlags[640];
static void RecordMenuFlags(ALLEGRO_MENU *menu, int id, int flags) {
  ++menuFlagWrites;
  if (id >= 0 && id < 640) observedMenuFlags[id].store(flags);
  al_set_menu_item_flags(menu, id, flags);
}
static std::atomic<ALLEGRO_DISPLAY *> applicationDisplay(nullptr);
static std::atomic<bool> delayedMenuResize(false);
static std::atomic<int> renderedWidth(0),renderedHeight(0);
static ALLEGRO_DISPLAY *RecordDisplay(int width,int height) {
  auto *display=al_create_display(width,height);ALLEGRO_DISPLAY *empty=nullptr;
  applicationDisplay.compare_exchange_strong(empty,display);return display;
}
static void RecordFrame() {
  auto *display=al_get_current_display();
  if(display==applicationDisplay.load()) {
    // Reproduce GTK's late menu ConfigureNotify after startup's initial resize.
    static unsigned startupFrames = 0;
    if (++startupFrames == 8) {
      al_resize_display(display, 640, 451);
      delayedMenuResize.store(true);
    }
    renderedWidth.store(al_get_display_width(display));renderedHeight.store(al_get_display_height(display));
  }
  al_flip_display();
}
#define al_create_display RecordDisplay
#define al_flip_display RecordFrame
#define al_set_menu_item_flags RecordMenuFlags
#define main GomasApplicationMain
#include "../main.cpp"
#undef main
#undef al_create_display
#undef al_flip_display
#undef al_set_menu_item_flags
#include <X11/Xlib.h>
#include <sys/stat.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <chrono>
#include <thread>
#include <functional>
#include <stdexcept>

static void Wait(int milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

static Window FindWindow(Display *x, Window root, const char *title) {
  char *name = nullptr;
  if (XFetchName(x, root, &name) && name) {
    bool found = std::string(name) == title;
    XFree(name);
    if (found) return root;
  }
  Window parent, realRoot, *children = nullptr;
  unsigned count = 0;
  if (!XQueryTree(x, root, &realRoot, &parent, &children, &count)) return 0;
  Window result = 0;
  for (unsigned i = 0; i < count && !result; ++i) result = FindWindow(x, children[i], title);
  if (children) XFree(children);
  return result;
}

static bool Until(const std::function<bool()> &predicate, int milliseconds) {
  for (int elapsed = 0; elapsed < milliseconds; elapsed += 10) {
    if (predicate()) return true;
    Wait(10);
  }
  return false;
}

static void Menu(int id) {
  ALLEGRO_EVENT event = {};
  event.user.type = ALLEGRO_EVENT_MENU_CLICK;
  event.user.data1 = id;
  al_emit_user_event(al_get_default_menu_event_source(), &event, nullptr);
}

static unsigned Pixel(Display *x, Window window, int px, int py) {
  XImage *image = XGetImage(x, window, px, py, 1, 1, AllPlanes, ZPixmap);
  if (!image) throw std::runtime_error("Cannot read tape window");
  unsigned value = XGetPixel(image, 0, 0) & 0xFFFFFF;
  XDestroyImage(image);
  return value;
}

static void Click(Display *x, Window window, int px, int py) {
  Window child;
  int rootX, rootY;
  XTranslateCoordinates(x, window, DefaultRootWindow(x), px, py, &rootX, &rootY, &child);
  XTestFakeMotionEvent(x, -1, rootX, rootY, 0);
  XTestFakeButtonEvent(x, 1, True, 0);
  XTestFakeButtonEvent(x, 1, False, 0);
  XFlush(x);
}

static void Check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}

static void CursorKeys() {
  ULA ula;
  auto key=[&](int code,bool down){ALLEGRO_EVENT event={};event.type=down?ALLEGRO_EVENT_KEY_DOWN:ALLEGRO_EVENT_KEY_UP;event.keyboard.keycode=code;ReadKeyboard(event,&ula);};
  const int arrows[]={ALLEGRO_KEY_LEFT,ALLEGRO_KEY_DOWN,ALLEGRO_KEY_UP,ALLEGRO_KEY_RIGHT};
  for(unsigned n=0;n<4;++n){key(arrows[n],true);auto state=ula.CaptureState();
    Check(state.keys[0]==1,"Cursor did not hold CAPS SHIFT");
    Check(state.keys[3]==(n==0?16:0) && state.keys[4]==(n==0?0:1u<<(5-n)),"Cursor digit mapping is wrong");
    key(arrows[n],false);state=ula.CaptureState();Check(!state.keys[0] && !state.keys[3] && !state.keys[4],"Cursor release left a key pressed");}
  key(ALLEGRO_KEY_LSHIFT,true);key(ALLEGRO_KEY_UP,true);key(ALLEGRO_KEY_DOWN,true);key(ALLEGRO_KEY_UP,false);
  Check(ula.CaptureState().keys[0]==1 && ula.CaptureState().keys[4]==16,"Overlapping cursor release lost Shift or Down");
  key(ALLEGRO_KEY_DOWN,false);Check(ula.CaptureState().keys[0]==1,"Arrow release lost physical Shift");
  key(ALLEGRO_KEY_5,true);key(ALLEGRO_KEY_LEFT,true);key(ALLEGRO_KEY_LEFT,false);
  Check(ula.CaptureState().keys[3]==16,"Arrow release lost physical digit 5");
  ReleaseKeys(ula);key(ALLEGRO_KEY_RIGHT,true);key(ALLEGRO_KEY_RIGHT,false);
  auto state=ula.CaptureState();for(auto row:state.keys)Check(!row,"Focus release left host keys held");
}
int main() {
  CursorKeys();
  XInitThreads();
  // Never read or overwrite the user's actual last-ROM preference.
  char configPath[] = "/tmp/gomas-ui-settings-XXXXXX";
  int configFd = mkstemp(configPath);
  if (configFd < 0) return 1;
  close(configFd);
  { std::ofstream config(configPath);
    config << "[view]\nsize_version=1\nwidth=640\nheight=451\nfilter=2\nfullscreen=0\n"; }
  const char *oldConfig = getenv("GOMAS_CONFIG");
  bool hadConfig = oldConfig != nullptr;
  std::string savedConfig = oldConfig ? oldConfig : "";
  setenv("GOMAS_CONFIG", configPath, 1);
  char savesPath[] = "/tmp/gomas-ui-saves-XXXXXX";
  if (!mkdtemp(savesPath)) return 1;
  const char *oldSaves = getenv("GOMAS_SAVE_DIR");
  bool hadSaves = oldSaves != nullptr;
  std::string savedSaves = oldSaves ? oldSaves : "";
  setenv("GOMAS_SAVE_DIR", savesPath, 1);
  std::string gamesPath=std::string(savesPath)+"/games";
  const char *oldGames=getenv("GOMAS_GAMES_DIR");bool hadGames=oldGames!=nullptr;std::string savedGamesDir=oldGames?oldGames:"";
  setenv("GOMAS_GAMES_DIR",gamesPath.c_str(),1);
  bool passed = false;
  std::string failure;
  std::vector<std::string> cachedArtwork;
  std::thread input([&]() {
    Display *x = XOpenDisplay(nullptr);
    if (!x) { failure = "test-ui requires an X11 display"; return; }
    Window mainWindow = 0, deck = 0;
    try {
      Check(Until([&]() {
        mainWindow = FindWindow(x, DefaultRootWindow(x), "Gomas - ZX Spectrum 48K - cobra.tap");
        return mainWindow != 0;
      }, 5000), "Main window did not open");
      // The title is set immediately before the event loop starts.
      Wait(100);
      // A settled menu must not be rebuilt by every refresh tick. Allegro's
      // CHECKBOX flag is immutable; requesting it on plain items loops forever.
      Check(Until([&]() { return renderedWidth.load() == 640 && renderedHeight.load() == 480 &&
          ScreenView::Viewport(renderedWidth.load(), renderedHeight.load()).scale == 2 &&
          (observedMenuFlags[MENU_FILTER_SCANLINES_STRONG].load() & ALLEGRO_MENU_ITEM_CHECKED); }, 2000),
          "Saved 2x window/filter did not restore at startup (native menu reduced the drawable area)");
      Check(Until([&]() { return delayedMenuResize.load(); }, 2000), "Delayed menu resize was not injected");
      Wait(1200); // Allow the late GTK configure and size reconciliation to settle.
      Check(renderedWidth.load() == 640 && renderedHeight.load() == 480,
          "Native menu shrank the restored 2x window after mapping");
      Menu(MENU_FULLSCREEN);
      Check(Until([&]() { return observedMenuFlags[MENU_FULLSCREEN].load() & ALLEGRO_MENU_ITEM_CHECKED; }, 2000),
          "Fullscreen did not activate");
      Menu(MENU_FULLSCREEN);
      Check(Until([&]() { return renderedWidth.load() == 640 && renderedHeight.load() == 480; }, 2000),
          "Fullscreen did not restore the saved window dimensions");
      Menu(MENU_FILTER_NONE); Wait(150);
      unsigned settledWrites = menuFlagWrites.load();
      Wait(250);
      Check(menuFlagWrites.load() == settledWrites, "Idle refresh keeps rebuilding the native ROM menu");
      auto checked = [&](int id) { return (observedMenuFlags[id].load() & ALLEGRO_MENU_ITEM_CHECKED) != 0; };
      auto speedIs = [&](double speed) {
        char *name = nullptr;
        if (!XFetchName(x, mainWindow, &name) || !name) return false;
        std::string title(name); XFree(name);
        std::string label = speed == 0.25 ? "0.25x" : speed == 0.5 ? "0.5x" : std::to_string(static_cast<unsigned>(speed)) + "x";
        return speed == 1 ? title.find("[Speed ") == std::string::npos :
            title.find("[Speed " + label + "]") != std::string::npos;
      };
      XRaiseWindow(x, mainWindow); XSetInputFocus(x, mainWindow, RevertToParent, CurrentTime);
      auto shortcut = [&](KeySym symbol) {
        XTestFakeKeyEvent(x, XKeysymToKeycode(x, symbol), True, 0);
        XTestFakeKeyEvent(x, XKeysymToKeycode(x, symbol), False, 0); XFlush(x);
      };
      Menu(MENU_PAUSE);
      Check(Until([&]() { return checked(MENU_PAUSE); }, 1000), "CPU Pause did not respond");
      SaveStates::Store saves(savesPath);
      shortcut(XK_F2);
      Check(Until([&]() { return saves.Exists(0); }, 2000), "F2 did not save the quick slot");
      Menu(MENU_SAVE_STATE);
      Window gallery=0;
      Check(Until([&]() { gallery=FindWindow(x,DefaultRootWindow(x),"Gomas - Save game");return gallery!=0; },2000), "Save gallery did not open");
      Wait(200);
      Click(x,gallery,125,150); // Reserved quick slot cannot be overwritten here.
      Click(x,gallery,830,660);Wait(150);
      Check(FindWindow(x,DefaultRootWindow(x),"Gomas - Save game")!=0,"Normal save overwrote reserved quick slot");
      Click(x,gallery,825,155); // Slot 3, different from the initial selection.
      Check(Until([&]() { return Pixel(x,gallery,718,80)==0x223444; },750),"Mouse did not select save slot 3");
      Click(x,gallery,825,155); // Second click saves directly.
      Check(Until([&]() { return saves.Exists(3) && !FindWindow(x,DefaultRootWindow(x),"Gomas - Save game"); },2000),"Selected slot was not saved");
      Menu(MENU_CPU_CLOCK_4X);
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_4X); },1000),"Could not change CPU frequency before gallery load");
      Menu(MENU_LOAD_STATE);
      Check(Until([&]() { gallery=FindWindow(x,DefaultRootWindow(x),"Gomas - Load game");return gallery!=0; },2000),"Load gallery did not open");
      Wait(200);Click(x,gallery,825,155);Wait(70);Click(x,gallery,825,155);
      Check(Until([&]() { return !FindWindow(x,DefaultRootWindow(x),"Gomas - Load game"); },2000),"Selected slot did not load");
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_1X); },1000),"Gallery load did not restore saved CPU frequency");
      Menu(MENU_LOAD_STATE);
      Check(Until([&]() { gallery=FindWindow(x,DefaultRootWindow(x),"Gomas - Load game");return gallery!=0; },2000),"Load gallery did not reopen");
      XSetInputFocus(x,gallery,RevertToParent,CurrentTime);shortcut(XK_Escape);
      Check(Until([&]() { return !FindWindow(x,DefaultRootWindow(x),"Gomas - Load game"); },1000),"Escape did not cancel gallery");
      Menu(MENU_CPU_CLOCK_4X);
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_4X); },1000),"Could not change CPU frequency before quick load");
      XSetInputFocus(x,mainWindow,RevertToParent,CurrentTime);shortcut(XK_F3);
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_1X); },1000),"F3 did not restore the quick slot");
      Check(checked(MENU_PAUSE),"Loading a saved game changed CPU pause");
      // Loading starts a new rewind timeline; advance before the existing F9 test.
      Menu(MENU_PAUSE);
      Check(Until([&]() { return !checked(MENU_PAUSE); },1000),"Could not resume after loading");
      Wait(300);Menu(MENU_PAUSE);
      Check(Until([&]() { return checked(MENU_PAUSE); },1000),"Could not pause after loading");
      Menu(MENU_SPEED_UP);
      Check(Until([&]() { return speedIs(2) && checked(MENU_PAUSE); }, 1000), "Speed up changed pause");
      Menu(MENU_SPEED_UP);
      Check(Until([&]() { return speedIs(4); }, 1000), "Second speed-up click did not select 4x");
      Menu(MENU_SPEED_DOWN);
      Check(Until([&]() { return speedIs(2); }, 1000), "Speed-down menu did not select 2x");
      Menu(MENU_CPU_CLOCK_4X);
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_4X); }, 1000), "CPU frequency did not change");
      shortcut(XK_F7);
      Check(Until([&]() { return speedIs(1) && checked(MENU_CPU_CLOCK_4X); }, 1000),
          "F7 should lower speed without rewinding CPU state");
      auto rewinding = [&]() {
        char *name = nullptr;
        if (!XFetchName(x, mainWindow, &name) || !name) return false;
        bool active = std::string(name).find("[Rewinding]") != std::string::npos;
        XFree(name); return active;
      };
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_F9), True, 0); XFlush(x);
      Check(Until(rewinding, 1000), "Held F9 did not start reverse playback");
      Wait(150);
      Check(rewinding(), "Rewind stopped before F9 was released");
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_F9), False, 0); XFlush(x);
      Check(Until([&]() { return !rewinding(); }, 1000), "F9 release did not stop rewind");
      Check(Until([&]() { return checked(MENU_CPU_CLOCK_1X) && checked(MENU_PAUSE); }, 1000),
          "F9 did not rewind the CPU clock while preserving pause");
      Menu(MENU_SPEED_UP);
      Check(Until([&]() { return speedIs(2); }, 1000), "Could not increase speed while paused");
      Menu(MENU_SPEED_NORMAL);
      Check(Until([&]() { return speedIs(1); }, 1000), "Normal-speed menu did not restore 1x");
      shortcut(XK_F5);
      Check(Until([&]() { return !checked(MENU_PAUSE); }, 1000), "F5 did not resume the machine");
      for (unsigned speed : {2u, 4u, 8u, 16u}) {
        shortcut(XK_F6);
        Check(Until([&]() { return speedIs(speed); }, 1000), "F6 multiplier ladder failed");
      }
      Check(Until([&]() { return (observedMenuFlags[MENU_SPEED_UP].load() & ALLEGRO_MENU_ITEM_DISABLED) != 0; }, 1000),
          "Speed-up menu was not disabled at 16x");
      shortcut(XK_F6); Wait(100); Check(speedIs(16), "F6 exceeded 16x");
      shortcut(XK_F7);
      Check(Until([&]() { return speedIs(8); }, 1000), "F7 did not halve 16x to 8x");
      shortcut(XK_F8);
      Check(Until([&]() { return speedIs(1); }, 1000), "F8 did not restore 1x directly");
      shortcut(XK_F7);
      Check(Until([&]() { return speedIs(0.5); }, 1000), "F7 did not slow playback to 0.5x");
      shortcut(XK_F7);
      Check(Until([&]() { return speedIs(0.25); }, 1000), "F7 did not slow playback to 0.25x");
      Check(Until([&]() { return (observedMenuFlags[MENU_SPEED_DOWN].load() & ALLEGRO_MENU_ITEM_DISABLED) != 0; }, 1000),
          "Speed-down menu was not disabled at 0.25x");
      shortcut(XK_F7); Wait(100); Check(speedIs(0.25), "F7 went below 0.25x");
      shortcut(XK_F6);
      Check(Until([&]() { return speedIs(0.5); }, 1000), "F6 did not raise slow playback to 0.5x");
      shortcut(XK_F8);
      Check(Until([&]() { return speedIs(1); }, 1000), "F8 did not restore 1x from slow playback");
      // GTK's toplevel also includes a menu whose height can change after
      // mapping. Measure the rendered Allegro area, which View actually scales.
      auto size = [&]() {return std::make_pair(renderedWidth.load(),renderedHeight.load());};
      Menu(MENU_VIEW_2X);
      Check(Until([&]() { return checked(MENU_VIEW_2X) && size().first == 640; }, 2000), "2x size was not applied");
      Wait(100); // Wait for a frame at the requested size.
      const auto originalSize = size();
      Menu(MENU_VIEW_4X);
      const bool resized4x = Until([&]() { return checked(MENU_VIEW_4X) && size().first == 1280 &&
          size().second == originalSize.second + 480; }, 2000);
      if (!resized4x) fprintf(stderr, "View geometry: initial %dx%d, actual %dx%d, flags=%d\n",
          originalSize.first, originalSize.second, size().first, size().second, observedMenuFlags[MENU_VIEW_4X].load());
      Check(resized4x, "4x size/proportions were not applied");
      for (int id : {MENU_FILTER_SCANLINES_LIGHT, MENU_FILTER_SCANLINES_STRONG, MENU_FILTER_GRID, MENU_FILTER_NONE}) {
        Menu(id);
        Check(Until([&]() { return checked(id); }, 1000), "View filter selection did not respond");
      }
      Menu(MENU_VIEW_2X);
      Check(Until([&]() { return size() == originalSize && checked(MENU_VIEW_2X); }, 2000), "Could not restore 2x");
      Menu(MENU_FULLSCREEN);
      Check(Until([&]() { return checked(MENU_FULLSCREEN) && size() != originalSize; }, 2000), "Fullscreen did not activate");
      XRaiseWindow(x, mainWindow);
      XSetInputFocus(x, mainWindow, RevertToParent, CurrentTime);
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_Escape), True, 0);
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_Escape), False, 0); XFlush(x);
      Check(Until([&]() { return !checked(MENU_FULLSCREEN) && size() == originalSize; }, 2000),
          "Escape did not restore the window size from fullscreen");
      XSetInputFocus(x, mainWindow, RevertToParent, CurrentTime);
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_F11), True, 0);
      XTestFakeKeyEvent(x, XKeysymToKeycode(x, XK_F11), False, 0); XFlush(x);
      Check(Until([&]() { return checked(MENU_FULLSCREEN) && size() != originalSize; }, 2000), "F11 did not enter fullscreen");
      Menu(MENU_VIEW_2X);
      Check(Until([&]() { return !checked(MENU_FULLSCREEN) && size() == originalSize; }, 2000),
          "Choosing 2x did not leave fullscreen");
      Wait(100);
      settledWrites = menuFlagWrites.load(); Wait(250);
      Check(menuFlagWrites.load() == settledWrites, "View menu flags keep rebuilding after a mode change");
      // Exercise the ROM selector through its real native file dialog.
      Menu(MENU_LOAD_ROM);
      Window chooser = 0;
      Check(Until([&]() {
        chooser = FindWindow(x, DefaultRootWindow(x), "Load system ROM");
        return chooser != 0;
      }, 5000), "ROM selector did not open");
      XSetInputFocus(x, chooser, RevertToParent, CurrentTime);
      auto key = [&](KeySym symbol, bool down) { XTestFakeKeyEvent(x, XKeysymToKeycode(x, symbol), down, 0); };
      key(XK_Control_L, true); key('l', true); key('l', false); key(XK_Control_L, false);
      XFlush(x); Wait(100);
      key(XK_Control_L, true); key('a', true); key('a', false); key(XK_Control_L, false);
      char absoluteROM[4096];
      Check(realpath("../roms/48.rom", absoluteROM) != nullptr, "Cannot locate ROM fixture");
      for (const char *c = absoluteROM; *c; ++c) { key(*c, true); key(*c, false); }
      key(XK_Return, true); key(XK_Return, false); XFlush(x);
      Check(Until([&]() { return FindWindow(x, DefaultRootWindow(x), "Load system ROM") == 0; }, 5000),
          "ROM selection did not complete");
      Wait(100);
      Check(FindWindow(x, DefaultRootWindow(x), "Gomas - ZX Spectrum 48K - cobra.tap") != 0,
          "ROM selection lost the mounted tape or selected the wrong model");
      Menu(MENU_START_48);
      Check(Until([&]() { return !ROMPreferences::Read(configPath).restoreLast; }, 750),
          "The 48K startup preference was not saved");
      Menu(MENU_START_LAST);
      Check(Until([&]() { return ROMPreferences::Read(configPath).restoreLast; }, 750),
          "The last-ROM startup preference was not saved");
      Check(!ROMPreferences::Read(configPath).lastROM.empty(), "The chosen ROM was not remembered");
      for (int id : {MENU_ROM_128_EN, MENU_ROM_128_ES, MENU_ROM_PLUS2, MENU_ROM_PLUS3}) {
        const auto &profile = MenuROMProfile(id);
        Menu(id);
        Check(Until([&]() {
          return FindWindow(x, DefaultRootWindow(x), (std::string("Gomas - ") + profile.name + " - cobra.tap").c_str()) != 0;
        }, 2000), "128K/+2 ROM menu did not switch model and preserve the tape");
        Check(!ROMPreferences::Read(configPath).lastROM.empty(), "128K ROM was not remembered");
      }
      Menu(MENU_ROM_48);
      Check(Until([&]() { return FindWindow(x, DefaultRootWindow(x), "Gomas - ZX Spectrum 48K - cobra.tap") != 0; }, 2000),
          "Could not switch back to 48K");
      Wait(100);
      Menu(MENU_FAST_LOAD);
      Check(Until([&]() {
        deck = FindWindow(x, DefaultRootWindow(x), "Gomas - Tape deck");
        return deck != 0;
      }, 5000), "Tape deck did not open");
      XRaiseWindow(x, deck);
      XSetInputFocus(x, deck, RevertToParent, CurrentTime);
      XFlush(x);
      Check(Until([&]() { return Pixel(x, deck, 25, 85) == 0x313E4D; }, 2000),
          "Play button did not become ready");

      // Burst of genuine X11 motion events, followed by a genuine click.
      Window child;
      int rootX, rootY;
      XTranslateCoordinates(x, deck, DefaultRootWindow(x), 200, 160, &rootX, &rootY, &child);
      for (int i = 0; i < 1000; ++i) XTestFakeMotionEvent(x, -1, rootX + i % 40, rootY + i % 20, 0);
      XSync(x, False);
      auto start = std::chrono::steady_clock::now();
      Click(x, deck, 75, 96); // Play
      Check(Until([&]() { return Pixel(x, deck, 25, 85) == 0x20252D; }, 750),
          "Play click was lost or delayed behind pointer motion");
      double latency = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
      Wait(200);
      Click(x, deck, 208, 96); // Pause: Play becomes enabled again.
      Check(Until([&]() { return Pixel(x, deck, 25, 85) == 0x313E4D; }, 750), "Pause click was not processed");
      Click(x, deck, 342, 96); // Stop: progress returns to zero.
      Check(Until([&]() { return Pixel(x, deck, 20, 580) == 0x313E4D; }, 750), "Stop did not rewind the block");
      // The top padding belongs to this row, not the previous one. Regresses
      // the hit-test origin being offset from the painted row rectangle.
      Click(x, deck, 117, 185); // Upper padding of the second row.
      Check(Until([&]() { return Pixel(x, deck, 333, 188) == 0x2B415B; }, 750),
          "Clicking the top of a row selected its predecessor");
      // Ordinary clicks on the label must select and retain the row too.
      Click(x, deck, 117, 166); // First block label.
      Check(Until([&]() { return Pixel(x, deck, 333, 160) == 0x2B415B; }, 750),
          "Clicking the first block label did not select it");
      Click(x, deck, 117, 193); // Second block label.
      Check(Until([&]() { return Pixel(x, deck, 333, 188) == 0x2B415B; }, 750),
          "Clicking the second block label did not select it");
      Wait(200);
      Check(Pixel(x, deck, 333, 188) == 0x2B415B, "The selected row was lost during redraw");
      // Selection remains available with instant loading enabled.
      Menu(MENU_FAST_LOAD);
      Wait(100);
      Click(x, deck, 117, 166);
      Check(Until([&]() { return Pixel(x, deck, 333, 160) == 0x2B415B; }, 750),
          "Block selection failed with Fast load on");
      Menu(MENU_FAST_LOAD);
      Wait(100);
      Click(x, deck, 117, 240); // Upper padding of the fourth row.
      Check(Until([&]() { return Pixel(x, deck, 333, 243) == 0x2B415B; }, 750), "Block selection did not respond");
      Click(x, deck, 608, 96); // Cue selected.
      Check(Until([&]() {
        XImage *image = XGetImage(x, deck, 15, 245, 40, 12, AllPlanes, ZPixmap);
        if (!image) return false;
        bool accent = false;
        for (int y = 0; y < 12; ++y)
          for (int px = 0; px < 40; ++px) accent |= (XGetPixel(image, px, y) & 0xFFFFFF) == 0x52D0B3;
        XDestroyImage(image);
        return accent;
      }, 750), "Cue selected did not move the current-block marker");
      printf("Saved games: quick save/load, gallery selection/load/cancel and reserved slot; CPU: pause, speed, clock and rewind; View: 2x/4x, filters, fullscreen/F11/Escape; mouse: 1000 motion events then Play in %.1f ms; Pause, Stop, selection, Cue and native ROM selector passed\n", latency);
      // Open an external snapshot through the native dialog, including auto-model selection.
      std::string snapshotPath=std::string(savesPath)+"/fixture.z80";
      std::vector<unsigned char> snapshot(55);snapshot[30]=23;snapshot[33]=0x80;snapshot[34]=3;
      for(unsigned bank=0;bank<8;++bank) {
        snapshot.insert(snapshot.end(),{255,255,static_cast<unsigned char>(bank+3)});
        size_t start=snapshot.size();snapshot.resize(start+16384,bank);
        if(bank==2){snapshot[start]=0xc3;snapshot[start+1]=0;snapshot[start+2]=0x80;}
      }
      {std::ofstream file(snapshotPath,std::ios::binary);file.write(reinterpret_cast<const char *>(snapshot.data()),snapshot.size());}
      Menu(MENU_OPEN);
      Check(Until([&]() { chooser=FindWindow(x,DefaultRootWindow(x),"Open Spectrum game");return chooser!=0; },5000),"Game selector did not open");
      XSetInputFocus(x,chooser,RevertToParent,CurrentTime);
      key(XK_Control_L,true);key('l',true);key('l',false);key(XK_Control_L,false);XFlush(x);Wait(100);
      key(XK_Control_L,true);key('a',true);key('a',false);key(XK_Control_L,false);
      for(unsigned char c:snapshotPath){bool upper=c>='A' && c<='Z';if(upper)key(XK_Shift_L,true);key(c,true);key(c,false);if(upper)key(XK_Shift_L,false);}
      key(XK_Return,true);key(XK_Return,false);XFlush(x);
      const auto &snapshotProfile=MenuROMProfile(MENU_ROM_128_EN);
      Check(Until([&]() {return FindWindow(x,DefaultRootWindow(x),(std::string("Gomas - ")+snapshotProfile.name+" - fixture.z80").c_str())!=0;},5000),
          "Native snapshot selection did not restore the 128K model");
      Check(Until([&]() {return !(observedMenuFlags[MENU_CLOSE].load()&ALLEGRO_MENU_ITEM_DISABLED);},1000),"Close was disabled for a snapshot");
      Menu(MENU_CLOSE);
      Check(Until([&]() {return FindWindow(x,DefaultRootWindow(x),(std::string("Gomas - ")+snapshotProfile.name).c_str())!=0;},2000),"Close did not close the snapshot");
      // Disk controls have their own window; tape controls remain available.
      Disks::Image image;image.cylinders=1;image.heads=1;image.tracks.resize(1);
      image.tracks[0].sectors.push_back({0,0,1,2,0,0,std::vector<unsigned char>(512)});
      std::string diskPath=std::string(savesPath)+"/fixture.dsk";auto diskBytes=image.Encode();
      {std::ofstream file(diskPath,std::ios::binary);file.write(reinterpret_cast<const char *>(diskBytes.data()),diskBytes.size());}
      auto enterPath=[&](const std::string &path){
        key(XK_Control_L,true);key('l',true);key('l',false);key(XK_Control_L,false);XFlush(x);Wait(100);
        key(XK_Control_L,true);key('a',true);key('a',false);key(XK_Control_L,false);
        for(unsigned char c:path){bool upper=c>='A' && c<='Z';if(upper)key(XK_Shift_L,true);key(c,true);key(c,false);if(upper)key(XK_Shift_L,false);}
        XFlush(x);Wait(200); // GTK resolves the typed location asynchronously.
        key(XK_Return,true);key(XK_Return,false);XFlush(x);
      };
      Menu(MENU_DISK_A_INSERT);
      Check(Until([&](){chooser=FindWindow(x,DefaultRootWindow(x),"Insert disk image");return chooser!=0;},5000),"Disk insert chooser did not open");
      Wait(200);XSetInputFocus(x,chooser,RevertToParent,CurrentTime);enterPath(diskPath);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Insert disk image");},5000),"Disk selection did not complete");
      Window drives=0;
      Check(Until([&](){drives=FindWindow(x,DefaultRootWindow(x),"Gomas - Disk drives");return drives!=0;},5000),"Disk drives window did not open");
      Wait(150);Click(x,drives,585,137);
      Check(Until([&](){return checked(MENU_DISK_A_PROTECT);},1000),"Disk write-protection click did not respond");
      Click(x,drives,257,137);
      Check(Until([&](){return (observedMenuFlags[MENU_DISK_A_EJECT].load()&ALLEGRO_MENU_ITEM_DISABLED)!=0;},1000),"Disk eject click did not respond");
      // Deferred TZX choices can be cancelled and reopened from Tape.
      std::vector<unsigned char> route={'Z','X','T','a','p','e','!',26,1,20,0x28,13,0,2,1,0,3,'O','n','e',3,0,3,'T','w','o',0x12,100,0,255,255,0x23,2,0,0x12,200,0,255,255,0x20,0,0};
      std::string routePath=std::string(savesPath)+"/route.tzx";
      {std::ofstream file(routePath,std::ios::binary);file.write(reinterpret_cast<const char *>(route.data()),route.size());}
      Menu(MENU_OPEN);
      Check(Until([&](){chooser=FindWindow(x,DefaultRootWindow(x),"Open Spectrum game");return chooser!=0;},5000),"TZX chooser did not open");
      Wait(200);XSetInputFocus(x,chooser,RevertToParent,CurrentTime);enterPath(routePath);
      Window choice=0;
      Check(Until([&](){choice=FindWindow(x,DefaultRootWindow(x),"Gomas - Choose TZX route");return choice!=0;},5000),"TZX route dialog did not open");
      XSetInputFocus(x,choice,RevertToParent,CurrentTime);shortcut(XK_Escape);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Gomas - Choose TZX route");},1000),"TZX cancel did not respond");
      Menu(MENU_TAPE_CHOICE);
      Check(Until([&](){choice=FindWindow(x,DefaultRootWindow(x),"Gomas - Choose TZX route");return choice!=0;},1000),"TZX route could not be reopened");
      Wait(150);Click(x,choice,80,90);Wait(70);Click(x,choice,80,90);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Gomas - Choose TZX route");},1000),"TZX double-click did not select a route");
      Menu(MENU_QUICK_SAVE);
      Check(Until([&](){SaveStates::Record saved;std::string error;return saves.Read(0,saved,error) && saved.core.deck.choices==std::vector<unsigned>{1};},2000),"Saved game lost the selected TZX route");
      unlink(diskPath.c_str());unlink(routePath.c_str());
      puts("Native media: disk insert/protect/eject and TZX choose/cancel/reopen/save passed");
      unlink(snapshotPath.c_str());
      puts("Native File > Open: Z80 snapshot and automatic 128K model selection passed");
      // The native catalog test uses a seeded local library and never contacts ZXArt.
      auto gameBytes=GameCatalog::Read("cobra.tap",16*1024*1024);
      GameCatalog::Json gameRow={{"id",7},{"title","Cobra"},{"year",1986},{"legalStatus","allowed"}};
      gameRow["imagesUrls"]={"https://zxart.ee/zximages/one","https://zxart.ee/screenshot/two"};
      GameCatalog::Json releaseRow={{"id",70},{"releaseType","original"},{"year",1986},{"hardwareRequired",GameCatalog::Json::array({"zx48"})},
        {"releaseStructure",GameCatalog::Json::array({{{"id",1},{"fileName","Cobra.tap"},{"type","tap"},{"size",gameBytes.size()},{"md5",GameCatalog::MD5(gameBytes)}}})}};
      GameCatalog::Fetcher fixture=[&](const std::string &url,size_t,GameCatalog::Progress &)->GameCatalog::Bytes{
        if(url.find("/zximages/")!=std::string::npos)return PreviewPNG();
        if(url.find("/screenshot/")!=std::string::npos)return PreviewGIF();
        if(url.find("/zxfile/")!=std::string::npos)return gameBytes;
        bool games=url.find("export:zxProd/")!=std::string::npos;GameCatalog::Json response={{"responseStatus","success"},{"totalAmount",1},{"responseData",{{games?"zxProd":"zxRelease",GameCatalog::Json::array({games?gameRow:releaseRow})}}}};
        auto body=response.dump();return {body.begin(),body.end()};
      };
      GameCatalog::Library seed(gamesPath,fixture);GameCatalog::Progress progress;
      auto seeded=seed.Search("",0,false,progress);auto variants=seed.Versions(seeded.games[0],progress);auto installedPath=seed.Install(variants[0],false,progress);cachedArtwork=seed.Artwork(seeded.games[0],false,progress);seed.SetOffline(true);
      Menu(MENU_CATALOG);Window catalog=0;
      Check(Until([&](){catalog=FindWindow(x,DefaultRootWindow(x),"Gomas - Game catalog");return catalog!=0;},2000),"Game catalog did not open");
      Wait(200);XSetInputFocus(x,catalog,RevertToParent,CurrentTime);Click(x,catalog,120,60);
      for(char c:std::string("cobra")){key(c,true);key(c,false);}XFlush(x);Wait(450);Click(x,catalog,100,128);
      Check(Until([&](){return Pixel(x,catalog,650,650)==0x31536d;},2000),"Installed catalog version did not become loadable");
      Check(Until([&](){return Pixel(x,catalog,850,253)==0xdc323c;},2000),"Cached PNG preview did not appear above versions");
      Click(x,catalog,1020,374);
      Check(Until([&](){return Pixel(x,catalog,850,253)==0x28c86e;},1000),"Artwork next button did not show the GIF preview");
      Click(x,catalog,675,374);
      Check(Until([&](){return Pixel(x,catalog,850,253)==0xdc323c;},1000),"Artwork previous button did not restore the PNG preview");
      Click(x,catalog,750,430); // File selection in the lower pane.
      Click(x,catalog,770,650);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Gomas - Game catalog") && FindWindow(x,DefaultRootWindow(x),"Gomas - ZX Spectrum 48K - Cobra.tap");},5000),"Catalog did not load the installed game");
      Menu(MENU_CATALOG);
      Check(Until([&](){catalog=FindWindow(x,DefaultRootWindow(x),"Gomas - Game catalog");return catalog!=0;},2000),"Offline catalog did not reopen");
      Wait(150);XSetInputFocus(x,catalog,RevertToParent,CurrentTime);shortcut(XK_Escape);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Gomas - Game catalog");},1000),"Catalog Escape did not close");
      Check(al_filename_exists(installedPath.c_str()),"Catalog removed an installed game on close");
      puts("Native catalog: offline search, installed version, one-click load, reopen and Escape passed");
      // The debugger pauses on open, steps without resuming, and only reads RAM.
      Menu(MENU_DEBUG_SHOW);Window debugger=0;
      Check(Until([&](){debugger=FindWindow(x,DefaultRootWindow(x),"Gomas - Debugger");return debugger && checked(MENU_PAUSE);},2000),"Debugger did not open paused");
      auto debugSnapshot=[&](){struct stat old={};stat(saves.Path(0).c_str(),&old);Menu(MENU_QUICK_SAVE);
        Check(Until([&](){struct stat now={};return stat(saves.Path(0).c_str(),&now)==0 && now.st_ino!=old.st_ino;},2000),"Debugger snapshot did not complete");
        SaveStates::Record record;std::string error;Check(saves.Read(0,record,error),"Cannot read debugger snapshot");return record.core;};
      auto debugBefore=debugSnapshot();Wait(200);auto debugAfter=debugSnapshot();
      auto encoded=[](const SpectrumMachine::State &state){SaveStates::Writer w;SaveStates::WriteCore(w,state);return w.bytes;};
      Check(encoded(debugBefore)==encoded(debugAfter),"Debugger drawing changed machine state");
      Wait(100);Click(x,debugger,225,60);Wait(100);auto instruction=debugSnapshot();
      Check(checked(MENU_PAUSE) && (instruction.frame!=debugAfter.frame || instruction.ula.frameTStates!=debugAfter.ula.frameTStates),"Debugger step did not advance an instruction while paused");
      Click(x,debugger,380,60);Wait(100);auto nextFrame=debugSnapshot();Check(nextFrame.frame==instruction.frame+1 && checked(MENU_PAUSE),"Debugger next frame did not stop at the next frame");
      Click(x,debugger,160,288);XSetInputFocus(x,debugger,RevertToParent,CurrentTime);
      for(KeySym symbol:{XK_c,XK_0,XK_0,XK_0})shortcut(symbol);shortcut(XK_Return);Click(x,debugger,820,60);Wait(150);
      Check(encoded(nextFrame)==encoded(debugSnapshot()),"Debugger RAM navigation modified emulated state");
      shortcut(XK_Escape);
      Check(Until([&](){return !FindWindow(x,DefaultRootWindow(x),"Gomas - Debugger") && !checked(MENU_DEBUG_SHOW);},1000),"Debugger Escape did not close only its own window");
      Check(checked(MENU_PAUSE),"Closing debugger changed pause");
      XSetInputFocus(x,mainWindow,RevertToParent,CurrentTime);shortcut(XK_F5);
      Check(Until([&](){return !checked(MENU_PAUSE);},1000),"Could not resume after debugging");
      puts("Native debugger: pause, instruction/frame steps, RAM navigation, read-only inspection and Escape passed");
      // Leave a deliberately different setup and verify what normal exit writes.
      Menu(MENU_VIEW_4X); Menu(MENU_FILTER_GRID); Menu(MENU_CPU_CLOCK_2X);
      Menu(MENU_SPEED_NORMAL); Menu(MENU_SPEED_DOWN);
      Check(Until([&]() { return checked(MENU_VIEW_4X) && checked(MENU_FILTER_GRID) &&
          checked(MENU_CPU_CLOCK_2X) && speedIs(0.5); }, 2000), "Final settings were not applied");
      if (checked(MENU_FAST_LOAD)) Menu(MENU_FAST_LOAD);
      Menu(MENU_FULLSCREEN);
      Check(Until([&]() { return checked(MENU_FULLSCREEN); }, 2000), "Final fullscreen was not applied");
      Menu(MENU_START_LAST);
      Check(Until([&]() { auto saved = ROMPreferences::Read(configPath);
        return saved.fullscreen && saved.windowWidth == 1280 && saved.windowHeight == 960;
      }, 1000), "Fullscreen lost its underlying window dimensions");
      Menu(MENU_FULLSCREEN); Menu(MENU_VIEW_2X);
      Check(Until([&]() { return checked(MENU_VIEW_2X); }, 2000), "Final 2x window was not applied");


      passed = true;
    } catch (const std::exception &error) {
      failure = error.what(); fprintf(stderr,"UI failure: %s\n",failure.c_str());
      for (const char *title : {"Load system ROM", "Open Spectrum game", "Insert disk image", "Gomas - Choose TZX route", "Gomas - Game catalog", "Gomas - Debugger", "Gomas", "Gomas - Save game", "Gomas - Load game"}) {
        Window modal = FindWindow(x, DefaultRootWindow(x), title), parent = 0;
        if(modal && std::string(title)=="Insert disk image"){
          XWindowAttributes a;XGetWindowAttributes(x,modal,&a);XImage *capture=XGetImage(x,modal,0,0,a.width,a.height,AllPlanes,ZPixmap);
          if(capture){std::ofstream out("/tmp/gomas-dialog-failure.ppm",std::ios::binary);out<<"P6\n"<<a.width<<" "<<a.height<<"\n255\n";
            for(int y=0;y<a.height;++y)for(int px=0;px<a.width;++px){unsigned rgb=XGetPixel(capture,px,y);out.put(rgb>>16);out.put(rgb>>8);out.put(rgb);}XDestroyImage(capture);}
        }
        if (modal && mainWindow && ((std::string(title).find("game") != std::string::npos || std::string(title).find("Choose TZX")!=std::string::npos || std::string(title)=="Gomas - Game catalog") || (XGetTransientForHint(x, modal, &parent) && parent == mainWindow))) {
          XEvent close = {};
          close.xclient.type = ClientMessage;
          close.xclient.window = modal;
          close.xclient.message_type = XInternAtom(x, "WM_PROTOCOLS", False);
          close.xclient.format = 32;
          close.xclient.data.l[0] = XInternAtom(x, "WM_DELETE_WINDOW", False);
          close.xclient.data.l[1] = CurrentTime;
          XSendEvent(x, modal, False, NoEventMask, &close);
        }
      }
      XFlush(x);
    }
    Menu(MENU_EXIT);
    XCloseDisplay(x);
  });
  char app[] = "gomas", tape[] = "cobra.tap";
  char *arguments[] = {app, tape, nullptr};
  int result = GomasApplicationMain(2, arguments);
  input.join();
  auto finalSettings = ROMPreferences::Read(configPath);
  if (passed && !(finalSettings.windowWidth == 640 && finalSettings.windowHeight == 480 &&
      !finalSettings.fullscreen && finalSettings.filter == 3 && finalSettings.playbackSpeed == 0.5 &&
      finalSettings.cpuMultiplier == 2 && !finalSettings.fastLoad)) {
    passed = false; failure = "Normal exit did not persist the final view/emulation configuration";
  }
  unlink(configPath);
  SaveStates::Store saves(savesPath);
  for(unsigned i=0;i<SaveStates::SlotCount;++i) unlink(saves.Path(i).c_str());
  unlink((std::string(savesPath)+"/fixture.z80").c_str());
  unlink((std::string(savesPath)+"/fixture.dsk").c_str());
  unlink((std::string(savesPath)+"/route.tzx").c_str());
  unlink((gamesPath+"/zxart/7/70-1/Cobra.tap").c_str());rmdir((gamesPath+"/zxart/7/70-1").c_str());rmdir((gamesPath+"/zxart/7").c_str());rmdir((gamesPath+"/zxart").c_str());
  for(const auto &path:cachedArtwork)unlink(path.c_str());rmdir((gamesPath+"/artwork").c_str());
  unlink((gamesPath+"/catalog.json").c_str());rmdir(gamesPath.c_str());
  if(hadGames)setenv("GOMAS_GAMES_DIR",savedGamesDir.c_str(),1);else unsetenv("GOMAS_GAMES_DIR");
  rmdir(savesPath);
  if(hadSaves) setenv("GOMAS_SAVE_DIR",savedSaves.c_str(),1); else unsetenv("GOMAS_SAVE_DIR");
  if (hadConfig) setenv("GOMAS_CONFIG", savedConfig.c_str(), 1); else unsetenv("GOMAS_CONFIG");
  if (!passed || result != 0) {
    fprintf(stderr, "Tape UI test failed: %s\n", failure.c_str());
    return 1;
  }
  return 0;
}
