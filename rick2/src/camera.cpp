#include "camera.h"

Camera::Camera() {
  pos_x = 0;
  pos_y = 0;
  pixels_width = 0;
  pixels_height = 0;
  tiles_width = 0;
  tiles_height = 0;
  tile_width = 0;
  tile_height = 0;
  world_width = 0;
  world_height = 0;
  screen = nullptr;
}

Camera::Camera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen) {
  pos_x = _pos_x;
  pos_y = _pos_y;
  pixels_width = _pixels_width;
  pixels_height = _pixels_height;
  tiles_width = _tiles_width;
  tiles_height = _tiles_height;
  tile_width = pixels_width / tiles_width;
  tile_height = pixels_height / tiles_height;
  // REVISIT: need to initialize world_width and world_height
  screen = _screen;
}

Camera::~Camera() {

}

void Camera::InitCamera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen) {
  pos_x = 496;//_pos_x;
  pos_y = _pos_y;
  pixels_width = _pixels_width;
  pixels_height = _pixels_height;
  tiles_width = _tiles_width;
  tiles_height = _tiles_height;
  tile_width = pixels_width / tiles_width;
  tile_height = pixels_height / tiles_height;
  // REVISIT: Need to pass map
  world_width = map->GetMapWidth()*tile_width;
  world_height = map->GetMapHeight()*tile_height;

  screen = _screen;
}

void Camera::SetPosX(int _pos_x) {
  if ((_pos_x < 0) || (_pos_x >= (world_width - pixels_width - tile_width)) ) // REVISIT
    return;

  pos_x = _pos_x;
}

void Camera::SetPosY(int _pos_y) {
  if ((_pos_y < 0) || (_pos_y >= (world_height - pixels_height)) )
    return;

  pos_y = _pos_y;
}

void Camera::SetPixelsWidth(int _pixels_width) {
  pixels_width = _pixels_width;
}

void Camera::SetPixelsHeight(int _pixels_height) {
  pixels_height = _pixels_height;
}

void Camera::SetTilesWidth(int _tiles_width) {
  tiles_width = _tiles_width;
}

void Camera::SetTilesHeight(int _tiles_height) {
  tiles_height = _tiles_height;
}

void Camera::DrawScreen() {
  Tile* tile;
  int left_up_x;
  int left_up_y;
  ALLEGRO_BITMAP* tileset_bitmap;

  // Get the tileset bitmap
  tileset_bitmap = map->GetWorldImage();

  // Compute x and y corrections to draw world
  int correct_x = pos_x % tile_width;
  int correct_y = pos_y % tile_height;

  // Traverse map and draw intro screen  
  int tile_y = pos_y / tile_height;
  for (int y = 0; y < tiles_height + 1; y++) {
    int tile_x = pos_x / tile_width;
    for (int x = 0; x < tiles_width + 1; x++) {      
      tile_x++;
      tile = map->GetTile(tile_x, tile_y);
      left_up_x = tile->GetLeftUpX();
      left_up_y = tile->GetLeftUpY();

      int dest_x = x*tile_width - correct_x;
      int dest_y = y*tile_height - correct_y;

      if ((dest_x < world_width) && (dest_y < world_height)) {
        al_draw_bitmap_region(tileset_bitmap,
                              left_up_x,
                              left_up_y,
                              tile_width,
                              tile_height,
                              dest_x,
                              dest_y,
                              0);
      }
    }
    tile_y++;    
  }
}