#ifndef ANIMATION_H
#define ANIMATION_H

#include <allegro5/allegro.h>
#include <list>
#include "sprite.h" // inheriting class's header file

using namespace std;

class Animation
{
  public:
    ALLEGRO_BITMAP*  source_bitmap;
    list<sprite_ptr> sprites;
    int              speed;

  public:
	  Animation();    // class constructor
	  ~Animation();   // class destructor
};

#endif // ANIMATION_H
