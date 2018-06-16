#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "pugixml.hpp"
#include "world.h"
#include "keyboard.h"

// REVISIT: state will be provided from a file
#define RICK_STOP     0
#define RICK_FALLING  1
#define RICK_JUMPING  2
#define RICK_RUNNING  3
#define RICK_CLIMBING 4


/* Character bounding boxes.
   The external box is defined by {height, width}
   The height internal box is defined by {height, width - width_internal}
   The width internal box is defined by {height - height_internal, width}

        /----/-----\----\
        |    | hi  |    |
        /---------------\
        |            wi |
        \---------------/
        |    |     |    |
        \----\-----/----/
*/

class Character {
  private:
    int pos_x;
    int pos_y;

    int height;
    int width;

    int height_internal;
    int width_internal;

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
    int  GetWidth()     { return width;    }
    int  GetState()     { return state;     }
    int  GetDirection() { return direction; }

    void GetCollisionsByCoords(World* map, vector<int> &mask_col, int left_up_x, int left_up_y, int width, int height);
    void GetCollisionsExternalBoxExt(World* map, vector<int> &mask_col);
    void GetCollisionsExternalBoxInt(World* map, vector<int> &mask_col);
    void GetCollisionsInternalWidthBoxExt(World* map, vector<int> &mask_col);
    void GetCollisionsInternalWidthBoxInt(World* map, vector<int> &mask_col);
    void GetCollisionsInternalHeightBoxExt(World* map, vector<int> &mask_col);
    void GetCollisionsInternalHeightBoxInt(World* map, vector<int> &mask_col);

    void ComputeNextState(vector<int> &mask_col_ext, vector<int> &mask_col_ver_int, vector<int> &mask_col_hor_int, Keyboard& keyboard);
};

#endif // CHARACTER_H
