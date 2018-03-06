#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>
#include "pugixml.hpp"

class Character {
  private:
    int pos_x;
    int pos_y;
    pugi::xml_document character_file;

  public:    
	  Character();    // class constructor
    Character(char* file);

    ~Character();   // class desructor
    
    void SetPosX(int x) { pos_x = x; }
    void SetPosY(int y) { pos_y = y; }
    int  GetPosX()      { return pos_x; }
    int  GetPosY()      { return pos_y; }
};

#endif // CHARACTER_H
