#ifndef CHARACTER_H
#define CHARACTER_H

#include "pugixml.hpp"

class Character {
  private:
    pugi::xml_document character_file;

  public:    
	  Character();    // class constructor
    Character(char* file);

	  ~Character();   // class desructor
};

#endif // CHARACTER_H
