#include "character.h" // class's header file

// class constructor
Character::Character() {
  pos_x = 264;  // REVISIT: should be 0
  pos_y = 2000; // REVISIT: should be 0

  height = 16;  // REVISIT: should be 0
  width = 16;  // REVISIT: should be 0

  height_internal = 3;
  width_internal = 3;

  state = RICK_STATE_STOP;
  direction = RICK_DIR_STOP;

  speed_x = 2.0;
  speed_y = 2.0;

  stepsInState = 0;
  stepsInDirectionX = 0;
  stepsInDirectionY = 0;
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

void Character::SetPosY(World* map, int y, bool all) {
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
    // REVISIT: improve coding for "all"
    if (((!all & (map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL)) ||
         (all && !(map->GetTile(tile_col_up_x, tile_col_y)->IsCollisionable()))) &&
        ((!all && (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) ||
         (all && !(map->GetTile(tile_col_down_x, tile_col_y)->IsCollisionable())))) {
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
    // REVISIT: improve coding for "all"
    if (((!all && (map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL)) ||
         (all && !(map->GetTile(tile_col_up_x, tile_col_y)->IsCollisionable()))) &&
        ((!all && (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) ||
         (all && !(map->GetTile(tile_col_down_x, tile_col_y)->IsCollisionable())))) {
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
                              width + 1,
                              height + 1);
}

void Character::GetCollisionsExternalWidthBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x - 1,
                              pos_y,
                              width + 1,
                              height - 1);
}

void Character::GetCollisionsExternalHeightBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y - 1,
                              width - 1,
                              height + 1);
}

void Character::GetCollisionsExternalBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y,
                              width - 1,
                              height - 1);
}

void Character::GetCollisionsInternalWidthBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x - 1,
                              pos_y + height_internal,
                              width + 2 - 1,
                              height - 2*height_internal - 1);
}

void Character::GetCollisionsInternalWidthBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x,
                              pos_y + height_internal,
                              width - 1,
                              height - 2*height_internal - 1);
}

void Character::GetCollisionsInternalHeightBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + width_internal,
                              pos_y - 1,
                              width - 2*width_internal - 1,
                              height + 1);
}

void Character::GetCollisionsInternalHeightBoxInt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              pos_x + width_internal,
                              pos_y,
                              width - 2*width_internal - 1,
                              height - 1);
}

