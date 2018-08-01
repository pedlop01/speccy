#ifndef ENEMYIA_H
#define ENEMYIA_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <time.h>

#include "rick_params.h"
#include "keyboard.h"

#define ENEMY_IA_WALKER 0
#define ENEMY_IA_CHASER 1

#define STEPS_IN_HOR_DIRECTION 200
#define RANDOM_DECISION_VALUE  15

class EnemyIA {
  private:
    int  type;
    bool random_decisions;
    int  initial_x;
    int  initial_y;
    int  limit_x;
    int  limit_y;
    bool limited_hor;
    bool limited_ver;

  public:    
	EnemyIA();
    EnemyIA(int _type,
            int _random_decisions,
            int initial_x, int _initial_y,
            int limit_x, int limit_y);
    ~EnemyIA();

    int GetType();

    void IAStep(Keyboard &keyboard,
                int direction, int x, int y,
                bool col_right, bool col_left,
                int steps_in_x);
};

#endif // ENEMYIA_H
