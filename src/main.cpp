#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_native_dialog.h>
#include "emulator.h"
#include "tape_view.h"
#include "screen_view.h"
#include "rom_settings.h"

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

using namespace std;

void ReadKeyboard(const ALLEGRO_EVENT &ev, ULA* ula) {
  bool keyEvent = false;
  bool keyDown  = false;

  if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
    keyEvent = true;
    keyDown  = true;
  } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
    keyEvent = true;
  }

  if (keyEvent) {
    switch (ev.keyboard.keycode) {
      // Row 0
      case ALLEGRO_KEY_0: ula->PressKey(0, 9, keyDown); break;
      case ALLEGRO_KEY_1: ula->PressKey(0, 0, keyDown); break;
      case ALLEGRO_KEY_2: ula->PressKey(0, 1, keyDown); break;
      case ALLEGRO_KEY_3: ula->PressKey(0, 2, keyDown); break;
      case ALLEGRO_KEY_4: ula->PressKey(0, 3, keyDown); break;
      case ALLEGRO_KEY_5: ula->PressKey(0, 4, keyDown); break;
      case ALLEGRO_KEY_6: ula->PressKey(0, 5, keyDown); break;
      case ALLEGRO_KEY_7: ula->PressKey(0, 6, keyDown); break;
      case ALLEGRO_KEY_8: ula->PressKey(0, 7, keyDown); break;
      case ALLEGRO_KEY_9: ula->PressKey(0, 8, keyDown); break;
      // Row 1
      case ALLEGRO_KEY_Q: ula->PressKey(1, 0, keyDown); break;
      case ALLEGRO_KEY_W: ula->PressKey(1, 1, keyDown); break;
      case ALLEGRO_KEY_E: ula->PressKey(1, 2, keyDown); break;
      case ALLEGRO_KEY_R: ula->PressKey(1, 3, keyDown); break;
      case ALLEGRO_KEY_T: ula->PressKey(1, 4, keyDown); break;
      case ALLEGRO_KEY_Y: ula->PressKey(1, 5, keyDown); break;
      case ALLEGRO_KEY_U: ula->PressKey(1, 6, keyDown); break;
      case ALLEGRO_KEY_I: ula->PressKey(1, 7, keyDown); break;
      case ALLEGRO_KEY_O: ula->PressKey(1, 8, keyDown); break;
      case ALLEGRO_KEY_P: ula->PressKey(1, 9, keyDown); break;
      // Row 2
      case ALLEGRO_KEY_A: ula->PressKey(2, 0, keyDown); break;
      case ALLEGRO_KEY_S: ula->PressKey(2, 1, keyDown); break;
      case ALLEGRO_KEY_D: ula->PressKey(2, 2, keyDown); break;
      case ALLEGRO_KEY_F: ula->PressKey(2, 3, keyDown); break;
      case ALLEGRO_KEY_G: ula->PressKey(2, 4, keyDown); break;
      case ALLEGRO_KEY_H: ula->PressKey(2, 5, keyDown); break;
      case ALLEGRO_KEY_J: ula->PressKey(2, 6, keyDown); break;
      case ALLEGRO_KEY_K: ula->PressKey(2, 7, keyDown); break;
      case ALLEGRO_KEY_L: ula->PressKey(2, 8, keyDown); break;
      case ALLEGRO_KEY_ENTER: ula->PressKey(2, 9, keyDown); break;
      // Row 2
      case ALLEGRO_KEY_LSHIFT: ula->PressKey(3, 0, keyDown); break;
      case ALLEGRO_KEY_Z: ula->PressKey(3, 1, keyDown); break;
      case ALLEGRO_KEY_X: ula->PressKey(3, 2, keyDown); break;
      case ALLEGRO_KEY_C: ula->PressKey(3, 3, keyDown); break;
      case ALLEGRO_KEY_V: ula->PressKey(3, 4, keyDown); break;
      case ALLEGRO_KEY_B: ula->PressKey(3, 5, keyDown); break;
      case ALLEGRO_KEY_N: ula->PressKey(3, 6, keyDown); break;
      case ALLEGRO_KEY_M: ula->PressKey(3, 7, keyDown); break;
      case ALLEGRO_KEY_RSHIFT: ula->PressKey(3, 8, keyDown); break;
      case ALLEGRO_KEY_SPACE:  ula->PressKey(3, 9, keyDown); break;
    }
  }
}


