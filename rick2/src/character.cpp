#include "character.h" // class's header file

// class constructor
Character::Character() {
  pos_x = 264;  // REVISIT: should be 0
  pos_y = 2000; // REVISIT: should be 0

  height = 20;  // REVISIT: should be 0
  width  = 16;  // REVISIT: should be 0
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
}

Character::Character(const char* file) {
  // REVISIT: most of this information should be read from the file
  pos_x = 264;  // REVISIT: should be 0
  pos_y = 2000; // REVISIT: should be 0
  height = 20;  // REVISIT: should be 0
  width  = 16;  // REVISIT: should be 0
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
      printf("\t\tSprite %d: x = %d, y = %d, width = %d, height = %d\n", num_sprites,
                                                                         sprite.attribute("x").as_int(),
                                                                         sprite.attribute("y").as_int(),
                                                                         sprite.attribute("width").as_int(),
                                                                         sprite.attribute("height").as_int());
      player_anim->AddSprite(sprite.attribute("x").as_int(),
                             sprite.attribute("y").as_int(),
                             sprite.attribute("width").as_int(),
                             sprite.attribute("height").as_int());      
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
  int character_width = this->width;
  int character_height = this->height;

  // Compute the displacement in x
  int desp_x = (x > pos_x) ? (x - pos_x) : (pos_x - x);

  // Compute x and y corrections to draw world
  int tile_col_x;
  int tile_col_up_y   = pos_y / tile_height;
  int tile_col_down_y = (pos_y + character_height - 1) / tile_height;
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
  int tile_width = map->GetTilesetTileWidth();
  int tile_height = map->GetTilesetTileHeight();
  int character_width = this->width;
  int character_height = this->height;

  // Compute the displacement in x
  int desp_y = (y > pos_y) ? (y - pos_y) : (pos_y - y);

  // Compute x and y corrections to draw world
  int tile_col_y;
  int tile_col_up_x   = pos_x / tile_width;
  int tile_col_down_x = (pos_x + character_width - 1) / tile_width;
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


void Character::GetCollisionsExternalHeightBoxExtOrig(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              this->GetCorrectedPosX(),
                              this->GetCorrectedPosY() - 1,
                              width_orig - 1,
                              height_orig + 1);
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

void Character::ComputeCollisions(World* map) {
  int down_left_x;
  int down_right_x;
  int down_y;

  this->GetCollisionsExternalBoxInt(map, extColInt);
  this->GetCollisionsExternalBoxExt(map, extColExt);
  this->GetCollisionsExternalWidthBoxExt(map, extWidthColExt);
  this->GetCollisionsExternalHeightBoxExt(map, extHeightColExt);
  this->GetCollisionsExternalHeightBoxExtOrig(map, extHeightColExtOrig);
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

  // Check if there is a collision with the tiles
  inStairs = ((heightColInt.GetLeftUpCol() == TILE_STAIRS) ||
              (heightColInt.GetRightUpCol() == TILE_STAIRS) ||
              (heightColInt.GetLeftUpCol() == TILE_STAIRS) ||
              (heightColInt.GetRightUpCol() == TILE_STAIRS) ||
              (heightColInt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
              (heightColInt.GetRightDownCol() == TILE_STAIRS_TOP));

  overStairs = (heightColExt.GetLeftDownCol() == TILE_STAIRS_TOP) ||
               (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP);

  inFloor = inPlatform ||
            (extHeightColExt.GetLeftDownCol() == TILE_COL) ||
            (extHeightColExt.GetRightDownCol() == TILE_COL);

  inAir = !inPlatform &&
          ((extHeightColExt.GetLeftDownCol() == 0) ||             // No tile is air
           (extHeightColExt.GetLeftDownCol() == TILE_STAIRS)) &&  // Stairs is also air
          ((extHeightColExt.GetRightDownCol() == 0) ||
           (extHeightColExt.GetRightDownCol() == TILE_STAIRS));

  inAirInt = ((heightColInt.GetLeftDownCol() == 0) &&
              (heightColInt.GetRightDownCol() == 0));

  // - Collision with head on collisionable tile
  collisionHead = (extHeightColExt.GetLeftUpCol() == TILE_COL) ||
                  (extHeightColExt.GetRightUpCol() == TILE_COL);

  collisionHeadOrig = (extHeightColExtOrig.GetLeftUpCol() == TILE_COL) ||
                      (extHeightColExtOrig.GetRightUpCol() == TILE_COL);

  overStairsRight = (extColExt.GetLeftDownCol() == TILE_COL) &&
                    (extColExt.GetRightDownCol() == TILE_STAIRS_TOP) &&
                    (heightColExt.GetRightDownCol() == TILE_STAIRS_TOP);

  overStairsLeft = (extColExt.GetRightDownCol() == TILE_COL) &&
                   (extColExt.GetLeftDownCol() == TILE_STAIRS_TOP) &&
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

void Character::ComputeNextState(Keyboard& keyboard) {  
  int prevDirection;
  
  // Save current state before computing next state
  prevState = state;
  // Save current direction before computing next state and direction
  prevDirection = direction;

  //printf("Pre: State = %d, direction = %d, face = %d\n", state, direction, face);

  switch(state) {
    case RICK_STATE_STOP:
    case RICK_STATE_RUNNING:

      if (inAir) {
        state = RICK_STATE_JUMPING;
        direction = RICK_DIR_DOWN;
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
          if ((overStairsRight && (face & RICK_DIR_RIGHT)) || 
              (overStairsLeft  && (face & RICK_DIR_LEFT))  ||
              (!overStairsRight && !overStairsLeft && overStairs)) {
            state = RICK_STATE_CLIMBING;
            direction = RICK_DIR_DOWN;
          } else {
            height = 15;                             // REVISIT: Hard-coded. Need to be obtained from state (now it is in animation)
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
        height = 20;                            // REVISIT: Hard-coded. Need to be obtained from state (now it is in animation)
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

  //printf("PRE: pos_x = %d pos_y %d\n", pos_x, pos_y);
  //printf("pos_x = %d, pos_y = %d, speed_x = %f, speed_y = %f\n", pos_x, pos_y, speed_x, speed_y);

  // First check if on platform
  // REVISIT: platform should be revisited once they will be properly implemented
  if (inPlatform) {
    // Correct y to be on top of platform
    SetPosY(map, GetPosY() - (GetPosY() + GetHeight() - inPlatformPtr->GetY()), false);
    if (inPlatformPtr->GetDirection() == OBJ_DIR_RIGHT) {
      SetPosX(map, GetPosX() + 1);
    } else if (inPlatformPtr->GetDirection() == OBJ_DIR_LEFT) {
      SetPosX(map, GetPosX() - 1);
    }
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
          SetPosX(map, GetPosX() + HOR_SPEED_MAX);
        } else if ((heightColExt.GetLeftUpCol() != TILE_COL) &&
                   (extColExt.GetLeftUpCol() != TILE_COL) &&
                   (extColExt.GetRightUpCol() == TILE_COL)) {
          SetPosX(map, GetPosX() - HOR_SPEED_MAX);
        }

        SetPosY(map, GetPosY() - speed_y, false);
      } else if (direction & RICK_DIR_DOWN) {

        // First, correct x to facilitate moving down
        if (overStairsRight) {
          SetPosX(map, GetPosX() + HOR_SPEED_MAX);
        } else if (overStairsLeft) {
          SetPosX(map, GetPosX() - HOR_SPEED_MAX);
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
    default:      
      speed_x = HOR_SPEED_MAX;
      speed_y = VERT_SPEED_MAX;
      break;
  }

  //printf("speed_y = %f\n", speed_y);
}

void Character::CharacterStep(World* map, Keyboard& keyboard) {
  // Collisions with world and platforms
  this->ComputeCollisions(map);
  // Compute next state
  this->ComputeNextState(keyboard);
  // Compute next position
  this->ComputeNextPosition(map);
  // Re-calulate speed
  this->ComputeNextSpeed();
  // Compute next animation frame
  if (direction == RICK_DIR_STOP)
    animations[state]->ResetAnim();
  else
    animations[state]->AnimStep();
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