#include "bomb.h"

Bomb::Bomb() {
  obj_type = OBJ_BOMB;
}

Bomb::Bomb(const char* file,
             int _x,
             int _y,
             int _width,
             int _height,
             int _direction) : 
  Object(_x, _y, _width, _height, true, true) {
  obj_type = OBJ_BOMB;

  // Initialize animations from parent class
  Object::Init(file, _x, _y, _width, _height, true, true, OBJ_STATE_MOVING, _direction, 1.0, 1.0, 1.0, 0.2, 3.0, 1.0);
}

Bomb::~Bomb() {

}

void Bomb::UpdateFSMState() {
  bool inCol;
  bool rightCol;
  bool leftCol;
  bool upCol;
  bool downCol;
  bool inAir;  
  Animation* current_anim;

  inAir = ((extColExt.GetLeftDownCol() == 0) &&
           (extColExt.GetRightDownCol() == 0));

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
      // wait until animation completes
      current_anim = this->GetCurrentAnimation();
      if (current_anim->CompletedLastAnim()) {
        state = OBJ_STATE_DYING;
      }

      if (inAir) {
        direction |= OBJ_DIR_DOWN;
      } else {
        direction &= ~OBJ_DIR_DOWN;
      }
      break;

    case OBJ_STATE_DYING:
      // wait until animation completes
      current_anim = this->GetCurrentAnimation();
      if (current_anim->CompletedLastAnim()) {
        state = OBJ_STATE_DEAD;
      }      
      break;

    default:
      break;
  }
}