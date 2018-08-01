#include "enemy.h"
#include "character.h"

// class constructor
Enemy::Enemy() : Character() {

}

Enemy::Enemy(const char* file,
             int _id,
             int _x, int _y,
             int _bb_x, int _bb_y, int _bb_width, int _bb_height,
             int _direction) : Character(file) {
  id = _id;

  pos_x = _x;
  pos_y = _y;

  using_bb    = true;
  width       = _bb_width;
  height      = _bb_height;
  width_orig  = width;
  height_orig = height;

  bb_x = _bb_x;
  bb_y = _bb_y;
  bb_width = _bb_width;
  bb_width_orig = bb_width;
  bb_height = _bb_height;
  bb_height_orig = bb_height;

  direction = _direction;
}

// class destructor
Enemy::~Enemy() {  

}
