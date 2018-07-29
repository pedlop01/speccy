#ifndef TRIGGER_H
#define TRIGGER_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "character.h"
#include "object.h"

using namespace std;

#define ACTION_EVENT_ENTERS 0
#define ACTION_EVENT_STAYS  1
#define ACTION_EVENT_EXITS  2
#define ACTION_EVENT_HITS   3

#define ACTION_FACE_ANY   0
#define ACTION_FACE_RIGHT 1
#define ACTION_FACE_LEFT  2

class Trigger {
  private:
    int id;

    // Trigger area
    int x;
    int y;
    int width;
    int height;

    int action_event;
    int action_face;

    bool onehot;    

    // List of next checkpoints from the current one
    vector<Object*> targets;
    // Associated delays for target
    vector<int> targets_delay;
    vector<bool> targets_triggered;

    bool player_was_in_trigger;
    bool already_triggered;
    bool trigger_targets;

    int steps;

  public:
    Trigger(int _id,
            int _x, int _y, int _width, int _height,
            int _action_event, int _action_face, int _onehot);
    ~Trigger();

    void AddTarget(Object* _object, int _delay);

    // Read methods
    int GetId()     { return id;     }
    int GetX()      { return x;      }
    int GetY()      { return y;      }
    int GetWidth()  { return width;  }
    int GetHeight() { return height; }

    int GetActionEvent() { return action_event; }
    int GetActionFace()  { return action_face;  }

    vector<Object*>* GetTargets()      { return &targets;       }
    vector<int>*     GetTargetsDelay() { return &targets_delay; }

    bool InTrigger(int x, int y, int width, int height);

    void TriggerStep(int _x, int _y, int _width, int _height,
                     int _face, int _state);

};

#endif // TRIGGER_H