#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <set>
#include "../emulator.h"

static void Frames(SpectrumMachine &m, unsigned count) { while (count--) m.Frame(); }
static void Key(SpectrumMachine &m, unsigned row, unsigned col) {
  m.ula.PressKey(row, col, true); Frames(m, 5);
  m.ula.PressKey(row, col, false); Frames(m, 5);
}
static std::string ScreenText(SpectrumMachine &m) {
  const auto &font = m.systemROM.pages[m.systemROM.pages.size() == 1 ? 0 : 1];
  std::string text;
  for (unsigned row = 0; row < 24; ++row) {
    for (unsigned col = 0; col < 32; ++col) {
      char match = '?';
      for (unsigned ch = 32; ch < 127; ++ch) {
        bool same = true;
        for (unsigned line = 0; line < 8; ++line) {
          unsigned y = row * 8 + line;
          unsigned offset = ((y & 0xC0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2) | col;
          if (m.memory.ReadBank(m.memory.ScreenBank(), offset) != font[0x3C00 + ch * 8 + line]) same = false;
        }
        if (same) { match = ch; break; }
      }
      text += match;
    }
    text += '\n';
  }
  return text;
}
static void Basic(const char *rom) {
  SpectrumMachine m; std::string error;
  assert(m.LoadROM(FindROM(rom), error)); Frames(m, 200);
  bool spanish = std::string(rom) == "128-spanish-0.rom";
  if (!spanish) {
    assert(ScreenText(m).find("128 BASIC") != std::string::npos);
    m.ula.PressKey(3, 0, true); Key(m, 0, 5); m.ula.PressKey(3, 0, false); // CAPS+6 down.
    Key(m, 2, 9); Frames(m, 200);
  }
  // Use the 128K editor to enter PRINT 1+1, rather than jumping into ROM code.
  Key(m, 1, 9); Key(m, 1, 3); Key(m, 1, 7); Key(m, 3, 6); Key(m, 1, 4);
  Key(m, 3, 9); Key(m, 0, 0);
  m.ula.PressKey(3, 8, true); Key(m, 2, 7); m.ula.PressKey(3, 8, false);
  Key(m, 0, 0); Key(m, 2, 9); Frames(m, 100);
  const auto text = ScreenText(m);
  assert(text.find("2 ") == 0 || text.find("\n2 ") != std::string::npos);
  assert(!m.memory.PagingLocked());
  if (!spanish) {
    m.Reset(); Frames(m, 200);
    for (unsigned i = 0; i < 3; ++i) {
      m.ula.PressKey(3, 0, true); Key(m, 0, 5); m.ula.PressKey(3, 0, false);
    }
    Key(m, 2, 9); Frames(m, 200);
    assert(m.memory.PagingLocked() && m.memory.ROMPage() == 1);
    Key(m, 1, 9); Key(m, 0, 1); Key(m, 2, 9); Frames(m, 30); // PRINT 2 in 48 BASIC.
    assert(ScreenText(m).find("2 ") != std::string::npos);
  }
  printf("%s: menu/BASIC, keyboard calculation and compatibility mode passed\n", rom);
}
static void Back3(bool fast) {
  SpectrumMachine m; std::string error;
  assert(m.LoadROM(FindROM("128-0.rom"), error));
  assert(m.Open("back3.tap", error)); m.SetFastLoad(fast);
  int intro = -1, menu = -1;
  std::set<unsigned> screens, music;
  unsigned audibleFrames = 0;
  for (int frame = 0; frame < 30000; ++frame) {
    auto key = [&](int when, unsigned row, unsigned col) {
      if (frame == when) m.ula.PressKey(row, col, true);
      if (frame == when + 10) m.ula.PressKey(row, col, false);
    };
    key(100, 2, 9); // Tape Loader in the real 128K menu.
    if (frame == 120 && !fast) assert(m.PlayTape());
    if (intro >= 0) key(intro + 400, 3, 9); // Continue intro.
    // Resume the cued block only once the game's animation has finished and
    // LD-BYTES is waiting for its pilot. An early Play would miss the flag.
    if (intro >= 0 && !fast && m.deck.CurrentBlock() == 8 && !m.deck.IsPlaying() &&
        m.cpu.regs.PC >= 0x0556 && m.cpu.regs.PC <= 0x0605) assert(m.PlayTape());
    if (menu >= 0) { key(menu + 1600, 0, 0); key(menu + 1800, 3, 9); }
    m.Frame();
    if (intro < 0 && m.deck.CurrentBlock() == 8) {
      intro = frame;
      if (!fast) m.StopTape(); // Keep block 9 ready throughout the intro.
    }
    if (menu < 0 && m.deck.CurrentBlock() >= 10) {
      menu = frame;
      if (!fast) m.StopTape(); // Preserve the later stages for the next multicarga.
    }
    if (intro >= 0 && menu < 0 && m.cpu.regs.PC >= 0x8000) {
      unsigned hash = 0; bool audible = false;
      for (unsigned i = 0; i < m.ula.FrameAudioCount; ++i) {
        hash = hash * 33 + static_cast<unsigned short>(m.ula.FrameAudio[i]);
        audible |= std::abs(int(m.ula.FrameAudio[i])) > 100;
      }
      if (audible) { ++audibleFrames; music.insert(hash); }
    }
    if (menu >= 0 && frame > menu + 1850) {
      unsigned hash = 0;
      for (unsigned address = 0x4000; address < 0x5800; ++address) hash = hash * 33 + m.bus.Read(address);
      screens.insert(hash);
    }
    if (menu >= 0 && frame == menu + 2250) {
      assert(intro >= 0 && audibleFrames > 30 && music.size() > 20);
      assert(screens.size() > 10); // Actual moving first stage, beyond static instructions.
      assert(m.deck.CurrentBlock() == 10); // Later loads have not been consumed.
      assert(m.memory.ReadBank(3, 6) == 0xB7); // Sample data survived independent banking.
      printf("back3 %s: intro=%d menu=%d, %u music frames, %zu changing gameplay screens\n",
          fast ? "fast" : "pulses", intro, menu, audibleFrames, screens.size());
      return;
    }
  }
  assert(false && "back3 failed to reach the first playable stage");
}
int main() {
  for (const char *rom : {"128-0.rom", "128-spanish-0.rom", "plus2-0.rom"}) Basic(rom);
  Back3(true); Back3(false);
}
