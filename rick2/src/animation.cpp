#include "animation.h" // class's header file

// class constructor
Animation::Animation() {	
}

// class destructor
Animation::~Animation()
{
    for (list<sprite_ptr>::iterator it = sprites.begin() ; it != sprites.end() ; it++ ) {
        delete *it;
    }
    sprites.clear();
}
