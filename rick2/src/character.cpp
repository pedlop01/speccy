#include "character.h" // class's header file

// class constructor
Character::Character() {
  pos_x = 264;  // REVISIT: should be 0
  pos_y = 2000; // REVISIT: should be 0

  height = 16;  // REVISIT: should be 0
  width = 16;  // REVISIT: should be 0

  height_internal = 2;
  width_internal = 2;
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
  int tile_width = map->GetTilesetTileHeight();
  int character_width = this->height;

  // Compute the displacement in x
  int desp_x = (x > pos_x) ? (x - pos_x) : (pos_x - x);

  // Compute x and y corrections to draw world
  int tile_col_x;
  int tile_col_up_y   = pos_y / tile_width;
  int tile_col_down_y = (pos_y + character_width - 1) / tile_width;
  if ( x > pos_x) {
    // Collision moving right
    tile_col_x = (pos_x + desp_x + character_width)  / tile_width;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != TILE_COL)) {
      // No collision
      pos_x = pos_x + desp_x;      
    } else {
      // Collision. Move to safe position
      int correction = (pos_x + desp_x + character_width) % tile_width;
      pos_x = pos_x + desp_x - correction;
    }
  } else if ((x > 0) && (x < pos_x)) {
    // Collision moving left    
    tile_col_x = (pos_x - desp_x) / tile_width;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != TILE_COL)) {
      // No collision
      pos_x = pos_x - desp_x;
    } else {
      // Collision. Move to safe position
      int correction = tile_width - ((pos_x - desp_x) % tile_width);
      pos_x = pos_x - desp_x + correction;      
    }
  }
}

void Character::SetPosY(World* map, int y) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.
  int tile_height = map->GetTilesetTileHeight();
  int character_height = this->width;

  // Compute the displacement in x
  int desp_y = (y > pos_y) ? (y - pos_y) : (pos_y - y);

  // Compute x and y corrections to draw world
  int tile_col_y;
  int tile_col_up_x   = pos_x / tile_height;
  int tile_col_down_x = (pos_x + character_height - 1) / tile_height;
  if (y > pos_y) {
    // Collision moving down
    tile_col_y = (pos_y + desp_y + character_height)  / tile_height;
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) {
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
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) {
      // No collision
      pos_y = pos_y - desp_y;
    } else {
      // Collision. Move to safe position
      int correction = tile_height - ((pos_y - desp_y) % tile_height);
      pos_y = pos_y - desp_y + correction;
    }
  }
}

void Character::GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int width, int height) {
  mask_col.SetLeftUpCol(map->GetTileByCoord(left_up_x, left_up_y)->GetType());
  mask_col.SetRightUpCol(map->GetTileByCoord(left_up_x + width, left_up_y)->GetType());
  mask_col.SetRightDownCol(map->GetTileByCoord(left_up_x + width, left_up_y + height)->GetType());
  mask_col.SetLeftDownCol(map->GetTileByCoord(left_up_x, left_up_y + height)->GetType());
}

void Character::GetCollisionsExternalBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x - 1,
                              pos_y - 1,
                              width + 2,
                              height + 2);
}

void Character::GetCollisionsExternalBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y,
                              width + 1,
                              height + 1);
}

void Character::GetCollisionsInternalWidthBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x - 1,
                              pos_y + height_internal - 1,
                              width + 2,
                              height - 2*height_internal + 2);
}

void Character::GetCollisionsInternalWidthBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y + height_internal - 1,
                              width + 1,
                              height - 2*height_internal + 2);
}

void Character::GetCollisionsInternalHeightBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + width_internal - 1,
                              pos_y - 1,
                              width - 2*width_internal + 2,
                              height + 2);
}

void Character::GetCollisionsInternalHeightBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + width_internal - 1,
                              pos_y,
                              width - 2*width_internal + 2,
                              height + 1);
}

void Character::ComputeNextState(
  Colbox &mask_col_ext,
  Colbox &mask_col_ver_int,
  Colbox &mask_col_hor_int,
  Keyboard& keyboard) {

}