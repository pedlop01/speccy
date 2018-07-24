#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <list>

#include "rick_params.h"
#include "object.h"

class Action {
  private:
   int direction;
   int desp;
   int wait;
   float speed;

  public:
    Action();
    ~Action();

    Action(int _direction, int _desp, int _wait, float _speed);

    int   GetDirection() { return direction; }
    int   GetDesp()      { return desp;      }
    int   GetWait()      { return wait;      }
    float GetSpeed()     { return speed;     }
};

class Platform : public Object {
  private:
    int  platform_id;
    int  ini_state;
    int  ini_x;
    int  ini_y;    
    bool recursive;
    bool one_use;    

    bool trigger;
    int  current_desp;
    int  current_wait_time;

    // List of actions
    list<Action*> actions;
    list<Action*>::iterator current_action;

  public:
    Platform();
    Platform(const char* file,
             int _id,
             int _ini_state,
             int _x,
             int _y,
             int _width,
             int _height,
             bool _visible,
             bool _recursive,
             bool _one_use);
    ~Platform();

    // Add new action
    void AddAction(int direction, int desp, int wait, float speed);

    // Set the onehot trigger
    void SetTrigger() { trigger = true; }
    int  GetID()      { return platform_id; }

    // Get currection direction of the platform based on current action
    int   GetDirection();
    float GetSpeed();

    void PlatformStep();
};

#endif // PLATFORM_H