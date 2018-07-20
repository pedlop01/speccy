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

void Shoot::UpdateFSMState(World* map) {
  bool inCol;
  bool rightCol;
  bool leftCol;
  bool upCol;
  bool downCol;
  bool blockCollision;
  Animation* current_anim;

  upCol    = (extColExt.GetLeftUpCol() == TILE_COL) &&
             (extColExt.GetRightUpCol() == TILE_COL);

  downCol  = (extColExt.GetLeftDownCol() == TILE_COL) &&
             (extColExt.GetRightDownCol() == TILE_COL);

  rightCol = (extColExt.GetRightUpCol() == TILE_COL) &&
             (extColExt.GetRightDownCol() == TILE_COL);

  leftCol  = (extColExt.GetLeftUpCol() == TILE_COL) &&
             (extColExt.GetLeftDownCol() == TILE_COL  );

  inCol = (direction & OBJ_DIR_RIGHT && rightCol) ||
          (direction & OBJ_DIR_LEFT  && leftCol)  ||
          (direction & OBJ_DIR_UP    && upCol)    ||
          (direction & OBJ_DIR_DOWN  && downCol);

  ComputeCollisionBlocks(map);
  ComputeCollisionObjects(map);
  blockCollision = blockColDown ||
                   blockColUp   ||
                   blockColLeft ||
                   blockColRight;

  switch(state) {
    case OBJ_STATE_STOP:
      // REVISIT: Implement trigger actions here
      state = OBJ_STATE_MOVING;
      break;

    case OBJ_STATE_MOVING:

      if (inCol || blockCollision || itemCol) {
        state = OBJ_STATE_DEAD;
        if (itemCol) {
          itemColPtr->SetKilled();
        }
      }

      break;

    default:
      break;
  }
}