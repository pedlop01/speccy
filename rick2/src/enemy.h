#ifndef ENEMY_H
#define ENEMY_H

#include <stdio.h>
#include <vector>

#include "pugixml.hpp"
#include "rick_params.h"
#include "character.h"
#include "enemy_ia.h"
#include "colbox.h"


class World;
class Enemy : public Character {
  private:
    int id;
    EnemyIA* ia;
    Colbox weightColExt;

  public:    
	  Enemy();
    Enemy(const char* file,
          int _id,
          int _x, int _y,
          int _bb_x, int _bb_y, int _bb_width, int _bb_height,
          int _direction, float _speed_x, float _speed_y,
          int _ia_type, bool _ia_random, int _ia_limit_x, int _ia_limit_y);

    ~Enemy();

    int GetId() { return id; }

    void CharacterStep(World* map, Character* player);
};

#endif // ENEMY_H
