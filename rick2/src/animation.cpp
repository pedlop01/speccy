#include "animation.h" // class's header file

// class constructor
Animation::Animation() {
  current_anim = 0;
  steps_in_anim = 0;
}

Animation::Animation(ALLEGRO_BITMAP* _source_bitmap, int _speed) {
  source_bitmap = _source_bitmap;
  speed = _speed;
  current_anim = 0;
  steps_in_anim = 0;
}

// class destructor
Animation::~Animation()
{
    for (vector<sprite_ptr>::iterator it = sprites.begin() ; it != sprites.end() ; it++ ) {
        delete *it;
    }
    sprites.clear();

    al_destroy_bitmap(source_bitmap);
}

void Animation::AddSprite(int _x, int _y, int _width, int _height) {
  sprite_ptr sprite = new Sprite(_x, _y, _width, _height);
  sprites.push_back(sprite);
}

void Animation::AnimStep() {
  steps_in_anim++;
  if (steps_in_anim > speed) {
    current_anim = (current_anim + 1) % sprites.size();
    steps_in_anim = 0;
  }
}

void Animation::ResetAnim() {
  current_anim = 0;
  steps_in_anim = 0;
}