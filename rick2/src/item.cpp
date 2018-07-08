#include "item.h"

Item::Item() {
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
        state = OBJ_STATE_DEAD;
      } else if (inAir) {
        state = OBJ_STATE_MOVING;
        direction = OBJ_DIR_DOWN;
      } else {
        state = OBJ_STATE_STOP;
        direction = OBJ_DIR_STOP;
      }

      break;

    default:
      break;
  }
}

void Item::ObjectStep(World* map, Character* player) {
  // Call parent
  Object::ObjectStep(map, player);
}