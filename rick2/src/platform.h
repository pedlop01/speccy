#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "rick_params.h"
#include "object.h"

#define PLATFORM_DIR_STOP  0b0000
#define PLATFORM_DIR_RIGHT 0b0001
#define PLATFORM_DIR_LEFT  0b0010
#define PLATFORM_DIR_UP    0b0100
#define PLATFORM_DIR_DOWN  0b1000

class Platform : public Object {
  private:
    bool moving;
    int  start_x;
    int  start_y;
    int  direction;
    int  displacement;

  public:
    Platform();
    Platform(int _x, int _y, int _width, int _height, bool _visible, bool _active, int _dir, int _displacement);
    ~Platform();

    void PlatformStep();

};

#endif // PLATFORM_H