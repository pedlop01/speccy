#ifndef TAPE_VIEW_H
#define TAPE_VIEW_H

#include <allegro5/allegro_font.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include "emulator.h"

// A modeless deck: the Spectrum keeps running while blocks are browsed/cued.
class TapeView {
public:
  ~TapeView() { Close(); }
  bool Show(ALLEGRO_EVENT_QUEUE *queue) {
    if (display) {
      dirty = true;
      al_set_display_flag(display, ALLEGRO_MINIMIZED, false);
      return true;
    }
    int flags = al_get_new_display_flags();
    int deckFlags = ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE | ALLEGRO_GENERATE_EXPOSE_EVENTS;
#ifdef ALLEGRO_GTK_TOPLEVEL
    deckFlags |= ALLEGRO_GTK_TOPLEVEL;
#endif
    al_set_new_display_flags(deckFlags);
    display = al_create_display(840, 600);
    al_set_new_display_flags(flags);
    if (!display) return false;
    al_set_window_title(display, "Gomas - Tape deck");
    al_set_window_constraints(display, 840, 420, 0, 0);
    al_apply_window_constraints(display, true);
    al_set_target_backbuffer(display);
    if (!font) {
      int bitmapFlags = al_get_new_bitmap_flags();
      // The font belongs to this display. Software glyphs cost a texture upload
      // for each draw and made pointer motion overwhelm the event queue.
      al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
      font = al_create_builtin_font();
      al_set_new_bitmap_flags(bitmapFlags);
    }
    if (!font) { al_destroy_display(display); display = nullptr; return false; }
    dirty = true;
    events = queue;
    al_register_event_source(events, al_get_display_event_source(display));
    return true;
  }
  void Close() {
    if (!display) return;
    al_unregister_event_source(events, al_get_display_event_source(display));
    if (font) { al_destroy_font(font); font = nullptr; }
    al_destroy_display(display);
    display = nullptr;
  }
  void NewTape() { selected = first = observedBlock = 0; dirty = true; }
  bool IsOpen() const { return display != nullptr; }

