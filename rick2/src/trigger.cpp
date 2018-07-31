#include "trigger.h"

Trigger::Trigger(
  int _id,
  int _x, int _y, int _width, int _height,
  int _action_event, int _action_face, bool _recursive) {

  id      = _id;
  x       = _x;
  y       = _y;
  width   = _width;
  height  = _height;

  action_event = _action_event;
  action_face  = _action_face;

  recursive = _recursive;

  player_was_in_trigger = false;
  already_triggered = false;
  trigger_targets = false;

  steps = 0;
}

Trigger::~Trigger() {

}

void Trigger::Reset() {
  player_was_in_trigger = false;
  already_triggered = false;
  trigger_targets = false;

  for (vector<bool>::iterator it = targets_triggered.begin(); it != targets_triggered.end(); it++) {
    *it = false;
  }

  steps = 0;
}

void Trigger::AddTarget(Object* _object, int _delay, bool _trigger) {
  targets.push_back(_object);
  targets_delay.push_back(_delay);
  targets_triggered.push_back(false);
  targets_trigger.push_back(_trigger);
}

bool Trigger::InTrigger(int _x, int _y, int _width, int _height) {
  return (((_x >= x) && (_x <= (x + width)) &&
           (_y >= y) && (_y <= (y + height))) ||

          (((_x + _width) >= x) && ((_x + _width) <= (x + width)) &&
           (_y >= y) && (_y <= (y + height))) ||

          ((_x >= x) && (_x <= (x + width)) &&
           ((_y + _height) >= y) && ((_y + _height) <= (y + height))) ||

          (((_x + _width) >= x) && ((_x + _width) <= (x + width)) &&
           ((_y + _height) >= y) && ((_y + _height) <= (y + height))));
}

void Trigger::TriggerStep(int _x, int _y, int _width, int _height,
                          int _face, int _state) {

  if ((_state == CHAR_STATE_DYING) || (_state == CHAR_STATE_DEAD))
    return;

  if (!trigger_targets) {
    //printf("[Trigger %d] No trigger target. In analysis!\n", id);

    // Recursive triggers keep triggering all the time.
    // This is the same piece of code than the regular one for
    // initiating triggers. Not moved there to avoid all the 
    // next unnecessary computations.
    if (recursive && already_triggered) {
      steps = 0;
      trigger_targets = true;
      //printf("[Trigger %d] Initiate targets for this recursive trigger already triggered trigger!\n", id);
      return;
    }
      
    bool player_enters     = false;
    bool player_stays      = false;
    bool player_exits      = false;
    bool player_in_trigger = false;
    bool expected_face     = false;
    bool expected_event    = false;

    player_in_trigger = InTrigger(_x, _y, _width, _height);

    player_enters =  player_in_trigger && !player_was_in_trigger;
    player_stays  =  player_in_trigger &&  player_was_in_trigger;
    player_exits  = !player_in_trigger &&  player_was_in_trigger;

    expected_face = ((action_face == ACTION_FACE_RIGHT) ? (_face == CHAR_DIR_RIGHT) :
                     (action_face == ACTION_FACE_LEFT)  ? (_face == CHAR_DIR_LEFT)  :
                                                          true);

    expected_event = ((action_event == ACTION_EVENT_ENTERS) ? player_enters :
                      (action_event == ACTION_EVENT_STAYS)  ? player_stays  :
                      (action_event == ACTION_EVENT_EXITS)  ? player_exits  :
                      (action_event == ACTION_EVENT_HITS)   ? player_stays && (_state == CHAR_STATE_HITTING) :
                                                              false);

    player_was_in_trigger = player_in_trigger;

    if (expected_event && expected_face) {
      steps = 0;
      trigger_targets = true;
      //printf("[Trigger %d] Initiate targets for this trigger!\n", id);
    }
  } else {
    int num_target = 0;
    for (vector<Object*>::iterator it = targets.begin(); it != targets.end(); it++) {
      Object* object = *it;
      if ((steps >= targets_delay[num_target]) && !targets_triggered[num_target]) {
        // Set trigger on for object. Objects shall put trigger to false once
        // the action has been taken. Note that, recursive triggers may keep setting
        // object trigger continously.
        if (targets_trigger[num_target])
          object->SetTrigger();
        else
          object->UnsetTrigger();
        targets_triggered[num_target] = true;
      }
      num_target++;

    }

    // Check if all triggers have been set.
    bool all_completed = true;
    for (vector<bool>::iterator it = targets_triggered.begin(); (it != targets_triggered.end()) && all_completed; it++) {
      if (!(*it)) {
        all_completed = false;
      }
    }
    if (all_completed) {
      already_triggered = true;
      //printf("[Trigger %d] Completed targets for this trigger!\n", id);
      for (vector<bool>::iterator it = targets_triggered.begin(); it != targets_triggered.end(); it++) {
        *it = false;
      }
      trigger_targets = false;
      steps = 0;
    } else {
      steps++;
    }
  }
}