void Character::ComputeCollisions(World* map, vector<Platform*> *platforms) {
  int down_left_x;
  int down_right_x;
  int down_y;

  this->GetCollisionsExternalBoxInt(map, extColInt);
  this->GetCollisionsExternalBoxExt(map, extColExt);
  this->GetCollisionsExternalWidthBoxExt(map, extWidthColExt);
  this->GetCollisionsExternalHeightBoxExt(map, extHeightColExt);
  this->GetCollisionsInternalWidthBoxInt(map, widthColInt);
  this->GetCollisionsInternalWidthBoxExt(map, widthColExt);
  this->GetCollisionsInternalHeightBoxInt(map, heightColInt);
  this->GetCollisionsInternalHeightBoxExt(map, heightColExt);

/*    printf("[Collisions ext] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      extColExt.GetLeftUpCol(),
      extColExt.GetRightUpCol(),
      extColExt.GetRightDownCol(),
      extColExt.GetLeftDownCol());
    printf("[Collisions int width] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      widthColInt.GetLeftUpCol(),
      widthColInt.GetRightUpCol(),
      widthColInt.GetRightDownCol(),
      widthColInt.GetLeftDownCol());
    printf("[Collisions int height] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      heightColInt.GetLeftUpCol(),
      heightColInt.GetRightUpCol(),
      heightColInt.GetRightDownCol(),
      heightColInt.GetLeftDownCol());*/

  // First check if there is collision with an object over the tiles
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

  // Check if there is a collision with the tiles
  inStairs = ((heightColInt.GetLeftDownCol() == TILE_STAIRS) ||
              (heightColInt.GetRightDownCol() == TILE_STAIRS) ||
              (heightColInt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
              (heightColInt.GetRightDownCol() == TILE_STAIRS_TOP));

  inFloor = inPlatform ||
            (extHeightColExt.GetLeftDownCol() == TILE_COL) ||
            (extHeightColExt.GetRightDownCol() == TILE_COL);

  inTopStairs = (extHeightColExt.GetLeftDownCol() == TILE_STAIRS_TOP) &&
                (extHeightColExt.GetRightDownCol() == TILE_STAIRS_TOP);

  inAir = (extHeightColExt.GetLeftDownCol() == 0) &&
          (extHeightColExt.GetRightDownCol() == 0);

  inAirInt = ((heightColInt.GetLeftDownCol() == 0) &&
               (heightColInt.GetRightDownCol() == 0));
}

void Character::ComputeNextState(Keyboard& keyboard) {  
  int prevDirection;
  
  // Save current state before computing next state
  prevState = state;
  // Save current direction before computing next state and direction
  prevDirection = direction;

  //printf("Pre: State = %d, direction = %d\n", state, direction);

  switch(state) {
    case RICK_STATE_STOP:
      direction = RICK_DIR_STOP;

      if (keyboard.PressedUp()) {
        if ((heightColInt.GetLeftUpCol() == TILE_STAIRS) || (heightColInt.GetRightUpCol() == TILE_STAIRS)) {
          // In stairs
          state = RICK_STATE_CLIMBING;
          direction = RICK_DIR_UP;
        } else {
          if (inFloor || inTopStairs) {
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
      } else if (keyboard.PressedDown() &&
                 ((heightColExt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
                  (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP))) {
          state = RICK_STATE_CLIMBING;
          direction = RICK_DIR_DOWN;
      } else if (!inPlatform &&
                 ((extHeightColExt.GetLeftDownCol() != TILE_COL) && (extHeightColExt.GetRightDownCol() != TILE_COL)) &&
                 ((extHeightColExt.GetLeftDownCol() != TILE_STAIRS_TOP) && (extHeightColExt.GetRightDownCol() != TILE_STAIRS_TOP))) {
        state = RICK_STATE_JUMPING;
        direction = RICK_DIR_DOWN;
      } else if (keyboard.PressedRight() || keyboard.PressedLeft()) {
        state = RICK_STATE_RUNNING;
      }

      break;

    case RICK_STATE_RUNNING:
      if (keyboard.PressedDown() &&
          ((heightColExt.GetLeftDownCol() == TILE_STAIRS) ||
           (heightColExt.GetRightDownCol() == TILE_STAIRS) ||
           (heightColExt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
           (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP))) {
        state = RICK_STATE_CLIMBING;
        direction = RICK_DIR_DOWN;
      } else if ((extColExt.GetLeftDownCol() == 0) && (extColExt.GetRightDownCol() == 0)) {
        if (!inPlatform) {
          state = RICK_STATE_JUMPING;
          direction = RICK_DIR_DOWN;
        }
      } else if (keyboard.PressedUp()) {
        // Start jump
        state = RICK_STATE_JUMPING;
        direction = RICK_DIR_UP;
        // Save pos y
        pos_y_chk = pos_y;
      }

      if (keyboard.PressedRight()) {
        direction |= RICK_DIR_RIGHT;
        direction &= ~RICK_DIR_LEFT;
      } else if (keyboard.PressedLeft()) {
        direction |= RICK_DIR_LEFT;
        direction &= ~RICK_DIR_RIGHT;
      } else {
        // If no update to state while running and no left of right, then stop
        if (state == RICK_STATE_RUNNING)
          state = RICK_STATE_STOP;
        direction &= ~RICK_DIR_LEFT;
        direction &= ~RICK_DIR_RIGHT;          
      }
      break;

    case RICK_STATE_JUMPING:
      if ((direction & RICK_DIR_DOWN)) {
        if (((extHeightColExt.GetLeftDownCol() != 0) && (extHeightColExt.GetLeftDownCol() != TILE_STAIRS)) ||
            ((extHeightColExt.GetRightDownCol() != 0) && (extHeightColExt.GetRightDownCol() != TILE_STAIRS))) {
          // Factor keyboard to keep running without transitioning through stop
          if (keyboard.PressedRight()) {
            state = RICK_STATE_RUNNING;            
          } else if (keyboard.PressedLeft()) {
            state = RICK_STATE_RUNNING;
          } else {
            state = RICK_STATE_STOP;
            direction = RICK_DIR_STOP;
          }
        } else if (inPlatform) {
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
        }
      } else if (direction & RICK_DIR_UP) {
        // REVISIT: hard coded the maximum distance for jumping
        if ((extHeightColExt.GetLeftUpCol() == TILE_COL) || (extHeightColExt.GetRightUpCol() == TILE_COL) ||
            (abs(pos_y_chk - pos_y) >= 5*8)) {
          direction = RICK_DIR_DOWN;
        }
      }

      if (keyboard.PressedUp() &&
          ((heightColInt.GetLeftDownCol() == TILE_STAIRS) ||
           (heightColInt.GetRightDownCol() == TILE_STAIRS))) {
        state = RICK_STATE_CLIMBING;
        direction = RICK_DIR_UP;
      }

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
      break;

    case RICK_STATE_CLIMBING:

      if (keyboard.PressedUp()) {
        // If on stairs, and pressed key up, then keep climbing up
        if (inAirInt) {
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
        } else if (inStairs) {
          state = RICK_STATE_CLIMBING;
          direction = RICK_DIR_UP;
        }
      } else if (keyboard.PressedDown()) {
        // If on stais, and pressed key down, then climb down stairs
        if (inFloor) {
        // No in stairs, stop player because it was previously in stairs
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
        } else if (inStairs) {
          direction = RICK_DIR_DOWN;
        }
      } else {
        if (inAirInt || !inStairs) {
          state = RICK_STATE_STOP;
          direction = RICK_DIR_STOP;
        }
      }

      // Fix horizontal direction
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
      if (!keyboard.PressedUp() && !keyboard.PressedDown()) {
        direction &= ~RICK_DIR_UP;
        direction &= ~RICK_DIR_DOWN;
      }

      break;
    default:
      break;
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

  //printf("Post: State = %d, direction = %d\n", state, direction);
  //printf("Steps in state = %d, steps in direction x = %d, steps in direction y = %d\n", stepsInState, stepsInDirectionX, stepsInDirectionY);
}

void Character::ComputeNextPosition(World* map) {

  //printf("PRE: pos_x = %d pos_y %d\n", pos_x, pos_y);

  // First check if on platform
  if (inPlatform) {
    // Correct y to be on top of platform
    SetPosY(map, GetPosY() - (GetPosY() + GetHeight() - inPlatformPtr->GetY()), false);
  }

  switch(state) {
    case RICK_STATE_STOP:
      break;
    case RICK_STATE_RUNNING:
      if (direction & RICK_DIR_RIGHT) {
        SetPosX(map, GetPosX() + speed_x);
      } else if (direction & RICK_DIR_LEFT) {
        SetPosX(map, GetPosX() - speed_x);
      }
      break;
    case RICK_STATE_JUMPING:      
      if (direction & RICK_DIR_UP)
        SetPosY(map, GetPosY() - speed_y, false);
      else if (direction & RICK_DIR_DOWN)
        SetPosY(map, GetPosY() + speed_y, true);

      if (direction & RICK_DIR_RIGHT)
        SetPosX(map, GetPosX() + speed_x);
      else if (direction & RICK_DIR_LEFT)
        SetPosX(map, GetPosX() - speed_x);

      break;
    case RICK_STATE_CLIMBING:
      if (direction & RICK_DIR_UP) {
        // First, correct x to facilitate moving up
        if ((extColExt.GetLeftUpCol() == TILE_COL) &&
            (extColExt.GetRightUpCol() != TILE_COL) &&
            (heightColExt.GetRightUpCol() != TILE_COL)) {
          SetPosX(map, GetPosX() + speed_x);          
        } else if ((heightColExt.GetLeftUpCol() != TILE_COL) &&
                   (extColExt.GetLeftUpCol() != TILE_COL) &&
                   (extColExt.GetRightUpCol() == TILE_COL)) {
          SetPosX(map, GetPosX() - speed_x);          
        }

        SetPosY(map, GetPosY() - speed_y, false);
      } else if (direction & RICK_DIR_DOWN) {
        // First, correct x to facilitate moving down
        if ((extColExt.GetLeftDownCol() == TILE_COL) &&
            (extColExt.GetRightDownCol() != TILE_COL) &&
            (heightColExt.GetRightDownCol() != TILE_COL)) {
          SetPosX(map, GetPosX() + speed_x);
        } else if ((heightColExt.GetLeftDownCol() != TILE_COL) &&
                   (extColExt.GetLeftDownCol() != TILE_COL) &&
                   (extColExt.GetRightDownCol() == TILE_COL)) {
          SetPosX(map, GetPosX() - speed_x);
        }

        SetPosY(map, GetPosY() + speed_y, false);
      }
      if (direction & RICK_DIR_RIGHT) {
        SetPosX(map, GetPosX() + speed_x);
      } else if (direction & RICK_DIR_LEFT) {
        SetPosX(map, GetPosX() - speed_x);
      }
      break;
    default:
      break;
  }

  //printf("POST: pos_x = %d pos_y %d\n", pos_x, pos_y);
}

void Character::ComputeNextSpeed() {
  //printf("Speed Update %d\n", state);
  switch (state) {
    case RICK_STATE_STOP:
      speed_x = 0.0;
      speed_y = 0.0;
      break;
    case RICK_STATE_RUNNING:
      speed_x = 2.0;
      break;
    case RICK_STATE_JUMPING:
        if (!stepsInState) {
          if (direction & RICK_DIR_UP)
            speed_y = 2.0;
          else
            speed_y = 0.8;
        } else if ((direction & RICK_DIR_UP) && (stepsInDirectionY > 0)) {
          if (speed_y > 0)
            speed_y = speed_y - 0.05;
          else
            speed_y = 0.0;
        } else if ((direction & RICK_DIR_DOWN) && (stepsInDirectionY > 0)) {
          if (speed_y < 2.0)
            speed_y = speed_y + 0.1;
          else
            speed_y = 2.0;
        }
        speed_x = 2.0;
      break;
    case RICK_STATE_CLIMBING:
      speed_x = 2.0;
      speed_y = 2.0;
      break;
    default:      
      speed_x = 2.0;
      speed_y = 2.0;
      break;
  }

  //printf("speed_y = %f\n", speed_y);
}