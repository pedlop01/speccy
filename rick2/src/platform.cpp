#include "platform.h"

Action::Action() {

}

Action::~Action() {

}

Action::Action(int _direction, int _desp, int _wait, float _speed) {
  direction = _direction;
  desp = _desp;
  wait = _wait;
  speed = _speed;
}

Platform::Platform() {
  obj_type = OBJ_PLATFORM;
  actions.clear();
  current_action = actions.begin();  // REVISIT
  current_desp = 0;
  current_wait_time = 0;
}

Platform::Platform(const char* file,
                   int _id,
                   int _ini_state,
                   int _x,
                   int _y,
                   int _width,
                   int _height,
                   bool _visible,
                   bool _recursive,
                   bool _one_use) : 
  Object(_x, _y, _width, _height, _visible, true) {
  obj_type = OBJ_PLATFORM;

  platform_id = _id;
  ini_state = _ini_state;
  ini_x = _x;
  ini_y = _y;
  recursive = _recursive;
  one_use = _one_use;
  current_action = actions.begin();  // REVISIT
  current_desp = 0;
  current_wait_time = 0;
  // Trigger is false by default. Platforms moving without trigger
  // go directly to state MOVING, so no need to use trigger
  trigger = false;

  // Initialize animations from parent class
  // Speeds and direction are not really relevant for initialization  
  Object::Init(file, _x, _y, _width, _height, _visible, true, _ini_state, OBJ_DIR_STOP, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0);
}

Platform::~Platform() {
  // Remove all created actions
  for(list<Action*>::iterator it = actions.begin(); it != actions.end(); it++) {
    delete *it;
  }
  actions.clear();
}

void Platform::AddAction(int direction, int desp, int wait, float speed) {
  Action* action = new Action(direction, desp, wait, speed);
  actions.push_back(action);

  // Initialize current_action if first push
  if (actions.size() == 1) {
    current_action = actions.begin();
  }
}

// No use of ObjectStep
void Platform::PlatformStep() {
  bool advance_action = false;
  int  current_speed;

  // If no actions, then return
  if (actions.size() == 0) return;

  if (state == OBJ_STATE_STOP) {
    // wait for trigger before moving
    if (trigger) {
      trigger = false;
      state = OBJ_STATE_MOVING;      
      current_desp = 0;
      current_wait_time = 0;
      current_action = actions.begin();
    } else {
      // No actions for this platform yet      
      return;
    }
  }

  // Check if we have completed all actions
  if (current_action == actions.end()) {
    if (one_use) {
      // Object still is present but can not be used
      return;
    } else if (recursive) {
      current_action = actions.begin();
    } else {      
      state = OBJ_STATE_STOP;
      // Make sure trigger is cleared when sequence is completed.
      trigger = false;
      return;
    }
  }

  // Handle current actions
  Action* current_action_ptr = *current_action;
  current_speed = current_action_ptr->GetSpeed();
  //printf("platform dir=%d, desp=%d, wait=%d\n", current_action_ptr->GetDirection(), current_action_ptr->GetDesp(), current_action_ptr->GetWait());
  switch (current_action_ptr->GetDirection()) {    
    case OBJ_DIR_STOP:
      // only wait time can be used here
      if (current_wait_time >= current_action_ptr->GetWait()) {
        current_action++;
      }
      break;
    case OBJ_DIR_RIGHT:
    case OBJ_DIR_LEFT:
      if (current_action_ptr->GetDesp() != 0) {
        if (current_action_ptr->GetDirection() == OBJ_DIR_RIGHT) {
          x += current_speed;
        } else {
          x -= current_speed;
        }
        current_desp += current_speed;
        if (current_desp >= current_action_ptr->GetDesp()) {
          advance_action = true;
        }
      } else {
        if (current_wait_time >= current_action_ptr->GetWait()) {
          advance_action = true;
        }
      }
      break;
    case OBJ_DIR_UP:
    case OBJ_DIR_DOWN:
      if (current_action_ptr->GetDesp() != 0) {
        if (current_action_ptr->GetDirection() == OBJ_DIR_DOWN) {
          y += current_speed;
        } else {
          y -= current_speed;
        }
        current_desp += current_speed;
        if (current_desp >= current_action_ptr->GetDesp()) {          
          advance_action = true;
        }
      } else {        
        if (current_wait_time >= current_action_ptr->GetWait()) {                    
          advance_action = true;
        }
      }
      break;
    default:
      break;
  }

  if (advance_action) {
    current_action++;
    current_desp = 0;
    current_wait_time = 0;
  }

  current_wait_time++;
}