#include "character.h" // class's header file
#include "camera.h"

// class constructor
Character::Character() {
  pos_x = 264;  // REVISIT: should be 0
  pos_y = 2000; // REVISIT: should be 0

  using_bb = true;
  height = 21;  // REVISIT: should be 0
  width  = 23;  // REVISIT: should be 0
  height_orig = height;
  width_orig = width;

  // REVISIT: think on how to pass this information automatically
  bb_x = 5;
  bb_y = 0;
  bb_width = 13;
  bb_width_orig = bb_width;
  bb_height = 21;
  bb_height_orig = bb_height;

  height_internal = 3;
  width_internal = 3;

  state = RICK_STATE_STOP;
  direction = RICK_DIR_STOP;

  speed_x = HOR_SPEED_MAX;
  speed_y = VERT_SPEED_MAX;

  stepsInState = 0;
  stepsInDirectionX = 0;
  stepsInDirectionY = 0;

  face = RICK_DIR_RIGHT;

  inPlatform = false;

  initial_x = pos_x;
  initial_y = pos_y;
  initial_direction = direction;
  initial_speed_x = speed_x;
  initial_speed_y = speed_y;
  initial_state = state;

  animation_scaling_factor = 1.0;

  killed = false;

  stop_move_block_col = false;
}

Character::Character(const char* file) {
  // REVISIT: most of this information should be read from the file
  pos_x = 968;  // REVISIT: should be 0
  pos_y = 992; // REVISIT: should be 0
  height = 21;  // REVISIT: should be 0
  width  = 23;  // REVISIT: should be 0
  // REVISIT: think on how to pass this information automatically
  using_bb = true;
  bb_x = 5;
  bb_y = 0;
  bb_width = 13;
  bb_width_orig = bb_width;
  bb_height = 21;
  bb_height_orig = bb_height;
  // END REVISIT
  height_orig = height;
  width_orig = width;
  height_internal = 3;
  width_internal = 3;
  state = RICK_STATE_STOP;
  direction = RICK_DIR_STOP;
  speed_x = HOR_SPEED_MAX;
  speed_y = VERT_SPEED_MAX;
  stepsInState = 0;
  stepsInDirectionX = 0;
  stepsInDirectionY = 0;
  face = RICK_DIR_RIGHT;
  inPlatform = false;
  initial_x = pos_x;
  initial_y = pos_y;
  initial_direction = direction;
  initial_speed_x = speed_x;
  initial_speed_y = speed_y;
  initial_state = state;
  animation_scaling_factor = 1.0;
  killed = false;
  stop_move_block_col = false;

  pugi::xml_parse_result result = character_file.load_file(file);
  if(!result) {
      printf("Error: loading character data from file = %s, description = %s\n", file, result.description());
  }

  // REVISIT: states are taking in order from the file. It would be better to find
  // a way to insert them by id instead. However, giving an id to the xml
  // state also requires synchronizing with the state id in the code.

  printf("- Initializing player:\n");
  // Iterate over states
  int num_anims = 0;
  for (pugi::xml_node state = character_file.child("character").child("states").first_child();
       state; state = state.next_sibling()) {
    printf("State name = %s, id = %d\n", state.attribute("name").as_string(), state.attribute("id").as_int());
    // Create state
    pugi::xml_node animation = state.child("animation");
    // Create animation and attach to state
    printf("\tAnimation %d: file = %s, speed = %d\n", num_anims, animation.attribute("bitmap").as_string(), animation.attribute("speed").as_int());
    ALLEGRO_BITMAP* anim_bitmap = al_load_bitmap(animation.attribute("bitmap").as_string());
    if (!anim_bitmap) {
      printf("Error: failed to load animation bitmap\n");
    }
    Animation* player_anim = new Animation(anim_bitmap, animation.attribute("speed").as_int());
    int num_sprites = 0;
    // Traverse all sprites in the animation
    for (pugi::xml_node sprite = animation.first_child(); sprite; sprite = sprite.next_sibling()) {
      int sprite_x      = sprite.attribute("x").as_int();
      int sprite_y      = sprite.attribute("y").as_int();
      int sprite_width  = sprite.attribute("width").as_int();
      int sprite_height = sprite.attribute("height").as_int();

      printf("\t\tSprite %d: x = %d, y = %d, width = %d, height = %d\n", num_sprites,
                                                                         sprite_x,
                                                                         sprite_y,
                                                                         sprite_width,
                                                                         sprite_height);

      ALLEGRO_BITMAP* sprite_bitmap = al_create_sub_bitmap(anim_bitmap, sprite_x, sprite_y, sprite_width, sprite_height);

      player_anim->AddSprite(sprite_bitmap,
                             sprite_x,
                             sprite_y,
                             sprite_width,
                             sprite_height);      
    }
    animations.push_back(player_anim);
  }
}

