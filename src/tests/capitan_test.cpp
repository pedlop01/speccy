#include <cassert>
#include <cstdio>
#include <set>
#include "../emulator.h"

static void Capitan(bool fast) {
  SpectrumMachine machine;
  std::string error;
  assert(machine.LoadROM(FindROM("48.rom"), error));
  assert(machine.Open("capitan1.tap", error));
  machine.SetFastLoad(fast);
  int loaded = -1, started = -1;
  std::set<unsigned> screens;
  for (int frame = 0; frame < 30000; ++frame) {
    auto key = [&](int when, unsigned row, unsigned col) {
      if (frame == when) machine.ula.PressKey(row, col, true);
      if (frame == when + 10) machine.ula.PressKey(row, col, false);
    };
    key(500, 2, 6);
    if (frame == 520 || frame == 540) machine.ula.PressKey(3, 8, true);
    key(520, 1, 9); key(540, 1, 9);
    if (frame == 550) machine.ula.PressKey(3, 8, false);
    key(560, 2, 9);
    if (frame == 570 && !fast) assert(machine.PlayTape());
    if (loaded >= 0) key(loaded + 600, 0, 0); // 1 Teclado in the actual game menu.
    machine.Frame();
    if (loaded < 0 && machine.deck.CurrentBlock() == machine.deck.Blocks().size()) loaded = frame;
    if (loaded >= 0 && frame > loaded + 600) {
      if (started < 0) started = frame;
      // A reset would return to IM 1 / ROM interrupt 0038, destroy IX and
      // overwrite these game vectors. Check ongoing execution, not just pixels.
      assert(machine.cpu.regs.PC >= 0x4000 && machine.cpu.regs.IM == 2);
      assert(machine.bus.Read(0xEEEE) == 0xC3);
      if (frame > loaded + 800) {
        unsigned hash = 0;
        for (unsigned a = 0x4000; a < 0x5B00; ++a) hash = hash * 33 + machine.bus.Read(a);
        screens.insert(hash);
      }
    }
    if (started >= 0 && frame == loaded + 2200) {
      assert(screens.size() > 20);
      printf("Capitan Trueno %s: loaded at frame %d; no reset, %zu changing gameplay screens\n",
          fast ? "fast" : "pulses", loaded, screens.size());
      return;
    }
  }
  assert(false && "Capitan Trueno failed to load and enter gameplay");
}
int main() { Capitan(true); Capitan(false); }
