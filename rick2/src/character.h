#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>

#include "pugixml.hpp"
#include "world.h"

class Character {
  private:
    int pos_x;
    int pos_y;

    int height;
    int weight;

    pugi::xml_document character_file;

  public:    
	  Character();    // class constructor
    Character(char* file);    

    ~Character();   // class desructor
    
    void SetPosX(World* map, int x);
    void SetPosY(World* map, int y);

    int  GetPosX()      { return pos_x;  }
    int  GetPosY()      { return pos_y;  }
    int  GetHeight()    { return height; }
    int  GetWeight()    { return weight; }
};

#endif // CHARACTER_H