// class destructor
Character::~Character() {  
  for(vector<Animation*>::iterator it = animations.begin(); it != animations.end(); ++it) {
    delete (*it);
  }
}

void Character::Reset() {
  pos_x = initial_x;
  pos_y = initial_y;
  direction = initial_direction;
  speed_x = initial_speed_x;
  speed_y = initial_speed_y;
  state = initial_state;
  face = RICK_DIR_RIGHT;
}

int Character::GetCorrectedPosX() {
  if (width == width_orig)
    return pos_x;
  else if (width > width_orig)
    return pos_x + (width - width_orig);
  else
    return pos_x - (width_orig - width);
}

int Character::GetCorrectedPosY() {
  if (height == height_orig)
    return pos_y;
  else if (height > height_orig)
    return pos_y + (height - height_orig);
  else
    return pos_y - (height_orig - height);
}

void Character::SetPosX(World* map, int x) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.
  int tile_width = map->GetTilesetTileWidth();
  int tile_height = map->GetTilesetTileHeight();
  int character_width = (using_bb ? this->bb_width : this->width) - 1;
  int character_height = (using_bb ? this->bb_height : this->height) - 1;

  // Compute the displacement in x
  int desp_x = (x > pos_x) ? (x - pos_x) : (pos_x - x);

  // Compute x and y corrections to draw world
  int tile_col_x;
  int tile_col_left_y  = (pos_y + bb_y) / tile_height;
  int tile_col_right_y = (pos_y + bb_y + character_height) / tile_height;
  if ( x > pos_x) {
    // Collision moving right
    tile_col_x = (pos_x + bb_x + desp_x + character_width)  / tile_width;
    if ((!map->IsTileCollisionable(tile_col_x, tile_col_left_y)) &&
        (!map->IsTileCollisionable(tile_col_x, tile_col_right_y))) {
      // No collision
      pos_x = pos_x + desp_x;
    } else {
      // Collision. Move to safe position
      int correction = (pos_x + bb_x + desp_x + character_width) % tile_width;
      pos_x = pos_x + desp_x - correction - 1;
    }
  } else if ((x > 0) && (x < pos_x)) {
    // Collision moving left    
    tile_col_x = (pos_x + bb_x - desp_x) / tile_width;
    if ((!map->IsTileCollisionable(tile_col_x, tile_col_left_y)) &&
        (!map->IsTileCollisionable(tile_col_x, tile_col_right_y))) {
      // No collision
      pos_x = pos_x - desp_x;
    } else {
      // Collision. Move to safe position
      int correction = tile_width - ((pos_x + bb_x - desp_x) % tile_width);
      pos_x = pos_x - desp_x + correction;
    }
  }
}

void Character::SetPosY(World* map, int y, bool all) {
  // REVISIT: character is currently a single box, this function requires to be updated
  // when the character will be based on a generic player.
  int tile_width       = map->GetTilesetTileWidth();
  int tile_height      = map->GetTilesetTileHeight();
  int character_width  = (using_bb ? this->bb_width : this->width) - 1;
  int character_height = (using_bb ? this->bb_height : this->height) - 1;

  // Compute the displacement in x
  int desp_y = (y > pos_y) ? (y - pos_y) : (pos_y - y);

  // Compute x and y corrections to draw world
  int tile_col_y;
  int tile_col_up_x   = (pos_x + bb_x) / tile_width;
  int tile_col_down_x = (pos_x + bb_x +character_width) / tile_width;
  if (y > pos_y) {
    // Collision moving down
    tile_col_y = (pos_y + bb_y + desp_y + character_height)  / tile_height;
    // REVISIT: improve coding for "all"
    if (((!all && (!map->IsTileCollisionable(tile_col_up_x, tile_col_y))) ||
         ( all && (!map->IsTileCollisionableDown(tile_col_up_x, tile_col_y)))) &&
        ((!all && (!map->IsTileCollisionable(tile_col_down_x, tile_col_y))) ||
         ( all && !(map->IsTileCollisionableDown(tile_col_down_x, tile_col_y))))) {
      // No collision
      pos_y = pos_y + desp_y;
    } else {      
      // Collision. Move to safe position
      int correction = (pos_y + bb_y + desp_y + character_height) % tile_height;
      pos_y = pos_y + desp_y - correction - 1;
    }
  } else if ((y > 0) && (y < pos_y)) {
    // Collision moving up    
    tile_col_y = (pos_y + bb_y - desp_y) / tile_height;
    // REVISIT: improve coding for "all"
    if (((!all && (!map->IsTileCollisionable(tile_col_up_x, tile_col_y))) ||
         ( all && (!map->IsTileCollisionableDown(tile_col_up_x, tile_col_y)))) &&
        ((!all && (!map->IsTileCollisionable(tile_col_down_x, tile_col_y))) ||
         ( all && (!map->IsTileCollisionableDown(tile_col_down_x, tile_col_y))))) {
      // No collision
      pos_y = pos_y - desp_y;
    } else {
      // Collision. Move to safe position
      int correction = tile_height - ((pos_y + bb_y - desp_y) % tile_height);
      pos_y = pos_y - desp_y + correction;
    }
  }
}

