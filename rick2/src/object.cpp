#include "object.h"

Object::Object() {
  x       = 0;
  y       = 0;
  width   = 0;
  height  = 0;
  visible = false;
  active  = false;
}

Object::Object(int _x, int _y, int _width, int _height, bool _visible, bool _active) {
  x       = _x;
  y       = _y;
  width   = _width;
  height  = _height;
  visible = _visible;
  active  = _active;
}

Object::~Object() {
}