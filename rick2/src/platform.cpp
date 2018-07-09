#include "platform.h"

Platform::Platform() {
  obj_type = OBJ_PLATFORM;
  moving = true;
}

Platform::Platform(const char* file,
                   int _x,
                   int _y,
                   int _width,
                   int _height,
                   bool _visible,
                   bool _active,
                   int _dir,
                   int _displacement) : 
  Object(_x, _y, _width, _height, _visible, _active) {
  obj_type = OBJ_PLATFORM;

  moving = true;
  direction = _dir;
  displacement = _displacement;
  start_x = _x;
  start_y = _y;

  // Initialize animations from parent class
  Object::Init(file, x, y, width, height, visible, active, OBJ_STATE_STOP, direction, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
}

Platform::~Platform() {

}

// No use of ObjectStep
void Platform::PlatformStep() {

  if (direction == OBJ_DIR_UP) {
    if (abs(y - start_y) < displacement) {
      y--;
    } else {
      start_y = start_y - displacement;
      direction = OBJ_DIR_DOWN;
    }
  } else if (direction == OBJ_DIR_DOWN) {
    if (abs(y - start_y) < displacement) {
      y++;
    } else {
      start_y = start_y + displacement;
      direction = OBJ_DIR_UP;
    }
  }

  if (direction == OBJ_DIR_LEFT) {
    if (abs(x - start_x) < displacement) {
      x--;
    } else {
      start_x = start_x - displacement;
      direction = OBJ_DIR_RIGHT;
    }
  } else if (direction == OBJ_DIR_RIGHT) {
    if (abs(x - start_x) < displacement) {
      x++;
    } else {
      start_x = start_x + displacement;
      direction = OBJ_DIR_LEFT;
    }
  }
}