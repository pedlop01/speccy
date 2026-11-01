#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <algorithm>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

using namespace std;

// Validate TAP framing before starting; malformed files must not overrun memory.
bool ValidateTape(const vector<unsigned char> &data) {
  size_t pos = 0;
  while (pos < data.size()) {
    if (data.size() - pos < 2) return false;
    size_t length = data[pos] | (data[pos + 1] << 8);
    pos += 2;
    if (length < 2 || length > data.size() - pos) return false;
    pos += length;
  }
  return true;
}

bool LoadTrap(Z80 &cpu, const vector<unsigned char> &data, size_t &pos) {
  if (pos == data.size()) return false;
  size_t length = data[pos] | (data[pos + 1] << 8);
  pos += 2;
  const size_t end = pos + length;
  const unsigned char flag = data[pos++];
  unsigned char checksum = flag;
  const unsigned char expectedFlag = cpu.regs.altAF >> 8;
  const bool load = (cpu.regs.altAF & 1) != 0;
  bool success = flag == expectedFlag && length - 2 == cpu.regs.DE;
  while (pos < end - 1) {
    unsigned char value = data[pos++];
    checksum ^= value;
    if (flag == expectedFlag && cpu.regs.DE) {
      if (load) cpu.DataBus->Write(cpu.regs.IX, value);
      else if (cpu.DataBus->Read(cpu.regs.IX) != value) success = false;
      ++cpu.regs.IX;
      --cpu.regs.DE;
    }
  }
  success = success && checksum == data[pos];
  pos = end;
  return success;
}

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