  bool Handle(const ALLEGRO_EVENT &event, SpectrumMachine &machine) {
    if (!display) return false;
    if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE && event.display.source == display) { Close(); return true; }
    if ((event.type == ALLEGRO_EVENT_DISPLAY_EXPOSE || event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) &&
        event.display.source == display) { dirty = true; return true; }
    if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE && event.display.source == display) {
      al_acknowledge_resize(display); dirty = true; return true;
    }
    if ((event.type == ALLEGRO_EVENT_KEY_DOWN || event.type == ALLEGRO_EVENT_KEY_UP) && event.keyboard.display == display) {
      if (event.type == ALLEGRO_EVENT_KEY_UP) return true;
      dirty = true;
      size_t count = machine.deck.Blocks().size();
      switch (event.keyboard.keycode) {
        case ALLEGRO_KEY_ESCAPE: Close(); break;
        case ALLEGRO_KEY_SPACE:
          if (machine.deck.IsPlaying()) machine.PauseTape(); else machine.PlayTape();
          break;
        case ALLEGRO_KEY_HOME: machine.CueTape(0); selected = first = 0; break;
        case ALLEGRO_KEY_ENTER: if (count) machine.CueTape(selected); break;
        case ALLEGRO_KEY_UP: if (selected) --selected; break;
        case ALLEGRO_KEY_DOWN: if (selected + 1 < count) ++selected; break;
      }
      KeepSelectionVisible();
      return true;
    }
    if (event.type == ALLEGRO_EVENT_MOUSE_AXES && event.mouse.display == display) {
      if (!event.mouse.dz) return true; // Pointer motion changes no visual state.
      dirty = true;
      int rows = VisibleRows();
      int limit = std::max(0, int(machine.deck.Blocks().size()) - rows);
      first = std::max(0, std::min(limit, first - event.mouse.dz * 3));
      return true;
    }
    if (event.type != ALLEGRO_EVENT_MOUSE_BUTTON_DOWN || event.mouse.display != display) return false;
    if (event.mouse.button != 1) return true;
    dirty = true;
    float x = event.mouse.x / Scale, y = event.mouse.y / Scale;
    if (y >= 65 && y <= 89) {
      int button = int((x - 12) / 106);
      if (x < 12 || button > 4 || x > 12 + button * 106 + 100) return true;
      switch (button) {
        case 0: machine.PlayTape(); break;
        case 1: machine.PauseTape(); break;
        case 2: machine.StopTape(); break;
        case 3: machine.CueTape(0); selected = first = 0; break;
        case 4: machine.CueTape(selected); break;
      }
    } else if (x >= 8 && x < Width() - 8 && y >= ListTop) {
      int row = int((y - ListTop) / RowHeight);
      size_t index = first + row;
      // Hit-test the same rectangle used to draw the row, including its top
      // padding. The old origin was the text origin, five pixels too low.
      if (row < VisibleRows() && y < ListTop + row * RowHeight + RowContentHeight &&
          index < machine.deck.Blocks().size()) selected = index;
    }
    return true;
  }

  void Draw(const SpectrumMachine &machine) {
    if (!display) return;
    const auto &deck = machine.deck;
    const auto &blocks = deck.Blocks();
    const unsigned tick = static_cast<unsigned>(deck.Seconds() * 25);
    if (!dirty && lastPath == machine.tapePath && lastFast == machine.fastLoad &&
        lastState == deck.GetState() && lastBlock == deck.CurrentBlock() && lastTick == tick)
      return;
    dirty = false;
    lastPath = machine.tapePath;
    lastFast = machine.fastLoad;
    lastState = deck.GetState();
    lastBlock = deck.CurrentBlock();
    lastTick = tick;
    if (deck.CurrentBlock() != observedBlock) {
      if (selected == observedBlock && !blocks.empty()) {
        selected = std::min(deck.CurrentBlock(), blocks.size() - 1);
        KeepSelectionVisible();
      }
      observedBlock = deck.CurrentBlock();
    }
    if (blocks.empty()) selected = first = 0;
    else if (selected >= blocks.size()) selected = blocks.size() - 1;
    first = std::min(first, std::max(0, int(blocks.size()) - VisibleRows()));
    al_set_target_backbuffer(display);
    al_clear_to_color(al_map_rgb(23, 27, 34));
    ALLEGRO_TRANSFORM transform;
    al_identity_transform(&transform);
    al_scale_transform(&transform, Scale, Scale);
    al_use_transform(&transform);
    auto white = al_map_rgb(231, 236, 243), muted = al_map_rgb(153, 165, 184);
    auto accent = al_map_rgb(82, 208, 179);
    al_draw_text(font, white, 12, 12, 0, "TAPE DECK");
    std::string state;
    switch (deck.GetState()) {
      case TapeDeck::State::Stopped: state = "Stopped"; break;
      case TapeDeck::State::Playing: state = "Playing"; break;
      case TapeDeck::State::Paused: state = "Paused"; break;
      case TapeDeck::State::End: state = blocks.empty() ? "No tape" : "End of tape"; break;
    }
    std::string mode = machine.fastLoad ? "FAST LOAD ON - instant loading" : "FAST LOAD OFF - " + state;
    al_draw_text(font, accent, 12, 30, 0, mode.c_str());
    std::string name = machine.tapePath.empty() ? "File > Open to insert a TAP" : machine.tapePath;
    if (name.size() > size_t((Width() - 24) / 8)) name = "..." + name.substr(name.size() - size_t((Width() - 48) / 8));
    al_draw_text(font, muted, 12, 46, 0, name.c_str());
    const char *buttons[] = {"Play", "Pause", "Stop", "Rewind", "Cue selected"};
    for (int i = 0; i < 5; ++i) {
      bool available = deck.CurrentBlock() < blocks.size();
      bool enabled = i >= 3 ? !blocks.empty() : !machine.fastLoad && available;
      if (i == 0) enabled = enabled && !deck.IsPlaying();
      if (i == 1) enabled = enabled && deck.IsPlaying();
      float x = 12 + i * 106;
      al_draw_filled_rectangle(x, 65, x + 100, 89, al_map_rgb(enabled ? 49 : 32, enabled ? 62 : 37, enabled ? 77 : 45));
      al_draw_text(font, enabled ? white : muted, x + 50, 73, ALLEGRO_ALIGN_CENTRE, buttons[i]);
    }
    al_draw_text(font, muted, 12, 105, 0, "#    Block / contents");
    al_draw_text(font, muted, Width() - 12, 105, ALLEGRO_ALIGN_RIGHT, "Time");
    al_set_clipping_rectangle(0, int(120 * Scale), al_get_display_width(display), int((Height() - 172) * Scale));
    for (int row = 0; row < VisibleRows(); ++row) {
      size_t i = first + row;
      if (i >= blocks.size()) break;
      float rowTop = ListTop + row * RowHeight;
      float y = rowTop + 5;
      if (i == selected) al_draw_filled_rectangle(8, rowTop, Width() - 8, rowTop + RowContentHeight, al_map_rgb(43, 65, 91));
      char number[32];
      std::snprintf(number, sizeof(number), "%s%3zu", i == deck.CurrentBlock() ? ">" : " ", i + 1);
      al_draw_text(font, i == deck.CurrentBlock() ? accent : white, 12, y, 0, number);
      std::string label = blocks[i].label + (blocks[i].checksumOK ? "" : " [bad checksum]");
      size_t maxChars = size_t(std::max(1.0f, (Width() - 140) / 8));
      if (label.size() > maxChars) label = label.substr(0, maxChars - 1) + "~";
      al_draw_text(font, white, 56, y, 0, label.c_str());
      char duration[32];
      std::snprintf(duration, sizeof(duration), "%.1fs", deck.DurationSeconds(i));
      al_draw_text(font, muted, Width() - 12, y, ALLEGRO_ALIGN_RIGHT, duration);
    }
    al_reset_clipping_rectangle();
    char status[160];
    if (deck.CurrentBlock() < blocks.size())
      std::snprintf(status, sizeof(status), "%s | Block %zu/%zu | %.1f / %.1f s", state.c_str(),
          deck.CurrentBlock() + 1, blocks.size(), deck.Seconds(), deck.DurationSeconds(deck.CurrentBlock()));
    else std::snprintf(status, sizeof(status), "%s | %zu blocks", state.c_str(), blocks.size());
    al_draw_text(font, white, 12, Height() - 39, 0, status);
    al_draw_filled_rectangle(12, Height() - 20, Width() - 12, Height() - 12, al_map_rgb(49, 62, 77));
    al_draw_filled_rectangle(12, Height() - 20, 12 + (Width() - 24) * (blocks.empty() ? 0 : deck.Progress()), Height() - 12, accent);
    al_flip_display();
    al_identity_transform(&transform);
    al_use_transform(&transform);
  }

private:
  constexpr static float Scale = 1.25f;
  enum { ListTop = 125, RowHeight = 22, RowContentHeight = 20 };
  ALLEGRO_DISPLAY *display = nullptr;
  ALLEGRO_EVENT_QUEUE *events = nullptr;
  ALLEGRO_FONT *font = nullptr;
  size_t selected = 0, observedBlock = 0;
  int first = 0;
  bool dirty = true, lastFast = true;
  std::string lastPath;
  TapeDeck::State lastState = TapeDeck::State::End;
  size_t lastBlock = 0;
  unsigned lastTick = 0;
  float Width() const { return al_get_display_width(display) / Scale; }
  float Height() const { return al_get_display_height(display) / Scale; }
  int VisibleRows() const { return std::max(1, int((Height() - ListTop - 52) / RowHeight)); }
  void KeepSelectionVisible() {
    if (!display) return;
    if (int(selected) < first) first = selected;
    if (int(selected) >= first + VisibleRows()) first = selected - VisibleRows() + 1;
  }
};

#endif
