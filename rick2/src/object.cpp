#include "object.h"

int Object::id = 0;

Object::Object() {
  x       = 0;
  y       = 0;
  width   = 0;
  height  = 0;
  visible = false;
  active  = false;

  steps_in_state = 0;
  steps_in_direction_x = 0;
  steps_in_direction_y = 0;

  id++;
}

Object::Object(int _x, int _y, int _width, int _height, bool _visible, bool _active) {
  x       = _x;
  y       = _y;
  width   = _width;
  height  = _height;
  visible = _visible;
  active  = _active;

  steps_in_state = 0;
  steps_in_direction_x = 0;
  steps_in_direction_y = 0;

  id++;
}

Object::~Object() {
}

void Object::ComputeCollisions(World* map) {

}

void Object::ComputeNextState() {

}

void Object::ComputeNextPosition(World* map) {

}

void Object::ComputeNextSpeed() {

}

void Object::ObjectStep(World* map) {
  this->ComputeCollisions(map);
  this->ComputeNextState();
  this->ComputeNextPosition(map);
  this->ComputeNextSpeed();
}