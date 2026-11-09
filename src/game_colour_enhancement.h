#ifndef GAME_COLOUR_ENHANCEMENT_H
#define GAME_COLOUR_ENHANCEMENT_H
#include "army_colour_enhancement.h"

// Select once per session, using exact tape content rather than its filename.
class GameColourEnhancement {
public:
  bool enabled=false;
  void Attach(SpectrumMachine &m){generation=~std::uint64_t(0);Prepare(m);}
  void Prepare(SpectrumMachine &m) {
    if(generation!=m.SessionGeneration()) {
      generation=m.SessionGeneration();armySelected=ArmyColourEnhancement::Matches(m);
      if(armySelected)army.Attach(m);else manic.Attach(m);
    }
    army.enabled=enabled;manic.enabled=enabled;
  }
  bool Present(SpectrumMachine &m,ALLEGRO_BITMAP *bitmap) {
    Prepare(m);
    const bool available=armySelected?army.Available(m):manic.Available(m);
    if(!available && painted){m.ula.SetBitmap(bitmap);if(!m.ula.Present())return false;}
    painted=available;
    return armySelected?army.Present(m,bitmap):manic.Present(m,bitmap);
  }
private:
  std::uint64_t generation=~std::uint64_t(0);
  bool armySelected=false,painted=false;
  ManicColourEnhancement manic;
  ArmyColourEnhancement army;
};
#endif
