#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "pugixml.hpp"
#include "world.h"
#include "keyboard.h"
#include "colbox.h"

// REVISIT: state will be provided from a file
#define RICK_STATE_STOP     0
#define RICK_STATE_JUMPING  1
#define RICK_STATE_RUNNING  2
#define RICK_STATE_CLIMBING 3

// REVISIT: direction will be provided from a file
#define RICK_DIR_STOP  0b0000
#define RICK_DIR_RIGHT 0b0001
#define RICK_DIR_LEFT  0b0010
#define RICK_DIR_UP    0b0100
#define RICK_DIR_DOWN  0b1000

class Character {
  private:
    int pos_x;
    int pos_y;

    int pos_x_chk;
    int pos_y_chk;

    int height;
    int width;

    int height_internal;
    int width_internal;

    int state;
    int direction;

    float speed_x;
    float speed_y;

    Colbox extColInt;
    Colbox extColExt;
    Colbox extHeightColExt;
    Colbox extWidthColExt;
    Colbox heightColInt;
    Colbox heightColExt;
    Colbox widthColInt;
    Colbox widthColExt;

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
    int  GetWidth()     { return width;    }
    int  GetState()     { return state;     }
    int  GetDirection() { return direction; }

    void GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int width, int height);
    void GetCollisionsExternalBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalWidthBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalHeightBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalBoxInt(World* map, Colbox &mask_col);
    void GetCollisionsInternalWidthBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsInternalWidthBoxInt(World* map, Colbox &mask_col);
    void GetCollisionsInternalHeightBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsInternalHeightBoxInt(World* map, Colbox &mask_col);

    void ComputeCollisions(World* map);
    void ComputeNextState(Keyboard& keyboard);
    void ComputeNextPosition(World* map);
};

#endif // CHARACTER_H
