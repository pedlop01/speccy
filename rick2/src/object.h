#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "pugixml.hpp"
#include "animation.h"
#include "colbox.h"

// Object types
#define OBJ_NONE         0
#define OBJ_PLATFORM     1
#define OBJ_ITEM         2
#define OBJ_LASER        3
#define OBJ_BOMB         4
#define OBJ_HAZARD       5
#define OBJ_BLOCK        6

// Object states
#define OBJ_STATE_STOP   0
#define OBJ_STATE_MOVING 1
#define OBJ_STATE_DYING  2
#define OBJ_STATE_DEAD   3

// Object directions
#define OBJ_DIR_STOP  0b0000
#define OBJ_DIR_RIGHT 0b0001
#define OBJ_DIR_LEFT  0b0010
#define OBJ_DIR_UP    0b0100
#define OBJ_DIR_DOWN  0b1000

// Forward declaration
class World;
class Character;

class Object {
  protected:
    static int id;

    int x;
    int y;
    int width;
    int height;

    int obj_id;

    bool visible;
    bool active;

    // State variables
    int prev_state;
    int state;
    int steps_in_state;

    // Movement variables
    int   direction;
    int   prev_direction;
    int   face;
    float speed_x;
    float speed_x_step;
    float speed_x_max;
    float speed_x_min;
    float speed_y;
    float speed_y_max;
    float speed_y_min;
    float speed_y_step;
    int   steps_in_direction_x;
    int   steps_in_direction_y;    

    int obj_type;

    // Collisions
    Colbox extColExt;  // Collision with world
    bool playerCol;

    vector<Animation*> animations;

    pugi::xml_document obj_file;

  public:

    Object();
    Object(int _x, int _y, int _width, int _height, int _visible, int _active);
    ~Object();

    int GetType()  { return obj_type; }
    int GetState() { return state;    }

    void Init(const char* file,
              int _x,
              int _y,
              int _width,
              int _height,
              bool _visible,
              bool _active,
              int _state,
              int _direction,
              float _speed_x_step,
              float _speed_x_max,
              float _speed_x_min,
              float _speed_y_step,
              float _speed_y_max,
              float _speed_y_min);

    void SetX(int _x)              { x = _x;              };
    void SetY(int _y)              { y = _y;              };
    void SetWidth(int _width)      { width = _width;      };
    void SetHeight(int _height)    { height = _height;    };
    void SetVisible(bool _visible) { visible = _visible;  };
    void SetActive(bool _active)   { active = _active;    };

    // Differ from previous than this ones take into account the world collisions
    void SetX(World* map, int _x);
    void SetY(World* map, int _y);

    int GetId()       { return obj_id;      }
    int GetX()        { return x;       };
    int GetY()        { return y;       };
    int GetWidth()    { return width;   };
    int GetHeight()   { return height;  };
    bool GetVisible() { return visible; };
    bool GetActive()  { return active;  };

    void SetSpeedX(float _speed_x)          { speed_x = _speed_x;           };
    void SetSpeedXMax(float _speed_x_max)   { speed_x_max = _speed_x_max;   };
    void SetSpeedXMin(float _speed_x_min)   { speed_x_min = _speed_x_min;   };
    void SetSpeedXStep(float _speed_x_step) { speed_x_step = _speed_x_step; };
    void SetSpeedY(float _speed_y)          { speed_y = _speed_y;           };
    void SetSpeedYMax(float _speed_y_max)   { speed_y_max = _speed_y_max;   };
    void SetSpeedYMin(float _speed_y_min)   { speed_y_min = _speed_y_min;   };
    void SetSpeedYStep(float _speed_y_step) { speed_y_step = _speed_y_step; };
    void SetSpeeds(float _speed_x_max,
                   float _speed_x_min,
                   float _speed_x_step,                   
                   float _speed_y_max,
                   float _speed_y_min,
                   float _speed_y_step)     { speed_x_max = _speed_x_max;
                                              speed_x_min = _speed_x_min;
                                              speed_x_step = _speed_x_step;
                                              speed_y_max = _speed_y_max;
                                              speed_y_min = _speed_y_min;
                                              speed_y_step = _speed_y_step; };

    void ComputeCollisions(World* map, Character* player);
    void ComputeNextState();
    void ComputeNextPosition(World* map);
    void ComputeNextSpeed();

    void ObjectStep(World* map, Character* player);

    Animation*      GetCurrentAnimation();
    ALLEGRO_BITMAP* GetCurrentAnimationBitmap();
    int             GetCurrentAnimationBitmapAttributes();

  protected:
    virtual void UpdateFSMState();

  private:
    void GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int width, int height);
    void GetCollisionsExternalBoxExt(World* map, Colbox &mask_col);
};

#endif // OBJECT_H