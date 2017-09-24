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

  al_clear_to_color(al_map_rgb(255,255,255));

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

  unsigned int x = 0;

  // Main loop
  do {
    // Emulate instructions
    cpu.EmulateOne();


    if((x%16192)==0) {
      al_set_target_bitmap(bitmap);

      al_clear_to_color(al_map_rgb(0,0,0));

      for(unsigned int i = 0x4000; i < 0x5800; i++) {
        unsigned int row = (i >> 5);
        row = ((row & 0xC0) |
               ((row & 0x38) >> 3) |
               ((row & 0x07) << 3));

        unsigned int column = (i % 32)*8;

        for(unsigned int j = 0 ; j < 8; j++)
          if(ula.MemRead(i) & (0x01 << (7-j))) {
            printf("x=%d, y=%d\n", column+j+32, row+24);
            al_put_pixel(column+j+32, row + 24, al_map_rgb(0x255, 0x255, 0x255));
          }

      }
      //printf("pintado\n");
  
      al_set_target_bitmap(al_get_backbuffer(display));
      al_draw_bitmap(bitmap, 0, 0, 0);
      al_flip_display();
    }
    x++;
  } while(true);

  al_destroy_display(display);
}

