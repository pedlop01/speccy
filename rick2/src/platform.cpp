#include "platform.h"

Platform::Platform() {
  moving = true;
}

Platform::Platform(int _x,
                   int _y,
                   int _width,
                   int _height,
                   bool _visible,
                   bool _active,
                   int _dir,
                   int _displacement) : 
  Object(_x, _y, _width, _height, _visible, _active) {

  moving = true;
  direction = _dir;
  displacement = _displacement;
  start_x = _x;
  start_y = _y;
}

Platform::~Platform() {

}

void Platform::platformStep() {
  if (direction == PLATFORM_DIR_UP) {
    if (abs(y - start_y) < displacement) {
      y--;
    } else {
      start_y = start_y - displacement;
      direction = PLATFORM_DIR_DOWN;
    }
  } else if (direction == PLATFORM_DIR_DOWN) {
    if (abs(y - start_y) < displacement) {
      y++;
    } else {
      start_y = start_y + displacement;
      direction = PLATFORM_DIR_UP;
    }
  }
}