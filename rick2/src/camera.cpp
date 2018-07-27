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
  camera_bitmap = 0;
  screen = 0;
}

Camera::~Camera() {

}

void Camera::InitCamera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, World* _map, ALLEGRO_BITMAP* _screen) {
  map = _map;

  pixels_width = _pixels_width;
  pixels_height = _pixels_height;

  tiles_width = pixels_width / map->GetTilesetTileWidth();
  tiles_height = pixels_height / map->GetTilesetTileHeight();

  tile_width = pixels_width / tiles_width;
  tile_height = pixels_height / tiles_height;

  // REVISIT: Need to pass map
  world_width = map->GetMapWidth()*tile_width;
  world_height = map->GetMapHeight()*tile_height;

  // Using methods to set x and y because they control the limits of the camera in the world
  // These methods need to be call after initializing the other width parameters
  SetPosX(_pos_x);
  SetPosY(_pos_y);

  // If bitmap exists, then destroy it first
  al_destroy_bitmap(camera_bitmap);
  camera_bitmap = al_create_bitmap(pixels_width, pixels_height);

  // Finally, link the screen bitmap
  screen = _screen;
}

void Camera::PositionBasedOnPlayer(Character* player) {
  // Do not move the camera when DYING
  if (player->GetState() == RICK_STATE_DYING) return;
  SetPosX(player->GetCorrectedPosX() - pixels_width/2);
  SetPosY(player->GetCorrectedPosY() - pixels_height/2);
}

void Camera::SetPosX(int _pos_x) {
  if (_pos_x < 0) {
    pos_x = 0;
  } else if (_pos_x > (world_width - pixels_width - tile_width)) { // REVISIT
    pos_x = (world_width - pixels_width - tile_width);
  } else {
    pos_x = _pos_x;
}
}

