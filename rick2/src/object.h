#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "pugixml.hpp"

class Object {
  protected:
    int x;
    int y;
    int width;
    int height;

    bool visible;
    bool active;

  public:
    Object();
    Object(int _x, int _y, int _width, int _height, bool _visible, bool _active);
    ~Object();

    void SetX(int _x)              { x = _x;              };
    void SetY(int _y)              { y = _y;              };
    void SetWidth(int _width)      { width = _width;      };
    void SetHeight(int _height)    { height = _height;    };
    void SetVisible(bool _visible) { visible = _visible;  };
    void SetActive(bool _active)   { active = _active;    };

    int GetX()        { return x;       };
    int GetY()        { return y;       };
    int GetWidth()    { return width;   };
    int GetHeight()   { return height;  };
    bool GetVisible() { return visible; };
    bool GetActive()  { return active;  };
};

#endif // OBJECT_H