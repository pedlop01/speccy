#include "camera.h"

Camera::Camera() {
  pos_x = 0;
  pos_y = 0;
  pixels_width = 0;
  pixels_height = 0;
  tiles_width = 0;
  tiles_height = 0;
  screen = nullptr;
}

Camera::Camera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen) {
  pos_x = _pos_x;
  pos_y = _pos_y;
  pixels_width = _pixels_width;
  pixels_height = _pixels_height;
  tiles_width = _tiles_width;
  tiles_height = _tiles_height;
  screen = _screen;
}

Camera::~Camera() {

}

void Camera::InitCamera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen) {
  pos_x = 7000;//_pos_x;
  pos_y = _pos_y;
  pixels_width = _pixels_width;
  pixels_height = _pixels_height;
  tiles_width = _tiles_width;
  tiles_height = _tiles_height;
  screen = _screen;
}

void Camera::DrawScreen() {
  // Traverse map and draw intro screen  
  int tile_y = pos_y / map->GetMapHeight();  
  for (int y = 0; y < tiles_height; y++) {
    int tile_x = pos_x / map->GetMapWidth();
    for (int x = 0; x < tiles_width; x++) {
      tile_x++;
      // REVISIT pedro: optimize and remove hard-coded variables.
      al_draw_bitmap_region(map->GetWorldImage(),
                            map->GetTile(tile_x, tile_y)->GetLeftUpX(),
                            map->GetTile(tile_x, tile_y)->GetLeftUpY(),
                            map->GetTile(tile_x, tile_y)->GetRightDownX() - map->GetTile(tile_x, tile_y)->GetLeftUpX(),
                            map->GetTile(tile_x, tile_y)->GetRightDownY() - map->GetTile(tile_x, tile_y)->GetLeftUpY(),
                            x*8,
                            y*8,
                            0);
    }
    tile_y++;    
  }
}