enum MenuId {
  MENU_FILE = 100, MENU_OPEN, MENU_CLOSE, MENU_EXIT,
  MENU_CPU, MENU_RESET, MENU_VIEW, MENU_SETTINGS, MENU_HELP, MENU_ABOUT,
  MENU_TAPE = 200, MENU_FAST_LOAD, MENU_TAPE_PLAY, MENU_TAPE_PAUSE,
  MENU_TAPE_STOP, MENU_TAPE_REWIND, MENU_TAPE_DECK,
  MENU_LOAD_ROM = 300, MENU_CUSTOM_ROM, MENU_ROM_INFO, MENU_ROM,
  MENU_ROM_48, MENU_ROM_128_EN, MENU_ROM_128_ES, MENU_ROM_PLUS2, MENU_ROM_PLUS3,
  MENU_ROM_STARTUP, MENU_START_LAST, MENU_START_48,
  MENU_VIEW_2X = 400, MENU_VIEW_4X, MENU_FULLSCREEN, MENU_FILTERS,
  MENU_FILTER_NONE, MENU_FILTER_SCANLINES_LIGHT, MENU_FILTER_SCANLINES_STRONG, MENU_FILTER_GRID
};

// Menu IDs map to canonical ROM sets, independently of registry ordering.
const ROMProfile &MenuROMProfile(int id) {
  static const char *files[] = {"48.rom", "128-0.rom", "128-spanish-0.rom", "plus2-0.rom", "plus3-0.rom"};
  const char *file = files[id - MENU_ROM_48];
  for (const auto &profile : ROMProfiles())
    if (std::string(profile.files.front()) == file) return profile;
  throw std::logic_error("Missing built-in ROM profile");
}

// Allegro owns an attached menu. Detach/destroy it before its display goes away.
struct DisplayDeleter {
  void operator()(ALLEGRO_DISPLAY *display) const {
    al_set_display_menu(display, nullptr);
    al_destroy_display(display);
  }
};

