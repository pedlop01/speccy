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
  ALLEGRO_BITMAP*  bitmap = NULL;

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

  // Load ROM contents
  if(rom.Load("48.rom")) {
    printf("Error: Unable to load rom\n");
    return(-1);
  }

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
  cpu.IOBus = (BusComponent<0, 0x10000>*)((ULAIO*)&ula);

  // Add bitmap to ULA
  ula.SetBitmap(bitmap);

  cpu.regs.PC = 0;

  al_set_target_bitmap(bitmap);

  unsigned long dwFrameStartTime = GetTickCount();

  // Main loop
  do {
    cpu.tStates = 0;

    // Emulate instructions
    cpu.EmulateOne();

    bool irq = false;
    ula.AddCycles(cpu.tStates, irq);

    // 20ms has passed. Vertical screen sync triggered
    if (irq) {
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

