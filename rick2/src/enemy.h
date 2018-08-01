#ifndef ENEMY_H
#define ENEMY_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "character.h"
#include "pugixml.hpp"

class Enemy : public Character {
  private:
    int id;

  public:    
	  Enemy();
    Enemy(const char* file,
          int _id,
          int _x, int _y,
          int _bb_x, int _bb_y, int _bb_width, int _bb_height,
          int _direction);

    ~Enemy();

    // Methods visible to the IA to take decisions
    int GetId() { return id; }
};

#endif // ENEMY_H
