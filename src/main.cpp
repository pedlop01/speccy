#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <allegro5/allegro.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

unsigned long GetTickCount()
{
  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0)
    return 0;

  return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

int main(int argc, char *argv[]) {

  ALLEGRO_DISPLAY* display = NULL;
  ALLEGRO_BITMAP* bitmap = NULL;
  ALLEGRO_EVENT_QUEUE* event_queue = NULL;

  // Components
  Z80 cpu;
  ULA ula;
  Bus16 bus;
  ROM<0, 16384> rom;
  RAM<32768, 32*1024> ram;

  if(!al_init()) {
    printf("Error: failed to initialize allegro!\n");
    return -1;
  }

  if(!al_install_keyboard()) {
    printf("Error: failed to initialize keyboard!\n");
    return -1;
  }

  if(!al_init_primitives_addon()) {
    printf("Error: failed to initialize allegro primitives!\n");
    return -1;
  }

  display = al_create_display(320, 240);
  if(!display) {
    printf("Error: failed to create display!\n");
    return -1;
  }

  bitmap = al_create_bitmap(320, 240);
  if(!bitmap) {
    printf("Error: failed to create bitmap!\n");
    al_destroy_display(display);
    return -1;
  }

  event_queue = al_create_event_queue();
  if(!event_queue) {
    printf("Error: failted to create event_queue!\n");
    al_destroy_bitmap(bitmap);
    al_destroy_display(display);
    return -1;
  }

  // Load ROM contents
  if(rom.Load("48.rom")) {
    printf("Error: Unable to load rom\n");
    return(-1);
  }

  al_register_event_source(event_queue, al_get_keyboard_event_source());

  al_set_target_bitmap(bitmap);
  al_clear_to_color(al_map_rgb(0xD7, 0xD7, 0xD7));
  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_bitmap(bitmap, 0, 0, 0);
  al_flip_display();
  
  // Populate busses
  bus.AddBusComponent(&rom);
  bus.AddBusComponent(&ram);
  bus.AddBusComponent((ULAMemory*)&ula);

  // And attach busses to cpu core
  cpu.DataBus = &bus;
  cpu.IOBus = (BusComponent<0xFE, 1>*)((ULAIO*)&ula);

  // Add bitmap to ULA
  ula.SetBitmap(bitmap);

  cpu.regs.PC = 0;

  al_set_target_bitmap(bitmap);

  unsigned long dwFrameStartTime = GetTickCount();

  // Main loop
  do {
    cpu.tStates = 0;

    // Keyboard routine. Needs to be moved elsewhere
    if(!al_is_event_queue_empty(event_queue)) {
      bool keyEvent = false;
      bool keyDown  = false;
      ALLEGRO_EVENT ev;
      al_wait_for_event(event_queue, &ev);

      if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
        keyEvent = true;
        keyDown  = true;
      } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
        keyEvent = true;
      }

      if (keyEvent) {
        switch (ev.keyboard.keycode) {
          // Row 0
          case ALLEGRO_KEY_0: ula.PressKey(0, 9, keyDown); break;
          case ALLEGRO_KEY_1: ula.PressKey(0, 0, keyDown); break;
          case ALLEGRO_KEY_2: ula.PressKey(0, 1, keyDown); break;
          case ALLEGRO_KEY_3: ula.PressKey(0, 2, keyDown); break;
          case ALLEGRO_KEY_4: ula.PressKey(0, 3, keyDown); break;
          case ALLEGRO_KEY_5: ula.PressKey(0, 4, keyDown); break;
          case ALLEGRO_KEY_6: ula.PressKey(0, 5, keyDown); break;
          case ALLEGRO_KEY_7: ula.PressKey(0, 6, keyDown); break;
          case ALLEGRO_KEY_8: ula.PressKey(0, 7, keyDown); break;
          case ALLEGRO_KEY_9: ula.PressKey(0, 8, keyDown); break;
          // Row 1
          case ALLEGRO_KEY_Q: ula.PressKey(1, 0, keyDown); break;
          case ALLEGRO_KEY_W: ula.PressKey(1, 1, keyDown); break;
          case ALLEGRO_KEY_E: ula.PressKey(1, 2, keyDown); break;
          case ALLEGRO_KEY_R: ula.PressKey(1, 3, keyDown); break;
          case ALLEGRO_KEY_T: ula.PressKey(1, 4, keyDown); break;
          case ALLEGRO_KEY_Y: ula.PressKey(1, 5, keyDown); break;
          case ALLEGRO_KEY_U: ula.PressKey(1, 6, keyDown); break;
          case ALLEGRO_KEY_I: ula.PressKey(1, 7, keyDown); break;
          case ALLEGRO_KEY_O: ula.PressKey(1, 8, keyDown); break;
          case ALLEGRO_KEY_P: ula.PressKey(1, 9, keyDown); break;
          // Row 2
          case ALLEGRO_KEY_A: ula.PressKey(2, 0, keyDown); break;
          case ALLEGRO_KEY_S: ula.PressKey(2, 1, keyDown); break;
          case ALLEGRO_KEY_D: ula.PressKey(2, 2, keyDown); break;
          case ALLEGRO_KEY_F: ula.PressKey(2, 3, keyDown); break;
          case ALLEGRO_KEY_G: ula.PressKey(2, 4, keyDown); break;
          case ALLEGRO_KEY_H: ula.PressKey(2, 5, keyDown); break;
          case ALLEGRO_KEY_J: ula.PressKey(2, 6, keyDown); break;
          case ALLEGRO_KEY_K: ula.PressKey(2, 7, keyDown); break;
          case ALLEGRO_KEY_L: ula.PressKey(2, 8, keyDown); break;
          case ALLEGRO_KEY_ENTER: ula.PressKey(2, 9, keyDown); break;
          // Row 2
          case ALLEGRO_KEY_LSHIFT: ula.PressKey(3, 0, keyDown); break;
          case ALLEGRO_KEY_Z: ula.PressKey(3, 1, keyDown); break;
          case ALLEGRO_KEY_X: ula.PressKey(3, 2, keyDown); break;
          case ALLEGRO_KEY_C: ula.PressKey(3, 3, keyDown); break;
          case ALLEGRO_KEY_V: ula.PressKey(3, 4, keyDown); break;
          case ALLEGRO_KEY_B: ula.PressKey(3, 5, keyDown); break;
          case ALLEGRO_KEY_N: ula.PressKey(3, 6, keyDown); break;
          case ALLEGRO_KEY_M: ula.PressKey(3, 7, keyDown); break;
          case ALLEGRO_KEY_RSHIFT: ula.PressKey(3, 8, keyDown); break;
          case ALLEGRO_KEY_SPACE: ula.PressKey(3, 9, keyDown); break;
        }
      }

    }

    // Emulate instructions
    cpu.EmulateOne();

    bool irq = false;
    ula.AddCycles(cpu.tStates, irq);

    // 20ms has passed. Vertical screen sync triggered
    if (irq) {
      cpu.INT();

      al_set_target_bitmap(al_get_backbuffer(display));
      al_draw_bitmap(bitmap, 0, 0, 0);
      al_flip_display();
      al_set_target_bitmap(bitmap);

      // If our code is faster than 20ms (expected to be),
      // then wait for what is remaining.
      unsigned long dwNow = GetTickCount();
      unsigned long dwEllapsed = dwNow - dwFrameStartTime;
      if (dwEllapsed < 20) {
        usleep(20000 - dwEllapsed*1000);
        dwNow = GetTickCount();
      }
      dwFrameStartTime = dwNow;
    }

  } while(true);

  al_destroy_display(display);
}

