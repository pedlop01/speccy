#include <cassert>
#include <cstdio>
#include <set>
#include <unistd.h>
#include "../emulator.h"

// Full BASIC LOAD "", six ROM loads, trainer answers and keyboard game start.
// It also checks the machine frame loop disables the ROM trap in pulse mode.
static void Cobra(bool fast,const char *path="cobra.tap") {
  SpectrumMachine machine;
  std::string error;
  assert(machine.LoadROM(FindROM("48.rom"), error));
  assert(machine.Open(path, error));
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
      printf("Cobra %s (%s): loaded at frame %d; %zu changing gameplay frames\n",path,
          fast ? "fast" : "pulses", finished, screens.size());
      return;
    }
  }
  assert(false && "Cobra failed to load/start within the emulated time limit");
}

int main() {
  Cobra(true);
  Cobra(false);
  std::ifstream input("cobra.tap",std::ios::binary);
  std::vector<unsigned char> tap((std::istreambuf_iterator<char>(input)),{});assert(!tap.empty());
  char path[]="/tmp/gomas-cobra-XXXXXX.tzx";int fd=mkstemps(path,4);assert(fd>=0);close(fd);
  for(bool turbo:{false,true}) {
    std::vector<unsigned char> tzx={'Z','X','T','a','p','e','!',26,1,20};
    auto word=[&](unsigned n){tzx.push_back(n);tzx.push_back(n>>8);};
    for(size_t p=0;p<tap.size();) {
      unsigned n=tap[p]|(tap[p+1]<<8);
      if(turbo) {
        tzx.push_back(0x11);word(2168);word(667);word(735);word(855);word(1710);
        word(tap[p+2]<128?8063:3223);tzx.push_back(8);word(1000);word(n);tzx.push_back(0);
        tzx.insert(tzx.end(),tap.begin()+p+2,tap.begin()+p+2+n);
      } else {tzx.push_back(0x10);word(1000);tzx.insert(tzx.end(),tap.begin()+p,tap.begin()+p+2+n);}
      p+=n+2;
    }
    {std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char *>(tzx.data()),tzx.size());assert(output.good());}
    if(!turbo)Cobra(true,path);
    Cobra(false,path);
  }
  unlink(path);
}
