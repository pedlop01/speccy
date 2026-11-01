#include <cassert>
#include <cstdio>
#include <set>
#include "../emulator.h"

// Full BASIC LOAD "", six ROM loads, trainer answers and keyboard game start.
// It also checks the machine frame loop disables the ROM trap in pulse mode.
static void Cobra(bool fast) {
  SpectrumMachine machine;
  std::string error;
  assert(machine.LoadROM(FindROM("48.rom"), error));
  assert(machine.Open("cobra.tap", error));
  machine.SetFastLoad(fast);
  int finished = -1;
  std::set<unsigned> screens;
  for (int frame = 0; frame < 30000; ++frame) {
    auto key = [&](int when, int row, int col) {
      if (frame == when) machine.ula.PressKey(row, col, true);
      if (frame == when + 10) machine.ula.PressKey(row, col, false);
    };
    key(500, 2, 6); // J -> LOAD
    if (frame == 520 || frame == 540) machine.ula.PressKey(3, 8, true);
    key(520, 1, 9); key(540, 1, 9); // SYMBOL SHIFT + P -> two quotes
    if (frame == 550) machine.ula.PressKey(3, 8, false);
    key(560, 2, 9); // Enter
    if (frame == 570 && !fast) assert(machine.PlayTape());
    if (finished >= 0) {
      key(finished + 300, 3, 6); // N: infinite lives
      key(finished + 500, 3, 6); // N: invulnerability
      key(finished + 700, 0, 0);
      key(finished + 1100, 0, 0); // Keyboard option, allowing menu intro to finish.
    }
    machine.Frame();
    if (finished < 0 && machine.deck.CurrentBlock() == machine.deck.Blocks().size()) {
      finished = frame;
      if (fast) assert(frame < 1000);
      else assert(frame > 10000); // Several minutes of tape actually elapsed.
    }
    if (finished >= 0 && frame > finished + 1500) {
      unsigned hash = 0;
      for (unsigned address = 0x4000; address < 0x5B00; ++address)
        hash = hash * 33 + machine.bus.Read(address);
      screens.insert(hash);
    }
    if (finished >= 0 && frame == finished + 2000) {
      assert(screens.size() > 30); // Gameplay changes, not a static menu/loading image.
      printf("Cobra %s: loaded at frame %d; %zu changing gameplay frames\n",
          fast ? "fast" : "pulses", finished, screens.size());
      return;
    }
  }
  assert(false && "Cobra failed to load/start within the emulated time limit");
}

int main() {
  Cobra(true);
  Cobra(false);
}
