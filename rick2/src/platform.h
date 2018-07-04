#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "object.h"

class Platform : Object {
  private:
    bool moving;

  public:
    Platform();
    Platform(int _x, int _y, int _width, int _height, bool _visible, bool _active);
    ~Platform();

};

#endif // PLATFORM_H