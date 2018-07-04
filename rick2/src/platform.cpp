#include "platform.h"

Platform::Platform() {
  moving = true;
}

Platform::Platform(int _x, int _y, int _width, int _height, bool _visible, bool _active) {
  Object(_x, _y, _width, _height, _visible, _active);
  moving = true;
}

Platform::~Platform() {

}