#include <stdio.h>
#include <allegro5/allegro.h>

#include "z80.h"
#include "bus.hpp"
#include "ram.hpp"
#include "rom.hpp"
#include "ula.h"

int main(int argc, char *argv[]) {

  ALLEGRO_DISPLAY* display = NULL;
  ALLEGRO_BITMAP* bitmap = NULL;

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

  al_clear_to_color(al_map_rgb(255,50,50));

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

  cpu.regs.PC = 0;

  // Main loop
  do {
    // Emulate instructions
    cpu.EmulateOne();

    for(unsigned int i = 0x4000; i < 0x57FF; i++) {
      printf("%c", ula.Read(i));
      if((i%256) == 0) printf("\n");
    }
    printf("\n");

  } while(true);

  al_destroy_display(display);
}