// One emulated frame produces one immutable audio fragment. Instruction
// overshoot is retained in the ULA, including interrupt acknowledge cycles.
void EmulateFrame(Z80 &cpu, ULA &ula, const vector<unsigned char> &tape, size_t &pos) {
  bool irq = false;
  while (!irq) {
    if (cpu.regs.PC == 0x056B) {
      const bool loaded = LoadTrap(cpu, tape, pos);
      if (loaded) {
        cpu.regs.BC = 0xB001;
        cpu.regs.altAF = 0x0145;
      }
      cpu.regs.CF = loaded;
      cpu.regs.PC = 0x05E2;
    }
    cpu.tStates = 0;
    cpu.EmulateOne();
    ula.AddCycles(cpu.tStates, irq);
  }
  cpu.tStates = 0;
  cpu.INT();
  bool unused = false;
  ula.AddCycles(cpu.tStates, unused);
}

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s tape.tap [48.rom]\n", argv[0]);
    return 1;
  }
  ifstream input(argv[1], ios::binary);
  if (!input) {
    fprintf(stderr, "Cannot open TAP file: %s\n", argv[1]);
    return 1;
  }
  vector<unsigned char> tape((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
  if (!ValidateTape(tape)) {
    fprintf(stderr, "Malformed TAP file\n");
    return 1;
  }

  Z80 cpu;
  ULA ula;
  Bus16 bus;
  ROM<0, 16384> rom;
  RAM<32768, 32 * 1024> ram;
  if (rom.Load(argc == 3 ? argv[2] : "48.rom")) {
    fprintf(stderr, "Cannot load 16 KB Spectrum ROM (use optional ROM path)\n");
    return 1;
  }
  bus.AddBusComponent(&rom);
  bus.AddBusComponent(&ram);
  bus.AddBusComponent(static_cast<ULAMemory *>(&ula));
  cpu.DataBus = &bus;
  cpu.IOBus = static_cast<ULAIO *>(&ula);

  if (!al_init() || !al_install_keyboard() || !al_install_audio()) {
    fprintf(stderr, "Cannot initialize Allegro, keyboard or audio\n");
    return 1;
  }
  // Explicit ownership keeps every error and normal exit on the same cleanup path.
  al_set_new_display_flags(ALLEGRO_WINDOWED | ALLEGRO_RESIZABLE);
  al_set_new_display_option(ALLEGRO_VSYNC, 0, ALLEGRO_SUGGEST);
  unique_ptr<ALLEGRO_DISPLAY, decltype(&al_destroy_display)> display(
      al_create_display(640, 480), al_destroy_display);
  if (!display) { fprintf(stderr, "Cannot create display\n"); return 1; }
  al_set_window_title(display.get(), "Gomas - ZX Spectrum 48K");
  al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP); // nearest neighbour: no filtering flags
  unique_ptr<ALLEGRO_BITMAP, decltype(&al_destroy_bitmap)> bitmap(
      al_create_bitmap(320, 240), al_destroy_bitmap);
  unique_ptr<ALLEGRO_EVENT_QUEUE, decltype(&al_destroy_event_queue)> events(
      al_create_event_queue(), al_destroy_event_queue);
  unique_ptr<ALLEGRO_VOICE, decltype(&al_destroy_voice)> voice(
      al_create_voice(AUDIO_SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_INT16,
                      ALLEGRO_CHANNEL_CONF_2), al_destroy_voice);
  unique_ptr<ALLEGRO_MIXER, decltype(&al_destroy_mixer)> mixer(
      al_create_mixer(AUDIO_SAMPLE_RATE, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                      ALLEGRO_CHANNEL_CONF_2), al_destroy_mixer);
  unique_ptr<ALLEGRO_AUDIO_STREAM, decltype(&al_destroy_audio_stream)> stream(
      al_create_audio_stream(3, SAMPLES_PER_FRAME, AUDIO_SAMPLE_RATE,
          ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_1), al_destroy_audio_stream);
  if (!bitmap || !events || !voice || !mixer || !stream ||
      !al_attach_mixer_to_voice(mixer.get(), voice.get())) {
    fprintf(stderr, "Cannot create video/audio resources\n");
    return 1;
  }
  ula.SetBitmap(bitmap.get());
  al_register_event_source(events.get(), al_get_keyboard_event_source());
  al_register_event_source(events.get(), al_get_display_event_source(display.get()));
  al_register_event_source(events.get(), al_get_audio_stream_event_source(stream.get()));

  size_t tapePos = 0;
  // Prime all fragments before playback; the audio device then paces emulation.
  auto fillAudio = [&]() {
    while (void *fragment = al_get_audio_stream_fragment(stream.get())) {
      EmulateFrame(cpu, ula, tape, tapePos);
      memcpy(fragment, ula.FrameAudio, sizeof(ula.FrameAudio));
      if (!al_set_audio_stream_fragment(stream.get(), fragment)) return false;
    }
    return true;
  };
  if (!fillAudio() || !al_attach_audio_stream_to_mixer(stream.get(), mixer.get())) {
    fprintf(stderr, "Cannot start audio stream\n");
    return 1;
  }

  bool running = true;
  while (running) {
    ALLEGRO_EVENT event;
    al_wait_for_event(events.get(), &event);
    switch (event.type) {
      case ALLEGRO_EVENT_DISPLAY_CLOSE: running = false; break;
      case ALLEGRO_EVENT_DISPLAY_RESIZE: al_acknowledge_resize(display.get()); break;
      case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
        for (unsigned row = 0; row < 4; ++row)
          for (unsigned col = 0; col < 10; ++col) ula.PressKey(row, col, false);
        break;
      case ALLEGRO_EVENT_KEY_DOWN:
        if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) running = false;
        else ReadKeyboard(event, &ula);
        break;
      case ALLEGRO_EVENT_KEY_UP: ReadKeyboard(event, &ula); break;
      case ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT:
        if (!fillAudio()) { fprintf(stderr, "Cannot submit audio fragment\n"); return 1; }
        break;
    }
    if (!running) break;
    if (!ula.Present()) { fprintf(stderr, "Cannot upload frame\n"); return 1; }
    al_set_target_backbuffer(display.get());
    al_clear_to_color(al_map_rgb(0, 0, 0));
    int width = al_get_display_width(display.get());
    int height = al_get_display_height(display.get());
    float scale = min(width / 320.0f, height / 240.0f);
    if (scale >= 1) scale = static_cast<int>(scale);
    float w = 320 * scale, h = 240 * scale;
    al_draw_scaled_bitmap(bitmap.get(), 0, 0, 320, 240,
                          (width - w) / 2, (height - h) / 2, w, h, 0);
    al_flip_display();
  }
  return 0;
}
