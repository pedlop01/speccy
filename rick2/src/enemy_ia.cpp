#include "enemy_ia.h"
#include "enemy.h"
#include "player.h"

EnemyIA::EnemyIA() {
  type = ENEMY_IA_WALKER;
  random_decisions = true;
  srand(time(0));
}

EnemyIA::EnemyIA(int _type,
                 int _random_decisions, int _randomness, int _block_steps,
                 int _initial_x, int _initial_y, int _orig_x, int _orig_y, 
                 int _limit_x, int _limit_y) {
  type = _type;
  random_decisions = _random_decisions;
  randomness = _randomness;
  block_steps = _block_steps;
  orig_x = _orig_x;
  orig_y = _orig_y;
  initial_x = _initial_x;
  initial_y = _initial_y;
  limit_x = _limit_x;
  limit_y = _limit_y;
  limited_hor = (limit_x > 0);
  limited_ver = (limit_y > 0);

  srand(time(0));
  wait_for_decision = block_steps;
  randomness = RANDOM_DECISION_VALUE;
}

EnemyIA::~EnemyIA() {

}

int EnemyIA::GetType() {
  return type;
}

int EnemyIA::GetOrigX() {
  return orig_x;
}

int EnemyIA::GetOrigY() {
  return orig_y;
}

int EnemyIA::GetLimitX() {
  return limit_x;
}

int EnemyIA::GetLimitY() {
  return limit_y;
}

bool EnemyIA::IsLimited() {
  return limited_hor || limited_ver;
}

bool EnemyIA::RandomDecision(Keyboard& keyboard, int direction, int steps_in_x) {
  if (random_decisions) {    
    if ((rand() % randomness) == 0) {  // REVISIT: totally arbitrary number
      if (direction & CHAR_DIR_LEFT)
        keyboard.SetKeys(KEY_RIGHT);
      else
        keyboard.SetKeys(KEY_LEFT);
      return true;
    }
  }
  return false;
}

bool EnemyIA::WalkerDecision(Keyboard& keyboard, int direction, bool col_right, bool col_left) {
  if (direction & CHAR_DIR_RIGHT) {
    if (col_right) {
      keyboard.SetKeys(KEY_LEFT);
      return true;
    }
  } else if (direction & CHAR_DIR_LEFT) {
    if (col_left) {
      keyboard.SetKeys(KEY_RIGHT);
      return true;
    }
  }
  return false;
}

void EnemyIA::ChaserDecision(Keyboard& keyboard, bool block_hor, int player_x, int player_y, int enemy_x, int enemy_y, int enemy_state, bool over_stairs, bool in_floor) {
  if (!block_hor) {
    if (player_x > enemy_x) {
      keyboard.SetKeys(KEY_RIGHT);
    } else {
      keyboard.SetKeys(KEY_LEFT);
    }
  }

  if (over_stairs && (player_y > enemy_y)) {
    keyboard.SetKeys(KEY_DOWN);
  } else {
    if (enemy_state == CHAR_STATE_CLIMBING) {
      if (!in_floor) {
        if (player_y + 10 > enemy_y) {  // REVISIT
          keyboard.SetKeys(KEY_DOWN);
        } else {
          keyboard.SetKeys(KEY_UP);
        }
      } else {
        if (player_x > enemy_x) {
          keyboard.SetKeys(KEY_RIGHT);
        } else {
          keyboard.SetKeys(KEY_LEFT);
        }
      }
    }
  }
}

bool EnemyIA::CorrectDecisionBasedOnLimits(Keyboard& keyboard, int x, int y, int direction) {
  // REVISIT: consider if adding vertical limits
  if (limited_hor) {
    if (direction & CHAR_DIR_RIGHT) {
      if (x >= orig_x + limit_x) {
        keyboard.SetKeys(KEY_LEFT);
        return true;
      }
    } else if (direction & CHAR_DIR_LEFT) {
      if (x <= orig_x) {
        keyboard.SetKeys(KEY_RIGHT);
        return true;
      }
    }
  }
  return false;
}

void EnemyIA::SetKeyboardBasedOnDirection(Keyboard& keyboard, int direction) {
  int keys = 0;
  if (direction == CHAR_DIR_STOP) {
    keys = KEY_RIGHT;
  }
  if (direction & CHAR_DIR_RIGHT) {
    keys |= KEY_RIGHT;
  } else if (direction & CHAR_DIR_LEFT) {
    keys |= KEY_LEFT;
  }
  if (direction & CHAR_DIR_UP) {
    keys |= KEY_UP;
  } else if (direction & CHAR_DIR_DOWN) {
    keys |= KEY_DOWN;
  }  
  keyboard.SetKeys(keys);
}