bool Character::ComputeCollisionBlocks(World* map) {
  blockCollisionRight = false;
  blockCollisionLeft = false;
  blockCollisionPtr = 0;

  list<Block*> *blocks = map->GetBlocks();
  for (list<Block*>::iterator it = blocks->begin() ; it != blocks->end(); ++it) {
    Block* block = *it;

    if (block->CoordsWithinObject(pos_x + bb_x + 1 + bb_width, pos_y + bb_y) ||        
        block->CoordsWithinObject(pos_x + bb_x + 1 + bb_width, pos_y + bb_y + bb_height - 1)) {
      blockCollisionRight = true;
      blockCollisionPtr = block;
      // Take first block with collision
      break;
    } else if (block->CoordsWithinObject(pos_x + bb_x, pos_y + bb_y) ||
               block->CoordsWithinObject(pos_x + bb_x, pos_y + bb_y + bb_height - 1)) {
      blockCollisionLeft = true;
      blockCollisionPtr = block;
      // Take first block with collision
      break;
    }
  } 
}

void Character::GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int right_down_x, int right_down_y) {
  mask_col.SetLeftUpCol(map->GetTileByCoord(left_up_x, left_up_y)->GetType());
  mask_col.SetRightUpCol(map->GetTileByCoord(right_down_x, left_up_y)->GetType());
  mask_col.SetRightDownCol(map->GetTileByCoord(right_down_x, right_down_y)->GetType());
  mask_col.SetLeftDownCol(map->GetTileByCoord(left_up_x, right_down_y)->GetType());
}

void Character::GetCollisionsExternalBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x - 1,
                              pos_y - 1,
                              pos_x + width,
                              pos_y + height);
}

void Character::GetCollisionsExternalHeightBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y - 1,
                              pos_x + width - 1,
                              pos_y + height);
}

// REVISIT: use Bounding Box instead
void Character::GetCollisionsInternalHeightBoxExt(World* map, Colbox &mask_col) {  
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + bb_x,
                              pos_y + bb_y - 1,
                              pos_x + bb_x + bb_width - 1,
                              pos_y + bb_y + bb_height);
}

void Character::GetCollisionsInternalHeightBoxExtOrig(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              this->GetCorrectedPosX() + bb_x,
                              this->GetCorrectedPosY() + bb_y - 1,
                              this->GetCorrectedPosX() + bb_x + bb_width_orig - 1,
                              this->GetCorrectedPosY() + bb_y + bb_height_orig);
}

void Character::GetCollisionsInternalHeightBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + bb_x,
                              pos_y + bb_y,
                              pos_x + bb_x + bb_width - 1,
                              pos_y + bb_y + bb_height - 1);
}

