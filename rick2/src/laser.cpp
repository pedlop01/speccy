#include "laser.h"

Laser::Laser() {
  obj_type = OBJ_LASER;
  laser_type = LASER_TYPE_RECURSIVE;
}

Laser::Laser(const char* file,
                   int _x,
                   int _y,
                   int _width,
                   int _height,
                   int _type,
                   int _speed,
                   int _direction) : 
  Object(_x, _y, _width, _height, true, true) {
  obj_type = OBJ_LASER;

  start_x = x;
  start_y = y;
  laser_type = _type;

  // Initialize animations from parent class
  Object::Init(file, _x, _y, _width, _height, true, true, OBJ_STATE_STOP, _direction, _speed, _speed, _speed, _speed, _speed, _speed);
}

Laser::~Laser() {

}

void Laser::UpdateFSMState(World* map) {
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

  inCol = (direction & OBJ_DIR_RIGHT & rightCol) ||
          (direction & OBJ_DIR_LEFT  & leftCol)  ||
          (direction & OBJ_DIR_UP    & upCol)    ||
          (direction & OBJ_DIR_DOWN  & downCol);

  switch(state) {
    case OBJ_STATE_STOP:
      // REVISIT: Implement trigger actions here
      state = OBJ_STATE_MOVING;
      break;

    case OBJ_STATE_MOVING:
      if (inCol) {
        state = OBJ_STATE_DYING;
      }

      break;
    case OBJ_STATE_DYING:
      // wait until animation completes
      current_anim = this->GetCurrentAnimation();
      if (current_anim->CompletedLastAnim()) {
        // REVISIT: implement dying actions here        
        state = OBJ_STATE_STOP;
        x = start_x;
        y = start_y;
      }      
      break;

    default:
      break;
  }
}