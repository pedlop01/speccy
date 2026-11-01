// Optional X11 integration test: real pointer events through the application loop.
#define main GomasApplicationMain
#include "../main.cpp"
#undef main
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
  bool passed = false;
  std::string failure;
  std::thread input([&]() {
    Display *x = XOpenDisplay(nullptr);
    if (!x) { failure = "test-ui requires an X11 display"; return; }
    try {
      Window mainWindow = 0, deck = 0;
      Check(Until([&]() {
        mainWindow = FindWindow(x, DefaultRootWindow(x), "Gomas - ZX Spectrum 48K - cobra.tap");
        return mainWindow != 0;
      }, 5000), "Main window did not open");
      // The title is set immediately before the event loop starts.
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
      printf("Mouse: 1000 motion events then Play in %.1f ms; Pause, Stop, selection and Cue passed\n", latency);
      passed = true;
    } catch (const std::exception &error) {
      failure = error.what();
    }
    Menu(MENU_EXIT);
    XCloseDisplay(x);
  });
  char app[] = "gomas", tape[] = "cobra.tap", rom[] = "48.rom";
  char *arguments[] = {app, tape, rom, nullptr};
  int result = GomasApplicationMain(3, arguments);
  input.join();
  if (!passed || result != 0) {
    fprintf(stderr, "Tape UI test failed: %s\n", failure.c_str());
    return 1;
  }
  return 0;
}
