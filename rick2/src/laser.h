#ifndef LASER_H
#define LASER_H

#include <stdio.h>
#include <stdlib.h>

#include "rick_params.h"
#include "object.h"

#define LASER_TYPE_RECURSIVE 0
#define LASER_TYPE_ONESHOT   1

class Laser : public Object {
  private:
    int start_x;
    int start_y;
    int laser_type;

  private:
    void UpdateFSMState(World* map);

  public:
    Laser();
    Laser(const char* file, int _x, int _y, int _width, int _height, int _type, int _speed, int _direction);
    ~Laser();

    int GetTypeId() { return obj_id; };
};

#endif // LASER_H