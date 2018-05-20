#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>

#include "pugixml.hpp"
#include "world.h"
#include "keyboard.h"

// REVISIT: state will be provided from a file
#define RICK_STOP     0
#define RICK_FALLING  1
#define RICK_JUMPING  2
#define RICK_RUNNING  3
#define RICK_CLIMBING 4

class Character {
  private:
    int pos_x;
    int pos_y;

    int height;
    int weight;

    int state;
    int direction;

    pugi::xml_document character_file;

  public:    
	  Character();    // class constructor
    Character(char* file);    

    ~Character();   // class desructor
    
    void SetPosX(World* map, int x);
    void SetPosY(World* map, int y);

    int  GetPosX()      { return pos_x;     }
    int  GetPosY()      { return pos_y;     }
    int  GetHeight()    { return height;    }
    int  GetWeight()    { return weight;    }
    int  GetState()     { return state;     }
    int  GetDirection() { return direction; }

    void ComputeNextState(int mask_col_ext, int mask_col_ver_int, int mask_col_hor_int, Keyboard& keyboard);
};

#endif // CHARACTER_H
