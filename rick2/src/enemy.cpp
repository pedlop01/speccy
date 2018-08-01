#include "enemy.h"
#include "character.h"
#include "world.h"

// class constructor
Enemy::Enemy() : Character() {

}

Enemy::Enemy(const char* file,
             int _id,
             int _x, int _y,
             int _bb_x, int _bb_y, int _bb_width, int _bb_height,
             int _direction,
             int ia_type, bool ia_random, int ia_limit_x, int ia_limit_y) : Character(file) {
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

  ia = new EnemyIA(ENEMY_IA_WALKER, ia_random, pos_x, pos_y, ia_limit_x, ia_limit_y);
}

// class destructor
Enemy::~Enemy() {  

}

void Enemy::CharacterStep(World* map, Character* player) {
  // REVISIT: remove the speed_x forced
  speed_x = 1.0;

  this->GetCollisionsInternalWeightBoxExt(map, weightColExt);

  Keyboard keyboard_enemy;
  ia->IAStep(keyboard_enemy,
             direction, pos_x, pos_y,
             weightColExt.GetRightDownCol() == TILE_COL,
             weightColExt.GetLeftDownCol() == TILE_COL,
             stepsInDirectionX);
  Character::CharacterStep(map, keyboard_enemy);
}