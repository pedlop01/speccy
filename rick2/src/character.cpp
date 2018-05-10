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

void Character::SetPosX(World* map, int x) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.

  // Compute x and y corrections to draw world
  int tile_col_pos_x, tile_col_x;
  int tile_col_pos_y, tile_col_y;
  // Collision moving right
  if ( x > pos_x) {    
    tile_col_x = (x + 8 - 1)  / map->GetTilesetTileWidth();
    tile_col_y = pos_y / map->GetTilesetTileHeight();    
    if (map->GetTile(tile_col_x, tile_col_y)->GetType() != 308) {
      pos_x = x;      
    } else {      
      pos_x = x - 1;
    }
  }
  // Collision moving left
  if ((x > 0) && (x < pos_x)) {
    tile_col_x = (x - 1) / map->GetTilesetTileWidth();
    tile_col_y = pos_y / map->GetTilesetTileHeight();    
    if (map->GetTile(tile_col_x, tile_col_y)->GetType() != 308)
      pos_x = x;
    else
      pos_x = x + 1;
  }
}

void Character::SetPosY(World* map, int y) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.

  // Compute x and y corrections to draw world
  int tile_col_pos_x, tile_col_x;
  int tile_col_pos_y, tile_col_y;
  // Collision moving right
  if ( y > pos_y) {
    tile_col_x = pos_x  / map->GetTilesetTileWidth();
    tile_col_y = (y + 8 - 1) / map->GetTilesetTileHeight();
    if (map->GetTile(tile_col_x, tile_col_y)->GetType() != 308) {
      pos_y = y;
    } else {      
      pos_y = y - 1;
    }
  }
  // Collision moving left
  if ((y > 0) && (y < pos_y)) {
    tile_col_x = pos_x / map->GetTilesetTileWidth();
    tile_col_y = (y - 1) / map->GetTilesetTileHeight();    
    if (map->GetTile(tile_col_x, tile_col_y)->GetType() != 308)
      pos_y = y;
    else
      pos_y = y + 1;
  }
}
