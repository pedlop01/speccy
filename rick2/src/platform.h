#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "rick_params.h"
#include "object.h"

class Platform : public Object {
  private:
    bool moving;
    int  start_x;
    int  start_y;
    int  displacement;

  public:
    Platform();
    Platform(const char* file, int _x, int _y, int _width, int _height, bool _visible, bool _active, int _dir, int _displacement);
    ~Platform();

    void PlatformStep();

    int  GetDirection() { return direction; }

};

#endif // PLATFORM_H