void Character::ComputeCollisions(World* map) {
  int down_left_x;
  int down_right_x;
  int down_y;

//  printf("[ComputeCollisions] state = %d\n", state);

  // Do not check collisions when DYING
  if ((state == RICK_STATE_DYING) || (state == RICK_STATE_DEAD)) return;
  
  this->GetCollisionsExternalBoxExt(map, extColExt);
  this->GetCollisionsExternalHeightBoxExt(map, extHeightColExt);
  this->GetCollisionsInternalHeightBoxExtOrig(map, heightColExtOrig);
  this->GetCollisionsInternalHeightBoxInt(map, heightColInt);  
  this->GetCollisionsInternalHeightBoxExt(map, heightColExt);  

  // First check if there is collision with an object over the tiles
  //printf("[ComputeCollisions] Checking collisions with platforms\n");
  vector<Platform*> *platforms = map->GetPlatforms();
  for (vector<Platform*>::iterator it = platforms->begin() ; it != platforms->end(); ++it) {
    down_left_x = pos_x;
    down_right_x = pos_x + width;
    down_y = pos_y + height + 1;

    Platform* platform = *it;
    if ((down_y >= platform->GetY()) && (down_y <= (platform->GetY() + platform->GetHeight())) &&
      (((down_left_x >= platform->GetX()) && (down_left_x <= (platform->GetX() + platform->GetWidth()))) ||
       ((down_right_x >= platform->GetX()) && (down_right_x <= (platform->GetX() + platform->GetWidth()))))) {
      inPlatform = true;
      inPlatformPtr = platform;
      // Take first platform with collision
      break;
    } else {
      inPlatform = false;
      inPlatformPtr = 0;
    }
  } 

  //printf("[ComputeCollisions] Getting simple collisions checks\n");
  // Check if there is a collision with the tiles
  inStairs = ((heightColInt.GetLeftUpCol() == TILE_STAIRS) ||
              (heightColInt.GetRightUpCol() == TILE_STAIRS) ||
              (heightColInt.GetLeftUpCol() == TILE_STAIRS) ||
              (heightColInt.GetRightUpCol() == TILE_STAIRS) ||
              (heightColInt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
              (heightColInt.GetRightDownCol() == TILE_STAIRS_TOP));

  overStairs = (heightColExt.GetLeftDownCol() == TILE_STAIRS_TOP) &&
               (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP);

  inFloor = inPlatform ||
            (heightColExt.GetLeftDownCol() == TILE_COL) ||
            (heightColExt.GetRightDownCol() == TILE_COL);

  inAir = !inPlatform &&
          ((heightColExt.GetLeftDownCol() == 0) ||             // No tile is air
           (heightColExt.GetLeftDownCol() == TILE_STAIRS)) &&  // Stairs is also air
          ((heightColExt.GetRightDownCol() == 0) ||
           (heightColExt.GetRightDownCol() == TILE_STAIRS));

  inAirInt = ((heightColInt.GetLeftDownCol() == 0) &&
              (heightColInt.GetRightDownCol() == 0));

  // - Collision with head on collisionable tile
  collisionHead = (heightColExt.GetLeftUpCol() == TILE_COL) ||
                  (heightColExt.GetRightUpCol() == TILE_COL);

  collisionHeadOrig = (heightColExtOrig.GetLeftUpCol() == TILE_COL) ||
                      (heightColExtOrig.GetRightUpCol() == TILE_COL);

  overStairsRight = (extColExt.GetLeftDownCol() == TILE_COL) &&
                    (heightColExt.GetLeftDownCol() == TILE_COL) &&
                    (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP);                    

  overStairsLeft = (extColExt.GetRightDownCol() == TILE_COL) &&
                   (heightColExt.GetRightDownCol() == TILE_COL) &&
                   (heightColExt.GetLeftDownCol() == TILE_STAIRS_TOP);
}

// Helper function to reset direction based on keyboard
void Character::FixHorizontalDirection(Keyboard& keyboard) {
  if (keyboard.PressedRight()) {
    direction |= RICK_DIR_RIGHT;
    direction &= ~RICK_DIR_LEFT;
  } else if (keyboard.PressedLeft()) {
    direction |= RICK_DIR_LEFT;
    direction &= ~RICK_DIR_RIGHT;
  } else {
    direction &= ~RICK_DIR_LEFT;
    direction &= ~RICK_DIR_RIGHT;
  }
}

void Character::ComputeNextState(World* map, Keyboard& keyboard) {  
  int prevDirection;
  
  // Save current state before computing next state
  prevState = state;
  // Save current direction before computing next state and direction
  prevDirection = direction;

  //printf("Pre: State = %d, direction = %d, face = %d\n", state, direction, face);

  // No matter what is the state update, if rick is being killed, then
  // the kill takes precedence
  if (killed) {
    killed = false;
    state = RICK_STATE_DYING;
    direction = RICK_DIR_UP;
    direction |= RICK_DIR_RIGHT;
    // Save pos y
    pos_y_chk = pos_y;
  } else {
    switch(state) {
      case RICK_STATE_STOP:
      case RICK_STATE_RUNNING:

        if (inAir) {
          state = RICK_STATE_JUMPING;
          direction = RICK_DIR_DOWN;
        } else if (keyboard.PressedSpace()) {
          if (keyboard.PressedUp()) {
            state = RICK_STATE_SHOOTING;
          } else if (keyboard.PressedDown()) {            
            state = RICK_STATE_BOMBING;
          } else if (keyboard.PressedLeft()) {
            state = RICK_STATE_HITTING;
            direction = RICK_DIR_LEFT;
          } else if (keyboard.PressedRight()) {
            state = RICK_STATE_HITTING;
            direction = RICK_DIR_RIGHT;
          }
        } else if (keyboard.PressedUp()) {
          if (inStairs) {
            // In stairs
            state = RICK_STATE_CLIMBING;
            direction = RICK_DIR_UP;
          } else {
            if (!inAir) {
              // Start jump
              state = RICK_STATE_JUMPING;
              direction = RICK_DIR_UP;
              // Save pos y
              pos_y_chk = pos_y;
            } else {
              state = RICK_STATE_JUMPING;
              direction = RICK_DIR_DOWN;
            }
          }
        } else if (keyboard.PressedDown()) {
            if (overStairs) {
              state = RICK_STATE_CLIMBING;
              direction = RICK_DIR_DOWN;
            } else if (overStairsRight && (face == RICK_DIR_RIGHT)) {
              state = RICK_STATE_RUNNING;
              direction = RICK_DIR_RIGHT;
            } else if (overStairsLeft && (face == RICK_DIR_LEFT)) {              
              state = RICK_STATE_RUNNING;
              direction = RICK_DIR_LEFT;
            } else {
              height = 15;                             // REVISIT: Hard-coded. Need to be obtained from state (now it is in animation)              
              bb_height = height;                      // REVISIT: think on how to adapt this
              pos_y = pos_y + (height_orig - height);
              state = RICK_STATE_CROUCHING;
              direction = RICK_DIR_STOP;
            }
        } else if (keyboard.PressedRight()) {
          state = RICK_STATE_RUNNING;
          direction = RICK_DIR_RIGHT;
        } else if (keyboard.PressedLeft()) {
          state = RICK_STATE_RUNNING;
          direction = RICK_DIR_LEFT;
        } else {
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
        }
  
        break;
  
      case RICK_STATE_JUMPING:
        if ((direction & RICK_DIR_DOWN)) {
          if (!inAir) {
            // Factor keyboard to keep running without transitioning through stop
            if (keyboard.PressedRight() || keyboard.PressedLeft()) {
              state = RICK_STATE_RUNNING;
            } else {
              state = RICK_STATE_STOP;
              direction = RICK_DIR_STOP;
            }
          }
        } else if (direction & RICK_DIR_UP) {
          if (collisionHead ||
              (abs(pos_y_chk - pos_y) >= 5*8)) {                   // REVISIT: hard coded the maximum distance for jumping
            direction = RICK_DIR_DOWN;
          }
        }
  
        // Take stairs if pressing up when jumping
        if (keyboard.PressedUp() &&
            inStairs) {
          state = RICK_STATE_CLIMBING;
          direction = RICK_DIR_UP;
        }
  
        // Fix horizontal direction based on keyboard input
        this->FixHorizontalDirection(keyboard);
        break;
  
      case RICK_STATE_CROUCHING:
        if ((!collisionHeadOrig && !keyboard.PressedDown()) || inAir) {
          pos_y = pos_y - (height_orig - height);
          height = 21;                            // REVISIT: Hard-coded. Need to be obtained from state (now it is in animation)
          bb_height = height;                     // REVISIT: think on how to adapt this
          state = RICK_STATE_STOP;
        }
  
        // Fix horizontal direction based on keyboard input
        this->FixHorizontalDirection(keyboard);
        break;
  
      case RICK_STATE_CLIMBING:
  
        if (inAirInt && !overStairs & !inStairs) {
          // height rectangle is out of the stairs. Make Rick to fall
          state = RICK_STATE_JUMPING;
          direction = RICK_DIR_DOWN;
        } else if (keyboard.PressedUp()) {
          if (inAirInt && overStairs) {
          // Just arrived at the top of the stairs
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
          } else {
            // If on stairs, and pressed key up, then keep climbing up
            direction = RICK_DIR_UP;
          }
        } else if (keyboard.PressedDown()) {
          if (inFloor) {
          // No in stairs, stop player because it was previously in stairs
            state = RICK_STATE_STOP;
            direction = RICK_DIR_STOP;
          } else {
            direction = RICK_DIR_DOWN;
          }
        } else {
          // No up and down keyboard pressed
          direction = RICK_DIR_STOP;
        }
  
        // Fix horizontal direction based on keyboard input
        this->FixHorizontalDirection(keyboard);
        break;

      case RICK_STATE_SHOOTING:
        if (face == RICK_DIR_RIGHT) {
          map->CreateNewShoot(pos_x + 23, pos_y + 8, OBJ_DIR_RIGHT);
        } else {
          map->CreateNewShoot(pos_x - 10, pos_y + 8, OBJ_DIR_LEFT);
        }
        if (!(keyboard.PressedSpace() && keyboard.PressedUp())) {
          state = RICK_STATE_STOP;
        }
        break;

      case RICK_STATE_BOMBING:
        if (keyboard.PressedLeft()) {              
          direction = RICK_DIR_LEFT;
          map->CreateNewBomb(pos_x, pos_y, OBJ_DIR_LEFT);
        } else if (keyboard.PressedRight()) {
          direction = RICK_DIR_RIGHT;
          map->CreateNewBomb(pos_x, pos_y, OBJ_DIR_RIGHT);
        } else {
          direction = RICK_DIR_STOP;
          map->CreateNewBomb(pos_x, pos_y, OBJ_DIR_STOP);
        }

        if (!(keyboard.PressedSpace() && keyboard.PressedDown())) {
          state = RICK_STATE_STOP;
        }
        break;

      case RICK_STATE_HITTING:
        if (keyboard.PressedSpace() && keyboard.PressedLeft()) {
          state = RICK_STATE_HITTING;
          direction = RICK_DIR_LEFT;
        } else if (keyboard.PressedSpace() && keyboard.PressedRight()) {
          state = RICK_STATE_HITTING;
          direction = RICK_DIR_RIGHT;
        } else {
          state = RICK_STATE_STOP;
        }
        break;

      case RICK_STATE_DYING:
        if (direction & RICK_DIR_UP) {
          if (abs(pos_y_chk - pos_y) >= 6*8) {                   // REVISIT: hard coded the maximum distance for jumping
            direction &= ~RICK_DIR_UP;
            direction |=  RICK_DIR_DOWN;
          }
        } else if (direction & RICK_DIR_DOWN) {
          if (pos_y >= (camera->GetPosY() + CAMERA_Y)) {
            state = RICK_STATE_DEAD;
          }
        }
        break;

      case RICK_STATE_DEAD:
        this->Reset();
        break;
      default:
        break;
    }
  }

  // Increment steps in state if no change in state
  if (prevState == state)
    stepsInState++;
  else
    stepsInState = 0;

  if ((prevDirection == direction)) { 
    if ((direction == RICK_DIR_LEFT) || (direction == RICK_DIR_RIGHT)) {      
      stepsInDirectionX++;
    } else {
      stepsInDirectionY++;
    }
  } else {
    stepsInDirectionX = 0;
    stepsInDirectionY = 0;
  }

  // keep last direction in face
  if (direction & RICK_DIR_LEFT) {
    face = RICK_DIR_LEFT;
  } else if (direction & RICK_DIR_RIGHT) {
    face = RICK_DIR_RIGHT;
  }

  //printf("Post: State = %d, direction = %d, face = %d\n", state, direction, face);
  //printf("Steps in state = %d, steps in direction x = %d, steps in direction y = %d\n", stepsInState, stepsInDirectionX, stepsInDirectionY);
}

void Character::ComputeNextPosition(World* map) {
  int direction_old;
  //printf("PRE: pos_x = %d pos_y %d\n", pos_x, pos_y);
  //printf("pos_x = %d, pos_y = %d, speed_x = %f, speed_y = %f\n", pos_x, pos_y, speed_x, speed_y);

  // First check if on platform
  // REVISIT: platform should be revisited once they will be properly implemented
  if (inPlatform) {
    // Correct y to be on top of platform
    SetPosY(map, GetPosY() - (GetPosY() + GetHeight() - inPlatformPtr->GetY()), false);
    if (inPlatformPtr->GetDirection() == OBJ_DIR_RIGHT) {
      SetPosX(map, GetPosX() + inPlatformPtr->GetSpeed());
    } else if (inPlatformPtr->GetDirection() == OBJ_DIR_LEFT) {
      SetPosX(map, GetPosX() - inPlatformPtr->GetSpeed());
    }
  }

  // If player is collisioning with a block, then
  // we need to avoid the player to move.  
  if (stop_move_block_col) {
    direction_old = direction;
    direction &= ~RICK_DIR_RIGHT;
    direction &= ~RICK_DIR_LEFT;
  }

  switch(state) {
    case RICK_STATE_STOP:
      break;
    case RICK_STATE_RUNNING:
    case RICK_STATE_CROUCHING:
      if (direction & RICK_DIR_RIGHT) {
        SetPosX(map, GetPosX() + speed_x);
      } else if (direction & RICK_DIR_LEFT) {
        SetPosX(map, GetPosX() - speed_x);
      }
      break;
    case RICK_STATE_JUMPING:

      // Correct first the horizontal movement because
      // it may affect where the vertical movement is placed
      // at the end.
      if (direction & RICK_DIR_RIGHT)
        SetPosX(map, GetPosX() + speed_x);
      else if (direction & RICK_DIR_LEFT)
        SetPosX(map, GetPosX() - speed_x);

      if (direction & RICK_DIR_UP)
        SetPosY(map, GetPosY() - speed_y, false);
      else if (direction & RICK_DIR_DOWN)
        SetPosY(map, GetPosY() + speed_y, true);

      break;
    case RICK_STATE_CLIMBING:
      if (direction & RICK_DIR_UP) {
        // First, correct x to facilitate moving up
        if ((extColExt.GetLeftUpCol() == TILE_COL) &&
            (heightColExt.GetLeftUpCol() == TILE_COL) &&
            (heightColExt.GetRightUpCol() == TILE_STAIRS)) {
          SetPosX(map, GetPosX() + HOR_SPEED_MAX);
        } else if ((extColExt.GetRightUpCol() == TILE_COL) &&
                   (heightColExt.GetRightUpCol() == TILE_COL) &&
                   (heightColExt.GetLeftUpCol() == TILE_STAIRS)) {
          SetPosX(map, GetPosX() - HOR_SPEED_MAX);
        }

        SetPosY(map, GetPosY() - speed_y, false);
      } else if (direction & RICK_DIR_DOWN) {
        SetPosY(map, GetPosY() + speed_y, false);
      }
      if (direction & RICK_DIR_RIGHT) {
        SetPosX(map, GetPosX() + speed_x);
      } else if (direction & RICK_DIR_LEFT) {
        SetPosX(map, GetPosX() - speed_x);
      }
      break;
    case RICK_STATE_DYING:
      if (direction & RICK_DIR_UP)
        pos_y -= speed_y;
      else if (direction & RICK_DIR_DOWN)
        pos_y += speed_y;

      if (direction & RICK_DIR_RIGHT)
        pos_x += speed_x;
      else if (direction & RICK_DIR_LEFT)
        pos_x -= speed_x;
    default:
      break;
  }

  if (stop_move_block_col) {
    direction = direction_old;
  }
  //printf("POST: pos_x = %d pos_y %d\n", pos_x, pos_y);
}

void Character::ComputeNextPositionBasedOnBlocks(World* map, Keyboard& keyboard) {
  // If no block coliision then return
  bool blockCollision = blockCollisionLeft || blockCollisionRight;

  stop_move_block_col = false;

  if (!blockCollision) return;

  if (blockCollisionRight && (direction & RICK_DIR_RIGHT)) {
    SetPosX(map, GetPosX() - (pos_x + bb_x + bb_width - blockCollisionPtr->GetX()));
    // Stop movement in this direction
    stop_move_block_col = true;
  } else if (blockCollisionLeft && (direction & RICK_DIR_LEFT)) {
    SetPosX(map, GetPosX() + (blockCollisionPtr->GetX() + blockCollisionPtr->GetWidth() - (pos_x + bb_x)));
    // Stop movement in this direction
    stop_move_block_col = true;
  }
}

void Character::ComputeNextSpeed() {
  
  switch (state) {
    case RICK_STATE_STOP:
      speed_x = 0.0;
      speed_y = 0.0;
      break;
    case RICK_STATE_RUNNING:
      speed_x = HOR_SPEED_MAX;
      break;
    case RICK_STATE_JUMPING:
        if (!stepsInState) {
          if (direction & RICK_DIR_UP)
            speed_y = VERT_SPEED_MAX;
          else
            speed_y = VERT_SPEED_MIN;
        } else if ((direction & RICK_DIR_UP) && (stepsInDirectionY > 0)) {
          if (speed_y > VERT_SPEED_MIN)
            speed_y = speed_y - VERT_SPEED_STEP;
          else
            speed_y = VERT_SPEED_MIN;
        } else if ((direction & RICK_DIR_DOWN) && (stepsInDirectionY > 0)) {
          if (speed_y < VERT_SPEED_MAX)
            speed_y = speed_y + VERT_SPEED_STEP;
          else
            speed_y = VERT_SPEED_MAX;
        }

        speed_x = HOR_SPEED_MAX;
      break;

    case RICK_STATE_CLIMBING:
      speed_x = HOR_SPEED_MAX;
      speed_y = HOR_SPEED_MAX;  // Same as horizontal speed
      break;
      case RICK_STATE_DYING:
      if (!stepsInState) {
        if (direction & RICK_DIR_UP)
          speed_y = 2*VERT_SPEED_MAX;
        else
          speed_y = 2*VERT_SPEED_MIN;
      } else if ((direction & RICK_DIR_UP) && (stepsInDirectionY > 0)) {
        if (speed_y > 2*VERT_SPEED_MIN)
          speed_y = speed_y - 2*VERT_SPEED_STEP;
        else
          speed_y = VERT_SPEED_MIN;
      } else if ((direction & RICK_DIR_DOWN) && (stepsInDirectionY > 0)) {
        if (speed_y < 2*VERT_SPEED_MAX)
          speed_y = speed_y + 2*VERT_SPEED_STEP;
        else
          speed_y = 2*VERT_SPEED_MAX;
      }

      speed_x = HOR_SPEED_MAX;
      break;
    default:      
      speed_x = HOR_SPEED_MAX;
      speed_y = VERT_SPEED_MAX;
      break;
  }

  //printf("speed_y = %f\n", speed_y);
}

void Character::CharacterStep(World* map, Keyboard& keyboard) {
  // Collisions with world and platforms
//  printf("[CharacterStep] ComputeCollisions\n");
  this->ComputeCollisions(map);
  this->ComputeCollisionBlocks(map);
  // Compute next state
//  printf("[CharacterStep] ComputeNextState\n");
  this->ComputeNextState(map, keyboard);
  // Compute next position 
//  printf("[CharacterStep] ComputeNextPosition\n");
  this->ComputeNextPositionBasedOnBlocks(map, keyboard);
  this->ComputeNextPosition(map);  
  // Re-calulate speed
//  printf("[CharacterStep] ComputeNextSpeed\n");
  this->ComputeNextSpeed();
  // Compute next animation frame
//  printf("[CharacterStep] ComputeNextAnimation\n");
  if (direction == RICK_DIR_STOP)
    animations[state]->ResetAnim();
  else if (state != RICK_STATE_DEAD)
    animations[state]->AnimStep();

  // Animation scaling factor is only used when dying
  if (state == RICK_STATE_DYING)
    animation_scaling_factor += 0.1;
  else if (state == RICK_STATE_DEAD)
    animation_scaling_factor = 1.0;
  
//  printf("[CharacterStep] End CharacterStep\n");
}

ALLEGRO_BITMAP* Character::GetCurrentAnimationBitmap() {
  ALLEGRO_BITMAP* bitmap = animations[state]->source_bitmap;
  // Set transparent color
  al_convert_mask_to_alpha(bitmap, al_map_rgb(255,0,255));
  sprite_ptr sprite = &(*animations[state]->sprites[animations[state]->GetCurrentAnim()]);
  return al_create_sub_bitmap(bitmap,
                              sprite->x,
                              sprite->y,
                              sprite->width,
                              sprite->height);
}

int Character::GetCurrentAnimationBitmapAttributes() {
  if (face == RICK_DIR_LEFT) {
    return ALLEGRO_FLIP_HORIZONTAL;
  }
  return 0;
}

int Character::GetCurrentAnimationWidth() {
  return animations[state]->sprites[animations[state]->GetCurrentAnim()]->width;
}

int Character::GetCurrentAnimationHeight() {
  return animations[state]->sprites[animations[state]->GetCurrentAnim()]->height;
}

float Character::GetCurrentAnimationScalingFactor() {
  return animation_scaling_factor;
}
