#ifndef ENEMY_H
#define ENEMY_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "character.h"
#include "pugixml.hpp"


class Enemy : public Character {
  private:

  public:    
	  Enemy();
    Enemy(const char* file);    

    ~Enemy();

    // Methods visible to the IA to take decisions
};

#endif // ENEMY_H