bool InstallMenu(ALLEGRO_DISPLAY *display) {
  ALLEGRO_MENU_INFO items[] = {
    ALLEGRO_START_OF_MENU("&File", MENU_FILE),
      {"&Open...", MENU_OPEN, 0, nullptr},
      {"&Close", MENU_CLOSE, ALLEGRO_MENU_ITEM_DISABLED, nullptr},
      {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
      {"E&xit", MENU_EXIT, 0, nullptr},
    ALLEGRO_END_OF_MENU,
    ALLEGRO_START_OF_MENU("&Tape", MENU_TAPE),
      {"&Fast load", MENU_FAST_LOAD, ALLEGRO_MENU_ITEM_CHECKBOX | ALLEGRO_MENU_ITEM_CHECKED, nullptr},
      {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
      {"&Play", MENU_TAPE_PLAY, ALLEGRO_MENU_ITEM_DISABLED, nullptr},
      {"Pa&use", MENU_TAPE_PAUSE, ALLEGRO_MENU_ITEM_DISABLED, nullptr},
      {"&Stop", MENU_TAPE_STOP, ALLEGRO_MENU_ITEM_DISABLED, nullptr},
      {"&Rewind", MENU_TAPE_REWIND, ALLEGRO_MENU_ITEM_DISABLED, nullptr},
      {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
      {"Show tape &deck...", MENU_TAPE_DECK, 0, nullptr},
    ALLEGRO_END_OF_MENU,
    ALLEGRO_START_OF_MENU("&CPU", MENU_CPU),
      {"&Reset", MENU_RESET, 0, nullptr},
    ALLEGRO_END_OF_MENU,
    ALLEGRO_START_OF_MENU("&View", MENU_VIEW),
      {"&2x", MENU_VIEW_2X, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
      {"&4x", MENU_VIEW_4X, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
      {"&Fullscreen (F11)", MENU_FULLSCREEN, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
      {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
      ALLEGRO_START_OF_MENU("&Filters", MENU_FILTERS),
        {"&None", MENU_FILTER_NONE, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
        {"Scanlines (&light)", MENU_FILTER_SCANLINES_LIGHT, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
        {"Scanlines (&strong)", MENU_FILTER_SCANLINES_STRONG, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
        {"Pixel &grid", MENU_FILTER_GRID, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
      ALLEGRO_END_OF_MENU,
    ALLEGRO_END_OF_MENU,
    ALLEGRO_START_OF_MENU("&Settings", MENU_SETTINGS),
      ALLEGRO_START_OF_MENU("&ROM", MENU_ROM),
        {"&Open ROM...", MENU_LOAD_ROM, 0, nullptr},
        {"Open &custom 48K ROM...", MENU_CUSTOM_ROM, 0, nullptr},
        {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
        {"ZX Spectrum &48K", MENU_ROM_48, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
        // CHECKBOX is immutable after creation in Allegro native menus.
        {"ZX Spectrum 128K (English)", MENU_ROM_128_EN, ALLEGRO_MENU_ITEM_CHECKBOX | ALLEGRO_MENU_ITEM_DISABLED, nullptr},
        {"ZX Spectrum 128K (Spanish)", MENU_ROM_128_ES, ALLEGRO_MENU_ITEM_CHECKBOX | ALLEGRO_MENU_ITEM_DISABLED, nullptr},
        {"ZX Spectrum +2", MENU_ROM_PLUS2, ALLEGRO_MENU_ITEM_CHECKBOX | ALLEGRO_MENU_ITEM_DISABLED, nullptr},
        {"ZX Spectrum +3", MENU_ROM_PLUS3, ALLEGRO_MENU_ITEM_CHECKBOX | ALLEGRO_MENU_ITEM_DISABLED, nullptr},
        {nullptr, static_cast<uint16_t>(-1), 0, nullptr},
        ALLEGRO_START_OF_MENU("On &startup", MENU_ROM_STARTUP),
          {"&Last opened ROM", MENU_START_LAST, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
          {"Spectrum &48K", MENU_START_48, ALLEGRO_MENU_ITEM_CHECKBOX, nullptr},
        ALLEGRO_END_OF_MENU,
        {"&Information...", MENU_ROM_INFO, 0, nullptr},
      ALLEGRO_END_OF_MENU,
    ALLEGRO_END_OF_MENU,
    ALLEGRO_START_OF_MENU("&Help", MENU_HELP),
      {"&About / Help", MENU_ABOUT, 0, nullptr},
    ALLEGRO_END_OF_MENU,
    ALLEGRO_END_OF_MENU
  };
  unique_ptr<ALLEGRO_MENU, decltype(&al_destroy_menu)> menu(
      al_build_menu(items), al_destroy_menu);
  if (!menu) return false;
  if (!al_set_display_menu(display, menu.get())) return false;
  menu.release();
  return true;
}

void ReleaseKeys(ULA &ula) {
  for (unsigned row = 0; row < 4; ++row)
    for (unsigned col = 0; col < 10; ++col) ula.PressKey(row, col, false);
}

int main(int argc, char *argv[]) {
  const char *usage = "Usage: gomas [tape.tap [system.rom]] [--rom system.rom] [--custom-48-rom system.rom]";
  string tapePath, romPath;
  bool customROM = false;
  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "--help" || arg == "-h") { puts(usage); return 0; }
    if (arg == "--rom" || arg == "--custom-48-rom") {
      if (i + 1 >= argc || !romPath.empty()) { fprintf(stderr, "%s\n", usage); return 1; }
      customROM = arg == "--custom-48-rom";
      romPath = argv[++i];
    } else if (!arg.empty() && arg[0] == '-') {
      fprintf(stderr, "Unknown option: %s\n%s\n", arg.c_str(), usage); return 1;
    } else if (tapePath.empty()) tapePath = arg;
    else if (romPath.empty()) romPath = arg;
    else { fprintf(stderr, "%s\n", usage); return 1; }
  }
  if (!al_init()) { fprintf(stderr, "Cannot initialize Allegro\n"); return 1; }
  SpectrumMachine machine;
  ULA &ula = machine.ula;
  al_set_org_name("gomas");
  al_set_app_name("gomas");
  const string settingsPath = ROMSettingsPath();
  ROMPreferences preferences = ROMPreferences::Read(settingsPath);
  const bool explicitROM = !romPath.empty();
  unique_ptr<ALLEGRO_PATH, decltype(&al_destroy_path)> executable(
      al_get_standard_path(ALLEGRO_EXENAME_PATH), al_destroy_path);
  const string executablePath = executable ? al_path_cstr(executable.get(), ALLEGRO_NATIVE_PATH_SEP) : "";
  string error, notice;
  if (!LoadStartupROM(machine, preferences, romPath, customROM, executablePath, error, notice)) {
    fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
  if (!notice.empty()) fprintf(stderr, "%s\n", notice.c_str());
  if (!tapePath.empty() && !machine.Open(tapePath, error)) {
    fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }
#ifdef ALLEGRO_GTK_TOPLEVEL
  // Allegro native menus use X11/GLX. Keep GTK on that same connection even
  // when the desktop exports GDK_BACKEND=wayland (e.g. Steam Deck).
  setenv("GDK_BACKEND", "x11", 1);
#endif
  if (!al_install_keyboard() || !al_install_mouse() || !al_install_audio() ||
      !al_init_native_dialog_addon() || !al_init_primitives_addon()) {
    fprintf(stderr, "Cannot initialize keyboard, mouse, audio or native dialogs\n");
    return 1;
  }
  int displayFlags = ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE;
#ifdef ALLEGRO_GTK_TOPLEVEL
  displayFlags |= ALLEGRO_GTK_TOPLEVEL;
#endif
  al_set_new_display_flags(displayFlags);
  al_set_new_display_option(ALLEGRO_VSYNC, 0, ALLEGRO_SUGGEST);
  unique_ptr<ALLEGRO_DISPLAY, DisplayDeleter> display(al_create_display(640, 480));
  if (!display || !InstallMenu(display.get())) {
    fprintf(stderr, "Cannot create display/menu\n");
    return 1;
  }
  al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
  unique_ptr<ALLEGRO_BITMAP, decltype(&al_destroy_bitmap)> bitmap(
      al_create_bitmap(320, 240), al_destroy_bitmap);
  unique_ptr<ALLEGRO_EVENT_QUEUE, decltype(&al_destroy_event_queue)> events(
      al_create_event_queue(), al_destroy_event_queue);
  unique_ptr<ALLEGRO_TIMER, decltype(&al_destroy_timer)> refreshTimer(
      al_create_timer(1.0 / 50), al_destroy_timer);
  unique_ptr<ALLEGRO_VOICE, decltype(&al_destroy_voice)> voice(
      al_create_voice(AUDIO_SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16,
                      ALLEGRO_CHANNEL_CONF_2), al_destroy_voice);
  unique_ptr<ALLEGRO_MIXER, decltype(&al_destroy_mixer)> mixer(
      al_create_mixer(AUDIO_SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                      ALLEGRO_CHANNEL_CONF_2), al_destroy_mixer);
  unique_ptr<ALLEGRO_AUDIO_STREAM, decltype(&al_destroy_audio_stream)> stream(
      nullptr, al_destroy_audio_stream);
  if (!bitmap || !events || !refreshTimer || !voice || !mixer ||
      !al_attach_mixer_to_voice(mixer.get(), voice.get())) {
    fprintf(stderr, "Cannot create video/audio resources\n");
    return 1;
  }
  al_init_font_addon();
  TapeView tapeView;
  ScreenView screenView;
  ula.SetBitmap(bitmap.get());
  al_register_event_source(events.get(), al_get_keyboard_event_source());
  al_register_event_source(events.get(), al_get_mouse_event_source());
  al_register_event_source(events.get(), al_get_display_event_source(display.get()));
  al_register_event_source(events.get(), al_get_default_menu_event_source());
  al_register_event_source(events.get(), al_get_timer_event_source(refreshTimer.get()));

  auto updateUI = [&]() {
    string title = "Gomas - " + machine.systemROM.name;
    if (!machine.tapePath.empty()) {
      size_t slash = machine.tapePath.find_last_of("/\\");
      title += " - " + machine.tapePath.substr(slash == string::npos ? 0 : slash + 1);
    }
    static string lastTitle;
    if (title != lastTitle) { al_set_window_title(display.get(), title.c_str()); lastTitle = title; }
    auto menu = al_get_display_menu(display.get());
    auto flags = [&](int id, int value) {
      if (al_get_menu_item_flags(menu, id) != value) al_set_menu_item_flags(menu, id, value);
    };
    flags(MENU_VIEW_2X, ALLEGRO_MENU_ITEM_CHECKBOX | (screenView.IsScale(display.get(), 2) ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    flags(MENU_VIEW_4X, ALLEGRO_MENU_ITEM_CHECKBOX | (screenView.IsScale(display.get(), 4) ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    flags(MENU_FULLSCREEN, ALLEGRO_MENU_ITEM_CHECKBOX | (screenView.Fullscreen(display.get()) ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    for (int id = MENU_FILTER_NONE; id <= MENU_FILTER_GRID; ++id)
      flags(id, ALLEGRO_MENU_ITEM_CHECKBOX |
          (static_cast<int>(screenView.filter) == id - MENU_FILTER_NONE ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    for (int id = MENU_ROM_48; id <= MENU_ROM_PLUS3; ++id) {
      const auto &profile = MenuROMProfile(id);
      flags(id, ALLEGRO_MENU_ITEM_CHECKBOX |
          (machine.systemROM.name == profile.name ? ALLEGRO_MENU_ITEM_CHECKED : 0) |
          (SupportsHardware(profile.model) ? 0 : ALLEGRO_MENU_ITEM_DISABLED));
    }
    flags(MENU_START_LAST, ALLEGRO_MENU_ITEM_CHECKBOX | (preferences.restoreLast ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    flags(MENU_START_48, ALLEGRO_MENU_ITEM_CHECKBOX | (!preferences.restoreLast ? ALLEGRO_MENU_ITEM_CHECKED : 0));
    const bool loaded = !machine.tape.empty();
    const bool available = machine.deck.CurrentBlock() < machine.deck.Blocks().size();
    flags(MENU_CLOSE, loaded ? 0 : ALLEGRO_MENU_ITEM_DISABLED);
    flags(MENU_FAST_LOAD, ALLEGRO_MENU_ITEM_CHECKBOX | (machine.fastLoad ? ALLEGRO_MENU_ITEM_CHECKED : 0) |
        (machine.CanFastLoad() ? 0 : ALLEGRO_MENU_ITEM_DISABLED));
    flags(MENU_TAPE_PLAY, !machine.fastLoad && available && !machine.deck.IsPlaying() ? 0 : ALLEGRO_MENU_ITEM_DISABLED);
    flags(MENU_TAPE_PAUSE, !machine.fastLoad && machine.deck.IsPlaying() ? 0 : ALLEGRO_MENU_ITEM_DISABLED);
    flags(MENU_TAPE_STOP, !machine.fastLoad && available ? 0 : ALLEGRO_MENU_ITEM_DISABLED);
    flags(MENU_TAPE_REWIND, loaded ? 0 : ALLEGRO_MENU_ITEM_DISABLED);
  };
  auto savePreferences = [&]() {
    string settingsError;
    if (!preferences.Save(settingsPath, settingsError)) {
      bool playing = al_get_audio_stream_playing(stream.get());
      al_set_audio_stream_playing(stream.get(), false);
      al_show_native_message_box(display.get(), "Gomas", "Settings could not be saved",
          settingsError.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
      if (playing) al_set_audio_stream_playing(stream.get(), true);
      return false;
    }
    return true;
  };
  auto rememberROM = [&](bool custom) {
    preferences.Remember(machine.systemROM.source, custom);
    savePreferences();
  };
  auto viewResult = [&](bool success) {
    ReleaseKeys(ula);
    if (!success) {
      bool playing = al_get_audio_stream_playing(stream.get());
      al_set_audio_stream_playing(stream.get(), false);
      al_show_native_message_box(display.get(), "Gomas", "Cannot change display mode",
          "The window system could not apply the requested size or fullscreen mode.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
      al_set_audio_stream_playing(stream.get(), playing);
    }
  };
  auto fillAudio = [&]() {
    // Bound catch-up work so UI events cannot starve on slower machines.
    for (int i = 0; i < 3; ++i) {
      void *fragment = al_get_audio_stream_fragment(stream.get());
      if (!fragment) break;
      machine.ReadAudio(static_cast<short *>(fragment), SAMPLES_PER_FRAME);
      if (!al_set_audio_stream_fragment(stream.get(), fragment)) return false;
    }
    return true;
  };
  auto restartAudio = [&]() {
    // Drop every queued sample from the previous machine state.
    if (stream) {
      al_unregister_event_source(events.get(), al_get_audio_stream_event_source(stream.get()));
      stream.reset();
    }
    stream.reset(al_create_audio_stream(3, SAMPLES_PER_FRAME, AUDIO_SAMPLE_RATE,
        ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1));
    if (!stream) return false;
    al_register_event_source(events.get(), al_get_audio_stream_event_source(stream.get()));
    return fillAudio() && al_attach_audio_stream_to_mixer(stream.get(), mixer.get());
  };
  updateUI();
  if (!restartAudio()) { fprintf(stderr, "Cannot start audio stream\n"); return 1; }

  if (explicitROM || preferences.lastROM.empty()) rememberROM(customROM);
  al_start_timer(refreshTimer.get());
  bool running = true;
  while (running) {
    ALLEGRO_EVENT event;
    al_wait_for_event(events.get(), &event);
    bool resetAudio = false;
    // Handle input immediately; drawing is paced independently of event traffic.
    if (tapeView.Handle(event, machine)) continue;
    switch (event.type) {
      case ALLEGRO_EVENT_DISPLAY_CLOSE: running = false; break;
      case ALLEGRO_EVENT_DISPLAY_RESIZE: al_acknowledge_resize(display.get()); break;
      case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT: ReleaseKeys(ula); break;
      case ALLEGRO_EVENT_KEY_DOWN:
        if (event.keyboard.keycode == ALLEGRO_KEY_F11) {
          viewResult(screenView.SetFullscreen(display.get(), !screenView.Fullscreen(display.get())));
          updateUI();
        } else if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
          if (screenView.Fullscreen(display.get())) {
            viewResult(screenView.SetFullscreen(display.get(), false));
            updateUI();
          } else running = false;
        } else ReadKeyboard(event, &ula);
        break;
      case ALLEGRO_EVENT_KEY_UP: ReadKeyboard(event, &ula); break;
      case ALLEGRO_EVENT_MENU_CLICK:
        switch (event.user.data1) {
          case MENU_VIEW_2X: viewResult(screenView.SetScale(display.get(), 2)); break;
          case MENU_VIEW_4X: viewResult(screenView.SetScale(display.get(), 4)); break;
          case MENU_FULLSCREEN:
            viewResult(screenView.SetFullscreen(display.get(), !screenView.Fullscreen(display.get()))); break;
          case MENU_FILTER_NONE:
          case MENU_FILTER_SCANLINES_LIGHT:
          case MENU_FILTER_SCANLINES_STRONG:
          case MENU_FILTER_GRID:
            screenView.filter = static_cast<ScreenFilter>(event.user.data1 - MENU_FILTER_NONE); break;
          case MENU_OPEN: {
            al_set_audio_stream_playing(stream.get(), false);
            ReleaseKeys(ula);
            unique_ptr<ALLEGRO_FILECHOOSER, decltype(&al_destroy_native_file_dialog)> chooser(
                al_create_native_file_dialog(machine.tapePath.empty() ? nullptr : machine.tapePath.c_str(),
                    "Open Spectrum tape", "*.tap;*.TAP", ALLEGRO_FILECHOOSER_FILE_MUST_EXIST),
                al_destroy_native_file_dialog);
            if (!chooser || !al_show_native_file_dialog(display.get(), chooser.get())) {
              al_show_native_message_box(display.get(), "Gomas", "Cannot open file dialog",
                  "Please try again.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            } else if (al_get_native_file_dialog_count(chooser.get()) > 0) {
              const char *path = al_get_native_file_dialog_path(chooser.get(), 0);
              if (path && machine.Open(path, error)) { resetAudio = true; tapeView.NewTape(); }
              else al_show_native_message_box(display.get(), "Gomas", "Cannot open tape",
                  error.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            }
            ReleaseKeys(ula);
            if (!resetAudio) al_set_audio_stream_playing(stream.get(), true);
            break;
          }
          case MENU_CLOSE: machine.Close(); tapeView.NewTape(); resetAudio = true; break;
          case MENU_RESET: machine.Reset(); resetAudio = true; break;
          case MENU_FAST_LOAD:
            machine.SetFastLoad(!machine.fastLoad);
            resetAudio = true;
            if (!machine.fastLoad && !tapeView.Show(events.get()))
              al_show_native_message_box(display.get(), "Gomas", "Cannot open tape deck", "Try Tape > Show tape deck again.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            break;
          case MENU_TAPE_PLAY: machine.PlayTape(); break;
          case MENU_TAPE_PAUSE: machine.PauseTape(); break;
          case MENU_TAPE_STOP: machine.StopTape(); break;
          case MENU_TAPE_REWIND: machine.CueTape(0); tapeView.NewTape(); break;
          case MENU_TAPE_DECK:
            if (!tapeView.Show(events.get()))
              al_show_native_message_box(display.get(), "Gomas", "Cannot open tape deck", "No tape deck window could be created.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            break;
          case MENU_EXIT: running = false; break;
          case MENU_LOAD_ROM:
          case MENU_CUSTOM_ROM: {
            al_set_audio_stream_playing(stream.get(), false);
            ReleaseKeys(ula);
            bool custom = event.user.data1 == MENU_CUSTOM_ROM;
            unique_ptr<ALLEGRO_FILECHOOSER, decltype(&al_destroy_native_file_dialog)> chooser(
                al_create_native_file_dialog(machine.systemROM.source.c_str(),
                    custom ? "Load custom 48K ROM" : "Load system ROM", "*.rom;*.ROM;*.bin;*.BIN",
                    ALLEGRO_FILECHOOSER_FILE_MUST_EXIST), al_destroy_native_file_dialog);
            if (!chooser || !al_show_native_file_dialog(display.get(), chooser.get())) {
              al_show_native_message_box(display.get(), "Gomas", "Cannot open ROM dialog", "Please try again.", nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            } else if (al_get_native_file_dialog_count(chooser.get()) > 0) {
              const char *path = al_get_native_file_dialog_path(chooser.get(), 0);
              if (path && machine.LoadROM(path, error, custom)) {
                resetAudio = true; tapeView.NewTape(); rememberROM(custom);
              }
              else al_show_native_message_box(display.get(), "Gomas", "Cannot activate ROM", error.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
            }
            ReleaseKeys(ula);
            if (!resetAudio) al_set_audio_stream_playing(stream.get(), true);
            break;
          }
          case MENU_ROM_48:
          case MENU_ROM_128_EN:
          case MENU_ROM_128_ES:
          case MENU_ROM_PLUS2:
          case MENU_ROM_PLUS3: {
            al_set_audio_stream_playing(stream.get(), false);
            ReleaseKeys(ula);
            const auto &profile = MenuROMProfile(event.user.data1);
            string path = profile.model == MachineModel::Spectrum48 ? Default48ROM(executablePath) :
                FindROM(profile.files.front(), executablePath);
            if (!path.empty() && machine.LoadROM(path, error)) {
              resetAudio = true; tapeView.NewTape(); rememberROM(false);
            } else {
              if (path.empty()) error = "Cannot find the ROM set for " + string(profile.name) + " in roms/.";
              al_show_native_message_box(display.get(), "Gomas", "Cannot activate ROM", error.c_str(), nullptr, ALLEGRO_MESSAGEBOX_ERROR);
              al_set_audio_stream_playing(stream.get(), true);
            }
            break;
          }
          case MENU_START_LAST:
          case MENU_START_48: {
            bool previous = preferences.restoreLast;
            preferences.restoreLast = event.user.data1 == MENU_START_LAST;
            if (!savePreferences()) preferences.restoreLast = previous;
            break;
          }
          case MENU_ROM_INFO: {
            al_set_audio_stream_playing(stream.get(), false);
            ReleaseKeys(ula);
            string information = machine.systemROM.name + "\n" + machine.systemROM.source +
                "\n\nFast load: " + (machine.CanFastLoad() ? "supported" : "not available for this ROM; use tape playback") +
                "\n\nOn startup: " + (preferences.restoreLast ? "last opened ROM (48K fallback)" : "Spectrum 48K") +
                "\n\nSelecting a recognized ROM determines its machine model.\n128K, +2 and +3 hardware support is still pending.";
            al_show_native_message_box(display.get(), "Gomas", "Machine ROM", information.c_str(), nullptr, 0);
            al_set_audio_stream_playing(stream.get(), true);
            break;
          }
          case MENU_ABOUT:
            al_set_audio_stream_playing(stream.get(), false);
            ReleaseKeys(ula);
            al_show_native_message_box(display.get(), "Gomas", "ZX Spectrum 48K / 128K / +2 emulator",
                "File > Open: insert a TAP and restart the Spectrum.\n"
                "At the BASIC prompt, type LOAD \"\" and press Enter.\n"
                "J = LOAD; Right Shift + P = quotation mark.\n\n"
                "File > Close: eject the tape and restart.\n"
                "CPU > Reset: restart and rewind the current tape.\n"
                "Settings > ROM: choose a ROM and the startup default.\n"
                "Tape > Fast load: instant loading (default).\n"
                "Turn it off, enter LOAD \"\" and press Tape > Play for real-time loading.\n"
                "Tape > Show tape deck: browse blocks and cue a multi-load part.\n"
                "Pause keeps position; Stop returns to the start of this block.\n"
                "Left Shift = CAPS SHIFT; Right Shift = SYMBOL SHIFT.\n"
                "View: 2x, 4x, fullscreen and pixel filters.\n"
                "F11 = fullscreen; Escape = leave fullscreen / Exit in a window.\n\n"
                "By Pedro Lopez. Based on YASS by Fernando Marcos.\n"
                "http://yass.speccy.org/", nullptr, 0);
            ReleaseKeys(ula);
            al_set_audio_stream_playing(stream.get(), true);
            break;
        }
        updateUI();
        break;
      case ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT:
        if (event.any.source == al_get_audio_stream_event_source(stream.get()) && !fillAudio()) {
          fprintf(stderr, "Cannot submit audio fragment\n"); return 1;
        }
        break;
    }
    if (!running) break;
    if (resetAudio && !restartAudio()) {
      fprintf(stderr, "Cannot restart audio stream\n"); return 1;
    }
    if (event.type != ALLEGRO_EVENT_TIMER ||
        event.timer.source != refreshTimer.get() ||
        event.timer.count < al_get_timer_count(refreshTimer.get())) continue;
    // Skip obsolete refresh ticks after a dialog or a slow frame. Mouse motion
    // and audio events must never trigger expensive redraws on their own.
    updateUI();
    al_set_target_backbuffer(display.get());
    if (!ula.Present()) { fprintf(stderr, "Cannot upload frame\n"); return 1; }
    al_set_target_backbuffer(display.get());
    screenView.Draw(bitmap.get(), al_get_display_width(display.get()), al_get_display_height(display.get()));
    al_flip_display();
    if ((event.timer.count & 1) == 0) tapeView.Draw(machine);
  }
  return 0;
}
