#include "keyboard.h"

Keyboard::Keyboard() {
  keys = 0;
}

Keyboard::~Keyboard() {

}

int Keyboard::ReadKeyboard(ALLEGRO_EVENT_QUEUE *event_queue) {
  bool keyEvent = false;
  bool keyDown  = false;
  ALLEGRO_EVENT ev;
 
  if(al_is_event_queue_empty(event_queue)) {
    return keys;
  }

  al_wait_for_event(event_queue, &ev);

  if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
    keyEvent = true;
    keyDown  = true;
  } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
    keyEvent = true;
  }

  if (keyEvent) {
    switch (ev.keyboard.keycode) {
      case ALLEGRO_KEY_LEFT  : (keyDown ? keys |= KEY_LEFT  : keys &= ~KEY_LEFT);  break;
      case ALLEGRO_KEY_RIGHT : (keyDown ? keys |= KEY_RIGHT : keys &= ~KEY_RIGHT); break;
      case ALLEGRO_KEY_UP    : (keyDown ? keys |= KEY_UP    : keys &= ~KEY_UP);    break;
      case ALLEGRO_KEY_DOWN  : (keyDown ? keys |= KEY_DOWN  : keys &= ~KEY_DOWN);  break;
      case ALLEGRO_KEY_SPACE : (keyDown ? keys |= KEY_SPACE : keys &= ~KEY_SPACE); break;
    }
  }

  return keys;
}

int Keyboard::GetKeys() {
  return keys;
}

int Keyboard::PressedLeft() {
  return (keys && KEY_LEFT);
}

int Keyboard::PressedRight() {
  return (keys && KEY_RIGHT);
}

int Keyboard::PressedUp() {
  return (keys && KEY_UP);
}

int Keyboard::PressedDown() {
  return (keys && KEY_DOWN);
}

int Keyboard::PressedSpace() {
  return (keys && KEY_SPACE);
}