// Optional X11 integration test: real pointer events through the application loop.
#include <allegro5/allegro_native_dialog.h>
#include <atomic>
static std::atomic<unsigned> menuFlagWrites(0);
static std::atomic<int> observedMenuFlags[512];
static void RecordMenuFlags(ALLEGRO_MENU *menu, int id, int flags) {
  ++menuFlagWrites;
  if (id >= 0 && id < 512) observedMenuFlags[id].store(flags);
  al_set_menu_item_flags(menu, id, flags);
}
#define al_set_menu_item_flags RecordMenuFlags
#define main GomasApplicationMain
#include "../main.cpp"
#undef main
#undef al_set_menu_item_flags
#include <X11/Xlib.h>
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

int main() {
  XInitThreads();
  // Never read or overwrite the user's actual last-ROM preference.
  char configPath[] = "/tmp/gomas-ui-settings-XXXXXX";
  int configFd = mkstemp(configPath);
  if (configFd < 0) return 1;
  close(configFd);
  unlink(configPath);
  const char *oldConfig = getenv("GOMAS_CONFIG");
  bool hadConfig = oldConfig != nullptr;
  std::string savedConfig = oldConfig ? oldConfig : "";
  setenv("GOMAS_CONFIG", configPath, 1);
  bool passed = false;
  std::string failure;
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
      unsigned settledWrites = menuFlagWrites.load();
      Wait(250);
      Check(menuFlagWrites.load() == settledWrites, "Idle refresh keeps rebuilding the native ROM menu");
      auto checked = [&](int id) { return (observedMenuFlags[id].load() & ALLEGRO_MENU_ITEM_CHECKED) != 0; };
      auto size = [&]() {
        XWindowAttributes attributes;
        Check(XGetWindowAttributes(x, mainWindow, &attributes) != 0, "Cannot read main window size");
        return std::make_pair(attributes.width, attributes.height);
      };
      Menu(MENU_VIEW_2X);
      Check(Until([&]() { return checked(MENU_VIEW_2X) && size().first == 640; }, 2000), "2x size was not applied");
      const auto originalSize = size();
      Menu(MENU_VIEW_4X);
      Check(Until([&]() { return checked(MENU_VIEW_4X) && size().first == 1280; }, 2000), "4x size was not applied");
      Check(size().second == originalSize.second + 480, "4x changed the image's proportions");
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
      for (int id : {MENU_ROM_128_EN, MENU_ROM_128_ES, MENU_ROM_PLUS2}) {
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
      printf("View: 2x/4x, filters, fullscreen/F11/Escape; mouse: 1000 motion events then Play in %.1f ms; Pause, Stop, selection, Cue and native ROM selector passed\n", latency);
      passed = true;
    } catch (const std::exception &error) {
      failure = error.what();
      for (const char *title : {"Load system ROM", "Gomas"}) {
        Window modal = FindWindow(x, DefaultRootWindow(x), title), parent = 0;
        if (modal && mainWindow && XGetTransientForHint(x, modal, &parent) && parent == mainWindow) {
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
  unlink(configPath);
  if (hadConfig) setenv("GOMAS_CONFIG", savedConfig.c_str(), 1); else unsetenv("GOMAS_CONFIG");
  if (!passed || result != 0) {
    fprintf(stderr, "Tape UI test failed: %s\n", failure.c_str());
    return 1;
  }
  return 0;
}
