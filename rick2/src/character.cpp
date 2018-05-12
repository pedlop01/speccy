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
  int tile_weight = map->GetTilesetTileHeight();
  int character_weight = 8; // REVISIT

  // Compute the displacement in x
  int desp_x = (x > pos_x) ? (x - pos_x) : (pos_x - x);

  // Compute x and y corrections to draw world
  int tile_col_x;
  int tile_col_up_y   = pos_y / tile_weight;
  int tile_col_down_y = (pos_y + character_weight - 1) / tile_weight;
  if ( x > pos_x) {
    // Collision moving right
    tile_col_x = (pos_x + desp_x + character_weight)  / tile_weight;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != 308) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != 308)) {
      // No collision
      pos_x = pos_x + desp_x;      
    } else {
      // Collision. Move to safe position
      int correction = (pos_x + desp_x + character_weight) % tile_weight;
      pos_x = pos_x + desp_x - correction;
    }
  } else if ((x > 0) && (x < pos_x)) {
    // Collision moving left    
    tile_col_x = (pos_x - desp_x) / tile_weight;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != 308) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != 308)) {
      // No collision
      pos_x = pos_x - desp_x;
    } else {
      // Collision. Move to safe position
      int correction = tile_weight - ((pos_x - desp_x) % tile_weight);
      pos_x = pos_x - desp_x + correction;      
    }
  }
}

void Character::SetPosY(World* map, int y) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.
  int tile_height = map->GetTilesetTileHeight();
  int character_height = 8; // REVISIT

  // Compute the displacement in x
  int desp_y = (y > pos_y) ? (y - pos_y) : (pos_y - y);

  // Compute x and y corrections to draw world
  int tile_col_y;
  int tile_col_up_x   = pos_x / tile_height;
  int tile_col_down_x = (pos_x + character_height - 1) / tile_height;
  if (y > pos_y) {
    // Collision moving down
    tile_col_y = (pos_y + desp_y + character_height)  / tile_height;
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != 308) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != 308)) {
      // No collision
      pos_y = pos_y + desp_y;
    } else {
      // Collision. Move to safe position
      int correction = (pos_y + desp_y + character_height) % tile_height;
      pos_y = pos_y + desp_y - correction;
    }
  } else if ((y > 0) && (y < pos_y)) {
    // Collision moving up    
    tile_col_y = (pos_y - desp_y) / tile_height;
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != 308) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != 308)) {
      // No collision
      pos_y = pos_y - desp_y;
    } else {
      // Collision. Move to safe position
      int correction = tile_height - ((pos_y - desp_y) % tile_height);
      pos_y = pos_y - desp_y + correction;
    }
  }
}
