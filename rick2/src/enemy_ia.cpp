#include "enemy_ia.h"

EnemyIA::EnemyIA() {
  type = ENEMY_IA_WALKER;
  random_decisions = true;
  srand(time(0));
}

EnemyIA::EnemyIA(int _type,
                 int _random_decisions,
                 int _initial_x, int _initial_y,
                 int _limit_x, int _limit_y) {
  type = _type;
  random_decisions = _random_decisions;
  initial_x = _initial_x;
  initial_y = _initial_y;
  limit_x = _limit_x;
  limit_y = _limit_y;
  limited_hor = (limit_x > 0);
  limited_ver = (limit_y > 0);

  srand(time(0));
}

EnemyIA::~EnemyIA() {

}

int EnemyIA::GetType() {
  return type;
}

void EnemyIA::IAStep(Keyboard &keyboard,
                     int direction, int x, int y,
                     bool col_right, bool col_left,
                     int steps_in_x) {
  keyboard.SetKeys(0);

  //printf("[Enemy IA] direction=%d col_right=%d col_left=%d\n", direction, col_right, col_left);
  if (random_decisions) {    
    if ((steps_in_x >= STEPS_IN_HOR_DIRECTION) && ((rand() % RANDOM_DECISION_VALUE) == 0)) {  // REVISIT: totally arbitrary number
      if (direction & CHAR_DIR_LEFT)
        keyboard.SetKeys(KEY_RIGHT);
      else
        keyboard.SetKeys(KEY_LEFT);
      return;
    }
  }

  if (direction == CHAR_DIR_STOP) {
    keyboard.SetKeys(KEY_RIGHT);
  } else if (direction & CHAR_DIR_RIGHT) {
    if (col_right) {
      keyboard.SetKeys(KEY_LEFT);
    } else {
      keyboard.SetKeys(KEY_RIGHT);
    }
  } else if (direction & CHAR_DIR_LEFT) {
    if (col_left) {
      keyboard.SetKeys(KEY_RIGHT);
    } else {
      keyboard.SetKeys(KEY_LEFT);
    }
  }

  // If crossing limits, then recompute decision
  // REVISIT: consider if adding vertical limits
  if (limited_hor) {
    if (direction & CHAR_DIR_RIGHT) {
      if (x >= initial_x + limit_x) {
        keyboard.SetKeys(KEY_LEFT);
      }
    } else if (direction & CHAR_DIR_LEFT) {
      if (x <= initial_x) {
        keyboard.SetKeys(KEY_RIGHT);
      }
    }
  }
}