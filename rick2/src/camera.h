#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <list>

#include "world.h"
#include "character.h"
#include "platform.h"
#include "block.h"

using namespace std;

class Camera
{
  private:
    int pos_x;
    int pos_y;
    int pixels_width;
    int pixels_height;
    int tiles_width;
    int tiles_height;

    int tile_width;
    int tile_height;

    int world_width;
    int world_height;

    ALLEGRO_BITMAP* camera_bitmap;
    ALLEGRO_BITMAP* screen;
    World*          map;

  public:
    Camera();
    ~Camera();

    int GetPosX()         { return pos_x; }
    int GetPosY()         { return pos_y; }
    int GetPixelsWidth()  { return pixels_width; }
    int GetPixelsHeight() { return pixels_height; }
    int GetTilesWidth()   { return tiles_width; }
    int GetTilesHeight()  { return tiles_height; }    

    void SetPosX(int _pos_x);
    void SetPosY(int _pos_y);
    void SetPixelsWidth(int _pixels_width);
    void SetPixelsHeight(int _pixels_height);
    void SetTilesWidth(int _tiles_width);
    void SetTilesHeight(int _tiles_height);

    ALLEGRO_BITMAP* GetScreen() { return screen; }
    void SetScreen(ALLEGRO_BITMAP* _screen) { screen = _screen; }

    void InitCamera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, World* _map, ALLEGRO_BITMAP* _screen);
    void PositionBasedOnPlayer(Character* player);

    void SetMap(World* _map) { map = _map; }

    void DrawScreen(World* world, Character *player);

  private:
    bool CoordsWithinCamera(int x, int y);

};

#endif // CAMERA_H