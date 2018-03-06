#include "character.h" // class's header file

// class constructor
Character::Character() {
  pos_x = 0;
  pos_y = 0;
}

Character::Character(char* file) {
  pugi::xml_parse_result result = character_file.load_file(file);

  if(!result) {
      printf("Error: loading character data\n");      
  }

  // Iterate over states
  for (pugi::xml_node state = character_file.child("character").child("states").first_child();
       state; state = state.next_sibling()) {
    // Create state
    pugi::xml_node animation = state.child("animation");
    // Create animation and attach to state

    // Traverse all sprites in the animation
    for (pugi::xml_node sprite = animation.first_child(); sprite; sprite = sprite.next_sibling()) {

    }    
  }  
}

// class destructor
Character::~Character() {
}