void Camera::SetPosY(int _pos_y) {
  if (_pos_y < 0) {
    pos_y = 0;
  } else if (_pos_y > (world_height - pixels_height)) {
    pos_y = (world_height - pixels_height);
  } else {
    pos_y = _pos_y;
  }
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

void Camera::DrawBackTiles(World* world, Character* player, ALLEGRO_FONT *font) {
  Tile* tile;
  int left_up_x;
  int left_up_y;
  ALLEGRO_BITMAP* tileset_bitmap;

  // Compute x and y corrections to draw world
  int correct_x = pos_x % tile_width;
  int correct_y = pos_y % tile_height;

  // Get the tileset bitmap
  tileset_bitmap = map->GetWorldImage();

  // Check if it is needed to add an extra tile
  int tiles_width_corrected = (pos_x % tile_width) ? tiles_width + 1 : tiles_width;
  int tiles_height_corrected = (pos_y % tile_height) ? tiles_height + 1 : tiles_height;

  int tile_y = pos_y / tile_height;
  for (int y = 0; y < tiles_height_corrected; y++) {
    int tile_x = pos_x / tile_width;    
    for (int x = 0; x < tiles_width_corrected; x++) {      
      tile = map->GetTile(tile_x, tile_y);
      if (tile->GetValue() != 0) {
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
      tile_x++;
    }
    tile_y++;
  }

}

void Camera::DrawFrontTiles(World* world, Character* player, ALLEGRO_FONT *font) {
  Tile* tile;
  int left_up_x;
  int left_up_y;
  ALLEGRO_BITMAP* tileset_bitmap;

  // Compute x and y corrections to draw world
  int correct_x = pos_x % tile_width;
  int correct_y = pos_y % tile_height;

  // Get the tileset bitmap
  tileset_bitmap = map->GetWorldImage();

  // Check if it is needed to add an extra tile
  int tiles_width_corrected = (pos_x % tile_width) ? tiles_width + 1 : tiles_width;
  int tiles_height_corrected = (pos_y % tile_height) ? tiles_height + 1 : tiles_height;

  int tile_y = pos_y / tile_height;
  for (int y = 0; y < tiles_height_corrected; y++) {
    int tile_x = pos_x / tile_width;    
    for (int x = 0; x < tiles_width_corrected; x++) {
      tile = map->GetTileFront(tile_x, tile_y);
      if (tile->GetValue() != 0) {
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
      tile_x++;
    }
    tile_y++;
  }
}

void Camera::DrawBackObjects(World* world, Character* player, ALLEGRO_FONT *font) {
  list<Object*>* back_objects = world->GetBackObjects();
  for (list<Object*>::iterator it = back_objects->begin() ; it != back_objects->end(); ++it) {
    Object* object = *it;    
    if (object->GetState() != OBJ_STATE_DEAD) {
      // Only draw object in camera
      if (CoordsWithinCamera(object->GetX(),                      object->GetY()) ||
          CoordsWithinCamera(object->GetX() + object->GetWidth(), object->GetY()) ||
          CoordsWithinCamera(object->GetX(),                      object->GetY() + object->GetHeight()) ||
          CoordsWithinCamera(object->GetX() + object->GetWidth(), object->GetY() + object->GetHeight())) {
        ALLEGRO_BITMAP* object_sprite = object->GetCurrentAnimationBitmap();
        al_draw_bitmap(object_sprite,
                       object->GetX() - GetPosX(),
                       object->GetY() - GetPosY(),
                       object->GetCurrentAnimationBitmapAttributes());
      }
    }
  }
}

void Camera::DrawFrontObjects(World* world, Character* player, ALLEGRO_FONT *font) {
  // Draw objects
  list<Object*>* objects = world->GetObjects();
  for (list<Object*>::iterator it = objects->begin() ; it != objects->end(); ++it) {
    Object* object = *it;    
    if (object->GetVisible() && (object->GetState() != OBJ_STATE_DEAD)) {
      // Only draw object in camera
      if (CoordsWithinCamera(object->GetX(),                      object->GetY()) ||
          CoordsWithinCamera(object->GetX() + object->GetWidth(), object->GetY()) ||
          CoordsWithinCamera(object->GetX(),                      object->GetY() + object->GetHeight()) ||
          CoordsWithinCamera(object->GetX() + object->GetWidth(), object->GetY() + object->GetHeight())) {
        ALLEGRO_BITMAP* object_sprite = object->GetCurrentAnimationBitmap();
        al_draw_bitmap(object_sprite,
                       object->GetX() - GetPosX(),
                       object->GetY() - GetPosY(),
                       object->GetCurrentAnimationBitmapAttributes());
/*
        char buffer[30];
        sprintf(buffer, "%d", object->GetTypeId());
        al_draw_text(font,
                     al_map_rgb(255, 255, 0),
                     object->GetX() + object->GetBBX() - GetPosX(),
                     object->GetY() + object->GetBBY() - GetPosY(),
                     ALLEGRO_ALIGN_LEFT,
                     buffer);
        al_draw_rectangle(object->GetX() + object->GetBBX() - GetPosX() + 1,
                          object->GetY() + object->GetBBY() - GetPosY() + 1,
                          object->GetX() + object->GetBBX() + (object->GetBBWidth() - 1) - GetPosX() + 1,
                          object->GetY() + object->GetBBY() + (object->GetBBHeight() - 1) - GetPosY() + 1,
                          al_map_rgb(0xFF, 0xFF, 0xFF), 1.0);
        al_draw_rectangle(object->GetX() + object->GetBBX() - 1 - GetPosX() + 1,
                          object->GetY() + object->GetBBY() - 1 - GetPosY() + 1,
                          object->GetX() + object->GetBBX() + (object->GetBBWidth() - 1) + 1 - GetPosX() + 1,
                          object->GetY() + object->GetBBY() + (object->GetBBHeight() - 1) + 1 - GetPosY() + 1,
                          al_map_rgb(0xFF, 0x0F, 0x0F), 1.0);
*/
      }
    }
  }
}

void Camera::DrawPlayer(World* world, Character* player, ALLEGRO_FONT *font) {
  // DYING animation requires an special function to scale the sprite
  if ((player->GetState() == RICK_STATE_DEAD) || (player->GetState() == RICK_STATE_DYING))
    return;

  ALLEGRO_BITMAP* player_bitmap = player->GetCurrentAnimationBitmap();
  al_draw_bitmap(player_bitmap,
                 player->GetPosX() - GetPosX(),
                 player->GetPosY() - GetPosY(),
                 player->GetCurrentAnimationBitmapAttributes());
/*
  // Draw the player in front of back tiles
  al_draw_rectangle(player->GetPosX() - GetPosX() + 1,
                    player->GetPosY() - GetPosY() + 1,
                    player->GetPosX() + player->GetWidth() - 1 - GetPosX() + 1,
                    player->GetPosY() + player->GetHeight() - 1 - GetPosY() + 1,
                    al_map_rgb(0xAD, 0x21, 0x56), 1.0);
  // - External
  al_draw_rectangle(player->GetPosX() - 1 - GetPosX() + 1,
                    player->GetPosY() - 1 - GetPosY() + 1,
                    player->GetPosX() + player->GetWidth() - GetPosX() + 1,
                    player->GetPosY() + player->GetHeight() - GetPosY() + 1,
                    al_map_rgb(0xDF, 0xDF, 0xDF), 1.0);
  // - Bounding box
  al_draw_rectangle(player->GetPosX() + player->GetBBX() - GetPosX() + 1,
                    player->GetPosY() + player->GetBBY() - GetPosY() + 1,
                    player->GetPosX() + player->GetBBX() + player->GetBBWidth() - 1 - GetPosX() + 1,
                    player->GetPosY() + player->GetBBY() + player->GetBBHeight() - 1 - GetPosY() + 1,
                    al_map_rgb(0xAF, 0xAF, 0xAF), 1.0);
*/
}

void Camera::DrawPlayerDying(World* world, Character* player, ALLEGRO_FONT *font) {

  if ((player->GetState() != RICK_STATE_DYING))
    return;

  ALLEGRO_BITMAP* player_bitmap = player->GetCurrentAnimationBitmap();
  al_draw_scaled_bitmap(player_bitmap,
                        0,
                        0,
                        player->GetCurrentAnimationWidth(),
                        player->GetCurrentAnimationHeight(),
                        player->GetPosX() - GetPosX(),
                        player->GetPosY() - GetPosY(),
                        player->GetCurrentAnimationWidth()*player->GetCurrentAnimationScalingFactor(),
                        player->GetCurrentAnimationHeight()*player->GetCurrentAnimationScalingFactor(),
                        player->GetCurrentAnimationBitmapAttributes());
}

void Camera::DrawPlatforms(World* world, Character* player, ALLEGRO_FONT *font) {
  vector<Platform*>* platforms = world->GetPlatforms();
  for (vector<Platform*>::iterator it = platforms->begin() ; it != platforms->end(); ++it) {
    Platform* platform = *it;
    ALLEGRO_BITMAP* platform_sprite = platform->GetCurrentAnimationBitmap();
    al_draw_bitmap(platform_sprite,
                   platform->GetX() - GetPosX(),
                   platform->GetY() - GetPosY(),
                   platform->GetCurrentAnimationBitmapAttributes());
    char buffer[30];
    sprintf(buffer, "%d", platform->GetTypeId());
    al_draw_text(font,
                 al_map_rgb(255, 255, 0),
                 platform->GetX() - GetPosX(),
                 platform->GetY() - GetPosY(),
                 ALLEGRO_ALIGN_LEFT,
                 buffer);

  }
}

void Camera::DrawBlocks(World* world, Character* player, ALLEGRO_FONT *font) {
  list<Block*>* blocks = world->GetBlocks();
  for (list<Block*>::iterator it = blocks->begin() ; it != blocks->end(); ++it) {
    Block* block = *it;
    if (block->GetState() != OBJ_STATE_DEAD) {
      ALLEGRO_BITMAP* block_sprite = block->GetCurrentAnimationBitmap();
      al_draw_bitmap(block_sprite,
                     block->GetX() - GetPosX(),
                     block->GetY() - GetPosY(),
                     block->GetCurrentAnimationBitmapAttributes());
    }
  }
}

void Camera::DrawCheckpoints(World* world, Character* player, ALLEGRO_FONT *font) {
  list<Checkpoint*>* checkpoints = world->GetCheckpoints();
  for (list<Checkpoint*>::iterator it = checkpoints->begin(); it != checkpoints->end(); it++) {
    Checkpoint* checkpoint = *it;
    al_draw_rectangle(checkpoint->GetChkX() - GetPosX() + 1,
                      checkpoint->GetChkY() - GetPosY() + 1,
                      checkpoint->GetChkX() + checkpoint->GetChkWidth() - GetPosX() + 1,
                      checkpoint->GetChkY() + checkpoint->GetChkHeight() - GetPosY() + 1,
                      al_map_rgb(0xFF, 0xFF, 0xFF), 1.0);
  }
  // Now draw the current checkpoint
  al_draw_rectangle(world->GetCurrentCheckpoint()->GetChkX() - GetPosX() + 1,
                    world->GetCurrentCheckpoint()->GetChkY() - GetPosY() + 1,
                    world->GetCurrentCheckpoint()->GetChkX() + world->GetCurrentCheckpoint()->GetChkWidth() - GetPosX() + 1,
                    world->GetCurrentCheckpoint()->GetChkY() + world->GetCurrentCheckpoint()->GetChkHeight() - GetPosY() + 1,
                    al_map_rgb(0x0, 0xFF, 0xFF), 1.0);
}

void Camera::DrawScreen(World* world, Character* player, ALLEGRO_FONT *font) {

  // Draw everything on internal bitmap before resizing it
  // into the screen bitmap  
  al_set_target_bitmap(camera_bitmap);  
  al_clear_to_color(al_map_rgb(0, 0, 0));   // REVISIT: Drawing background as black. This helps with transparent tiles drawing  
    
  // Traverse map and draw background tiles in the screen
  this->DrawBackTiles(map, player, font);
  // Draw back objects
  this->DrawBackObjects(map, player, font);
  // Draw the player if he is not dying
  this->DrawPlayer(map, player, font);
  // Draw resting objects
  this->DrawFrontObjects(map, player, font);
  // Draw platforms
  this->DrawPlatforms(map, player, font);
  // Traverse map and draw front tiles in the screen
  this->DrawFrontTiles(map, player, font);
  // Draw blocks
  this->DrawBlocks(map, player, font);
  // Draw checkpoints (only for debug)
  this->DrawCheckpoints(map, player, font);
  // Draw player dying if required
  this->DrawPlayerDying(map, player, font);

  // Move camera to screen
  al_set_target_bitmap(screen);
  al_draw_scaled_bitmap(camera_bitmap,
                        0, 0, pixels_width, pixels_height,
                        0, 0, 320, 240, 0);
}

bool Camera::CoordsWithinCamera(int x, int y) {
  return ((x >= pos_x) &&
          (x <= pos_x + pixels_width) &&
          (y >= pos_y) &&
          (y <= pos_y + pixels_height));
}
