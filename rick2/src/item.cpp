#include "item.h"

Item::Item() {
  steps_dying = 0;
  obj_type = OBJ_ITEM;
}

Item::~Item() {
  printf("Calling destructor!\n");
}

void Item::UpdateFSMState() {
  bool inAir;  

  inAir = ((extColExt.GetLeftDownCol() == 0) &&
           (extColExt.GetRightDownCol() == 0));

  switch(state) {
    case OBJ_STATE_STOP:
    case OBJ_STATE_MOVING:

      if (playerCol) {
        if(strcmp(name, "bonus") != 0) {
          state = OBJ_STATE_DEAD;
        } else {
          state = OBJ_STATE_DYING;
        }
      } else if (inAir) {
        state = OBJ_STATE_MOVING;
        direction = OBJ_DIR_DOWN;
      } else {
        state = OBJ_STATE_STOP;
        direction = OBJ_DIR_STOP;
      }

      break;

    case OBJ_STATE_DYING:
      steps_dying++;
      if (steps_dying >= 30) {   // REVISIT: hard-coded. I do not really know if it is good to have this as a parameter
        state = OBJ_STATE_DEAD;
      }      
      break;
    default:
      break;
  }
}

void Item::ComputeNextPosition(World* map) {
  //printf("[ITEM] ComputeNextPosition x = %d, y = %d\n", GetX(), GetY());

  switch(state) {
    case OBJ_STATE_STOP:
      break;

    case OBJ_STATE_MOVING:
      if (direction & OBJ_DIR_UP)
        SetY(map, GetY() - speed_y);
      else if (direction & OBJ_DIR_DOWN)
        SetY(map, GetY() + speed_y);

      if (direction & OBJ_DIR_RIGHT)
        SetX(map, GetX() + speed_x);
      else if (direction & OBJ_DIR_LEFT)
        SetX(map, GetX() - speed_x);

      break;
    
    case OBJ_STATE_DYING:
      y -= speed_y;

      break;

    default:
      break;
  }
}

void Item::ObjectStep(World* map, Character* player) {
  // Call parent
  Object::ObjectStep(map, player);
}
