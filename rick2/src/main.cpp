#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <fstream>
#include <vector>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>

#include "world.h"
#include "keyboard.h"
#include "camera.h"

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
  map_level1 = new World("../maps/level1/Map1_final.tmx", false);  
  camera.InitCamera(0, 0, 320, 240, map_level1, bitmap);

  // Main loop
  do {
    al_set_target_bitmap(bitmap);

    keyboard.ReadKeyboard(event_queue);

    camera.DrawScreen();

    if(keyboard.PressedRight()) { camera.SetPosX(camera.GetPosX()+1); }
    if(keyboard.PressedLeft())  { camera.SetPosX(camera.GetPosX()-1); }
    if(keyboard.PressedUp())    { camera.SetPosY(camera.GetPosY()-1); }
    if(keyboard.PressedDown())  { camera.SetPosY(camera.GetPosY()+1); }

    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 0, 640, 480, 0);
       
    al_flip_display();
  } while(true);

  al_destroy_display(display);
}