void EnemyIA::IAStepWalker(Keyboard &keyboard,
                           int state, int direction, int x, int y,
                           bool col_right, bool col_left,
                           int steps_in_x) {

  bool disable_decisions = (wait_for_decision < block_steps);

  // No IA actions until enemy is in floor  
  if (state == CHAR_STATE_JUMPING)
    return;

  this->SetKeyboardBasedOnDirection(keyboard, direction);

  //printf("[Enemy IA] direction=%d col_right=%d col_left=%d\n", direction, col_right, col_left);
  if(!disable_decisions && this->RandomDecision(keyboard, direction, steps_in_x)) {
    wait_for_decision = 0;
  }

  this->WalkerDecision(keyboard, direction, col_right, col_left);
  wait_for_decision++;

  // If crossing limits, then recompute decision
  this->CorrectDecisionBasedOnLimits(keyboard, x, y, direction);
}

void EnemyIA::IAStepChaser(Keyboard &keyboard,
                           int player_x, int player_y,
                           int state, int direction, int x, int y,
                           bool col_right, bool col_left, bool over_stairs, bool in_floor,
                           int steps_in_x) {

  bool disable_decisions = (wait_for_decision < block_steps);

  // No IA actions until enemy is in floor
  if (state == CHAR_STATE_JUMPING)
    return;

  this->SetKeyboardBasedOnDirection(keyboard, direction);

  //printf("[Enemy IA] direction=%d col_right=%d col_left=%d\n", direction, col_right, col_left);
  if((state == CHAR_STATE_RUNNING) && !disable_decisions && this->RandomDecision(keyboard, direction, steps_in_x)) {
    wait_for_decision = 0;
  }

  if((state == CHAR_STATE_RUNNING) && this->WalkerDecision(keyboard, direction, col_right, col_left)) {    
    wait_for_decision = 0;
  }
  
  this->ChaserDecision(keyboard, disable_decisions, player_x, player_y, x, y, state, over_stairs, in_floor);
  wait_for_decision++;

  // If crossing limits, then recompute decision
  if (this->CorrectDecisionBasedOnLimits(keyboard, x, y, direction)) {
    wait_for_decision = 0;
  }

  // If in stairs, don´t allow movements right or left
  if (state == CHAR_STATE_CLIMBING) {
    int keys = keyboard.GetKeys();
    keys &= ~KEY_LEFT;
    keys &= ~KEY_RIGHT;
    keyboard.SetKeys(keys);
  }

}

void EnemyIA::IAStep(Keyboard &keyboard,
                     Player* player, Enemy* enemy) {
  int player_x = player->GetPosX();
  int player_y = player->GetPosY();
  int state = enemy->GetState();
  int direction = enemy->GetDirection();
  int x = enemy->GetPosX();
  int y = enemy->GetPosY();
  bool over_stairs = enemy->GetOverStairs();
  bool in_floor = enemy->GetInFloor();
  int steps_in_x = enemy->GetStepsInDirectionX();
  bool col_right;
  bool col_left;  

  col_right = (enemy->GetWeightColExt()->GetRightDownCol() == TILE_COL) ||
              (enemy->GetWeightColExt()->GetRightUpCol() == TILE_COL);

  col_left  = (enemy->GetWeightColExt()->GetLeftDownCol() == TILE_COL) ||
              (enemy->GetWeightColExt()->GetLeftUpCol() == TILE_COL);

  switch(type) {
    case ENEMY_IA_WALKER:
      this->IAStepWalker(keyboard, state, direction, x, y, col_right, col_left, steps_in_x);
      break;
    case ENEMY_IA_CHASER:
      // A chaser only behaves as a chaser if player is on the limits
      if (this->IsLimited() && this->OnIALimits(player_x, player_y))
        this->IAStepChaser(keyboard, player_x, player_y, state, direction, x, y, col_right, col_left, over_stairs, in_floor, steps_in_x);
      // REVISIT: I would rather prefer doing something when no in limits
      break;
    default:
      break;
  }
}

bool EnemyIA::OnIALimits(int player_x, int player_y) {
  return ((player_x >= orig_x) &&
          (player_x < orig_x + limit_x) &&
          (player_y >= orig_y) &&
          (player_y < orig_y + limit_y));
}