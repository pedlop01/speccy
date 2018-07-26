#ifndef HAZARD_H
#define HAZARD_H

#include <stdio.h>
#include <stdlib.h>
#include <list>

#include "rick_params.h"
#include "character.h"
#include "object.h"

class Hazard : public Object {
  private:
    int  hazard_id;
    int  ini_x;
    int  ini_y;    

    int  current_desp;
    int  current_wait_time;    

    bool trigger;
    bool always_trigger;

    // List of actions
    list<Action*> actions;
    list<Action*>::iterator current_action;

  public:
    Hazard();
    Hazard(const char* file,
           int _id,
           int _x,
           int _y,
           int _width,
           int _height,
           bool _trigger);
    ~Hazard();

    // Add new action
    void AddAction(int direction, int desp, int wait, float speed, bool enabled);

    // Set the onehot trigger
    void SetTrigger() { trigger = true; }

    int  GetID()      { return obj_id;    }
    int  GetTypeId()  { return hazard_id; };

    void ComputeCollisionsPlayer(Character* player);
    void HazardStep(Character* player);
};

#endif // HAZARD_H