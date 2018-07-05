#include "object.h"

int Object::id = 0;

Object::Object() {
  x       = 0;
  y       = 0;
  width   = 0;
  height  = 0;
  visible = false;
  active  = false;
  id++;
}

Object::Object(int _x, int _y, int _width, int _height, bool _visible, bool _active) {
  x       = _x;
  y       = _y;
  width   = _width;
  height  = _height;
  visible = _visible;
  active  = _active;
  id++;
}

Object::~Object() {
}