#ifndef CAMERA_H
#define CAMERA_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <iostream>

#include "world.h"

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

    ALLEGRO_BITMAP* screen;
    World*          map;

  public:
    Camera();
    Camera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen);
    ~Camera();

    int GetPosX()         { return pos_x; }
    int GetPosY()         { return pos_y; }
    int GetPixelsWidth()  { return pixels_width; }
    int GetPixelsHeight() { return pixels_height; }
    int GetTilesWidth()   { return tiles_width; }
    int GetTilesHeight()  { return tiles_height; }    

    void SetPosX(int _pos_x)                 { pos_x = _pos_x; }
    void SetPosY(int _pos_y)                 { pos_y = _pos_y; }
    void SetPixelsWidth(int _pixels_width)   { pixels_width = _pixels_width; }
    void SetPixelsHeight(int _pixels_height) { pixels_height = _pixels_height; }
    void SetTilesWidth(int _tiles_width)     { tiles_width = _tiles_width; }
    void SetTilesHeight(int _tiles_height)   { tiles_height = _tiles_height; }

    ALLEGRO_BITMAP* GetScreen() { return screen; }
    void SetScreen(ALLEGRO_BITMAP* _screen) { screen = _screen; }

    void InitCamera(int _pos_x, int _pos_y, int _pixels_width, int _pixels_height, int _tiles_width, int _tiles_height, ALLEGRO_BITMAP* _screen);

    void SetMap(World* _map) { map = _map; }

    void DrawScreen();
};

#endif // CAMERA_H