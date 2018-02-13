#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <allegro5/allegro.h>

#include <iostream>

#define KEY_LEFT  0x01
#define KEY_RIGHT 0x02
#define KEY_UP    0x04
#define KEY_DOWN  0x08
#define KEY_SPACE 0x10
#define KEY_A     0x20
#define KEY_Z     0x40

using namespace std;

class Keyboard
{
    private:
      int keys;

    public:
       Keyboard();
       ~Keyboard();

       int ReadKeyboard(ALLEGRO_EVENT_QUEUE *event_queue);

       int GetKeys();
       int PressedLeft();
       int PressedRight();
       int PressedUp();
       int PressedDown();
       int PressedSpace();
       int PressedA();
       int PressedZ();
};

#endif // KEYBOARD_H
