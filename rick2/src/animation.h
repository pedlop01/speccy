#ifndef ANIMATION_H
#define ANIMATION_H

#include <allegro5/allegro.h>
#include <vector>
#include "sprite.h" // inheriting class's header file

using namespace std;

typedef class Animation* animation_ptr;
class Animation
{
  public:
    ALLEGRO_BITMAP*    source_bitmap;
    vector<sprite_ptr> sprites;
    int                speed;
    int                current_anim;
    int                steps_in_anim;

  public:
	  Animation();    // class constructor
	  ~Animation();   // class destructor

    Animation(ALLEGRO_BITMAP* _source_bitmap, int _speed);

    void AddSprite(int _x, int _y, int _width, int _height);

    void AnimStep();
    void ResetAnim();

    int GetCurrentAnim() { return current_anim; }
};

#endif // ANIMATION_H
