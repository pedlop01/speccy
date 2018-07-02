#include <windows.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include <vector>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include "world.h"
#include "keyboard.h"
#include "camera.h"
#include "character.h"
#include "timer.h"
#include "colbox.h"

using namespace std;

int main(int argc, char *argv[]) {
  // Allegro variables
  ALLEGRO_DISPLAY*       display     = NULL;
  ALLEGRO_BITMAP*        bitmap      = NULL;
  ALLEGRO_EVENT_QUEUE*   event_queue = NULL;
  ALLEGRO_SAMPLE*        sample      = NULL;  
  World*                 map_level1;
  Keyboard               keyboard;
  Camera                 camera;
  Character              player;
  Timer                  timer;
  // REVISIT: not sure if collision will be in main loop
  Colbox ext_collisions;
  Colbox int_width_collisions;
  Colbox int_height_collisions;

  // Check arguments
  if(argc != 1) {
    printf("Error: wrong parameters. Usage: XXXX\n");
    exit(-1);
  }

  // allegro initializations
  if(!al_init()) {
    printf("Error: failed to initialize allegro!\n");
    return -1;
  }

  if(!al_install_keyboard()) {
    printf("Error: failed to initialize keyboard!\n");
    return -1;
  }

  al_set_new_display_flags(ALLEGRO_WINDOWED);
  display = al_create_display(640, 480);
  if(!display) {
    printf("Error: failed to create display!\n");
    return -1;
  }

  if(!al_init_image_addon()) {
    printf("Error: failed to load image addon!\n");
    return -1;
  }

  bitmap = al_create_bitmap(320, 240);
  if(!bitmap) {
    printf("Error: failed to create bitmap!\n");
    al_destroy_display(display);
    return -1;
  }

  if(!al_init_primitives_addon()) {
    printf("Error: failed to initialize allegro primitives!\n");
    return -1;
  }

  al_set_target_bitmap(bitmap);
  al_clear_to_color(al_map_rgb(0xD7, 0xD7, 0xD7));
  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_bitmap(bitmap, 0, 0, 0);
  al_flip_display();

  event_queue = al_create_event_queue();
  if(!event_queue) {
    printf("Error: failted to create event_queue!\n");
    return -1;
  }

  al_register_event_source(event_queue, al_get_keyboard_event_source());

  // Game initializations
  map_level1 = new World("../maps/level1/Map1_prueba.tmx", false);
  camera.InitCamera(0, 0, 320, 240, map_level1, bitmap);

  // Start counter for first iteration
  timer.StartCounter();

  // Main loop
  do {
    al_set_target_bitmap(bitmap);

    keyboard.ReadKeyboard(event_queue);

    // REVISIT: this will be internal to the player
    player.GetCollisionsExternalBoxExt(map_level1, ext_collisions);
    player.GetCollisionsInternalWidthBoxExt(map_level1, int_width_collisions);
    player.GetCollisionsInternalHeightBoxExt(map_level1, int_height_collisions);
/*    printf("[Collisions ext] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      ext_collisions.GetLeftUpCol(),
      ext_collisions.GetRightUpCol(),
      ext_collisions.GetRightDownCol(),
      ext_collisions.GetLeftDownCol());
    printf("[Collisions int width] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      int_width_collisions.GetLeftUpCol(),
      int_width_collisions.GetRightUpCol(),
      int_width_collisions.GetRightDownCol(),
      int_width_collisions.GetLeftDownCol());
    printf("[Collisions int height] lup=%d, rup=%d, rdw=%d, ldw=%d\n",
      int_height_collisions.GetLeftUpCol(),
      int_height_collisions.GetRightUpCol(),
      int_height_collisions.GetRightDownCol(),
      int_height_collisions.GetLeftDownCol());*/

    if(keyboard.PressedESC())   { return 0; }

    player.ComputeNextState(ext_collisions, int_height_collisions, int_width_collisions, keyboard);
    player.ComputeNextPosition(map_level1, ext_collisions, int_height_collisions, int_width_collisions);

    camera.PositionBasedOnPlayer(&player);
    camera.DrawScreen(&player);

    // Check counter value for adding waiting time
    double delay = ((double)timer.GetCounter());
    if(delay < 20)
      Sleep(20 - delay);

    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 0, 640, 480, 0);
    al_flip_display();

    // Start counter again for next iteration
    timer.StartCounter();
  } while(true);

  al_destroy_display(display);
}

