#include <cassert>
#include <cstdio>
#include <set>
#include "../emulator.h"

static void MenuText(SpectrumMachine &m, unsigned row, unsigned col, const char *text) {
  // Bomb Jack's own eight-byte font begins at 8FFA. Verify actual bitmap
  // cells at the positions encoded by AT control codes in its menu strings.
  for (; *text; ++text, ++col) {
    assert(col < 32);
    for (unsigned line = 0; line < 8; ++line) {
      const unsigned y = row * 8 + line;
      const unsigned address = 0x4000 + (((y & 0xC0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | col);
      const unsigned font = 0x8FFA + (static_cast<unsigned char>(*text) - 32) * 8 + line;
      assert(m.bus.Read(address) == m.bus.Read(font));
    }
  }
}
static void BombJack(bool fast) {
  SpectrumMachine m; std::string error;
  assert(m.LoadROM(FindROM("48.rom"), error));
  assert(m.Open("bombjack.tap", error)); m.SetFastLoad(fast);
  int loaded = -1;
  bool menuChecked = false;
  std::set<unsigned> screens;
  for (int frame = 0; frame < 30000; ++frame) {
    auto key = [&](int when, unsigned row, unsigned col) {
      if (frame == when) m.ula.PressKey(row, col, true);
      if (frame == when + 10) m.ula.PressKey(row, col, false);
    };
    key(500, 2, 6);
    if (frame == 520 || frame == 540) m.ula.PressKey(3, 8, true);
    key(520, 1, 9); key(540, 1, 9);
    if (frame == 550) m.ula.PressKey(3, 8, false);
    key(560, 2, 9);
    if (frame == 570 && !fast) assert(m.PlayTape());
    if (loaded >= 0) key(loaded + 600, 0, 0); // 1 - One Player Start.
    m.Frame();
    if (loaded < 0 && m.deck.CurrentBlock() == m.deck.Blocks().size()) loaded = frame;
    if (loaded >= 0 && frame == loaded + 400) {
      MenuText(m, 3, 2, "Elite Systems Presents");
      MenuText(m, 8, 3, "Licensed From Tehkan");
      MenuText(m, 9, 6, "Copyright 1985");
      MenuText(m, 10, 3, "All Rights Reserved");
      MenuText(m, 13, 3, "1 - One Player Start");
      MenuText(m, 14, 3, "2 - Two Player Start");
      MenuText(m, 17, 4, "K - Keyboard");
      MenuText(m, 18, 4, "T - Keyboard(Turbo)");
      menuChecked = true;
    }
    if (loaded >= 0 && frame > loaded + 900) {
      assert(m.cpu.regs.IM == 2 && m.cpu.regs.PC >= 0x4000);
      unsigned hash = 0;
      for (unsigned a = 0x4000; a < 0x5B00; ++a) hash = hash * 33 + m.bus.Read(a);
      screens.insert(hash);
    }
    if (loaded >= 0 && frame == loaded + 1600) {
      assert(menuChecked && screens.size() > 20);
      printf("Bomb Jack %s: clean menu glyphs at their expected positions; %zu changing gameplay screens\n",
          fast ? "fast" : "pulses", screens.size());
      return;
    }
  }
  assert(false && "Bomb Jack failed to load and enter gameplay");
}
int main() { BombJack(true); BombJack(false); }
