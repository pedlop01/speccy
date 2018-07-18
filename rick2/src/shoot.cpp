#include "shoot.h"

Shoot::Shoot() {
  obj_type = OBJ_SHOOT;
}

Shoot::Shoot(const char* file,
             int _x,
             int _y,
             int _width,
             int _height,
             int _direction) : 
  Object(_x, _y, _width, _height, true, true) {
  obj_type = OBJ_SHOOT;

  // Initialize animations from parent class
  Object::Init(file, _x, _y, _width, _height, true, true, OBJ_STATE_MOVING, _direction, 4.0, 4.0, 4.0, 0.0, 0.0, 0.0);
}

Shoot::~Shoot() {

}

void Shoot::UpdateFSMState() {
  bool inCol;
  bool rightCol;
  bool leftCol;
  bool upCol;
  bool downCol;
  Animation* current_anim;

  upCol    = ((extColExt.GetLeftUpCol() != 0) ||
              (extColExt.GetRightUpCol() != 0));

  downCol  = ((extColExt.GetLeftDownCol() != 0) ||
              (extColExt.GetRightDownCol() != 0));

  rightCol = ((extColExt.GetRightUpCol() != 0) ||
              (extColExt.GetRightDownCol() != 0));

  leftCol  = ((extColExt.GetLeftUpCol() != 0) ||
              (extColExt.GetLeftDownCol() != 0));

  inCol = (direction & OBJ_DIR_RIGHT && rightCol) ||
          (direction & OBJ_DIR_LEFT  && leftCol)  ||
          (direction & OBJ_DIR_UP    && upCol)    ||
          (direction & OBJ_DIR_DOWN  && downCol);

  switch(state) {
    case OBJ_STATE_STOP:
      // REVISIT: Implement trigger actions here
      state = OBJ_STATE_MOVING;
      break;

    case OBJ_STATE_MOVING:
      if (inCol) {
        state = OBJ_STATE_DEAD;
      }

      break;

    default:
      break;
  }
}