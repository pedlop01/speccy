#include "object.h"
#include "world.h"

int Object::id = 0;

Object::Object() {
  x       = 0;
  y       = 0;
  width   = 0;
  height  = 0;
  visible = false;
  active  = false;

  steps_in_state = 0;
  steps_in_direction_x = 0;
  steps_in_direction_y = 0;

  id++;
}

Object::Object(int _x, int _y, int _width, int _height, bool _visible, bool _active) {
  x       = _x;
  y       = _y;
  width   = _width;
  height  = _height;
  visible = _visible;
  active  = _active;

  steps_in_state = 0;
  steps_in_direction_x = 0;
  steps_in_direction_y = 0;

  id++;
}

Object::~Object() {
}

void Object::SetX(World* map, int _x) {

  int tile_width = map->GetTilesetTileWidth();
  int tile_height = map->GetTilesetTileHeight();
  int obj_width = this->width;
  int obj_height = this->height;

  // Compute the displacement in x
  int desp_x = (_x > x) ? (_x - x) : (x - _x);

  // Compute x and y corrections to draw world
  int tile_col_x;
  int tile_col_up_y   = y / tile_height;
  int tile_col_down_y = (y + obj_height - 1) / tile_height;
  if ( _x > x) {
    // Collision moving right
    tile_col_x = (x + desp_x + obj_width)  / tile_width;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != TILE_COL)) {
      // No collision
      x = x + desp_x;
    } else {
      // Collision. Move to safe position
      int correction = (x + desp_x + obj_width) % tile_width;
      x = x + desp_x - correction;
    }
  } else if ((_x > 0) && (x < x)) {
    // Collision moving left
    tile_col_x = (x - desp_x) / tile_width;
    if ((map->GetTile(tile_col_x, tile_col_up_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_x, tile_col_down_y)->GetType() != TILE_COL)) {
      // No collision
      x = x - desp_x;
    } else {
      // Collision. Move to safe position
      int correction = tile_width - ((x - desp_x) % tile_width);
      x = x - desp_x + correction;
    }
  }
}

void Object::SetY(World* map, int _y) {
  int tile_width = map->GetTilesetTileWidth();
  int tile_height = map->GetTilesetTileHeight();
  int obj_width = this->width;
  int obj_height = this->height;

  // Compute the displacement in x
  int desp_y = (_y > y) ? (_y - y) : (y - _y);

  // Compute x and y corrections to draw world
  int tile_col_y;
  int tile_col_up_x   = x / tile_width;
  int tile_col_down_x = (x + obj_width - 1) / tile_width;
  if (_y > y) {
    // Collision moving down
    tile_col_y = (y + desp_y + obj_height)  / tile_height;
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) {
      // No collision
      y = y + desp_y;
    } else {
      // Collision. Move to safe position
      int correction = (y + desp_y + obj_height) % tile_height;
      y = y + desp_y - correction;
    }
  } else if ((_y > 0) && (_y < y)) {
    // Collision moving up
    tile_col_y = (y - desp_y) / tile_height;
    if ((map->GetTile(tile_col_up_x, tile_col_y)->GetType() != TILE_COL) &&
        (map->GetTile(tile_col_down_x, tile_col_y)->GetType() != TILE_COL)) {
      // No collision
      y = y - desp_y;
    } else {
      // Collision. Move to safe position
      int correction = tile_height - ((y - desp_y) % tile_height);
      y = y - desp_y + correction;
    }
  }
}

void Object::GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int width, int height) {
  mask_col.SetLeftUpCol(map->GetTileByCoord(left_up_x, left_up_y)->GetType());
  mask_col.SetRightUpCol(map->GetTileByCoord(left_up_x + width, left_up_y)->GetType());
  mask_col.SetRightDownCol(map->GetTileByCoord(left_up_x + width, left_up_y + height)->GetType());
  mask_col.SetLeftDownCol(map->GetTileByCoord(left_up_x, left_up_y + height)->GetType());
}

void Object::GetCollisionsExternalBoxExt(World* map, Colbox &mask_col) {
  this->GetCollisionsByCoords(map,
                              mask_col,
                              x - 1,
                              y - 1,
                              width + 1,
                              height + 1);
}

void Object::ComputeCollisions(World* map) {
  // Check collisions with world
  this->GetCollisionsExternalBoxExt(map, extColExt);
}

void Object::ComputeNextState() {
  bool inAir;

  inAir = ((extColExt.GetLeftDownCol() == 0) &&
           (extColExt.GetRightDownCol() == 0));

  // Object implements simple states based on world collisions
  // Derivate classes should re-implement this function for more complex behaviour
  switch(state) {
    case OBJ_STATE_STOP:
    case OBJ_STATE_MOVING:

      if (inAir) {
        state = OBJ_STATE_MOVING;
        direction = OBJ_DIR_DOWN;
      } else {
        state = OBJ_STATE_STOP;
        direction = OBJ_DIR_STOP;
      }

      break;

    default:
      break;
  }

}

void Object::ComputeNextPosition(World* map) {

  switch(state) {
    case OBJ_STATE_STOP:
      break;

    case OBJ_STATE_MOVING:
      if (direction & OBJ_DIR_UP)
        SetY(map, GetY() - speed_y);
      else if (direction & OBJ_DIR_DOWN)
        SetY(map, GetY() + speed_y);

      if (direction & OBJ_DIR_RIGHT)
        SetX(map, GetX() + speed_x);
      else if (direction & OBJ_DIR_LEFT)
        SetX(map, GetX() - speed_x);

      break;

    default:
      break;
  }
}

void Object::ComputeNextSpeed() {

  switch (state) {
    case OBJ_STATE_STOP:
      speed_x = 0.0;
      speed_y = 0.0;
      break;

    case OBJ_STATE_MOVING:
        if (!steps_in_state) {
          if (direction & OBJ_DIR_UP)
            speed_y = speed_y_max;
          else
            speed_y = speed_y_min;
        } else if ((direction & OBJ_DIR_UP) && (steps_in_direction_y > 0)) {
          if (speed_y > speed_y_min)
            speed_y = speed_y - speed_y_step;
          else
            speed_y = speed_y_min;
        } else if ((direction & OBJ_DIR_DOWN) && (steps_in_direction_y > 0)) {
          if (speed_y < speed_y_max)
            speed_y = speed_y + speed_y_step;
          else
            speed_y = speed_y_max;
        }

        speed_x = speed_x_max;
      break;

    default:
      speed_x = speed_x_max;
      speed_y = speed_y_max;
      break;
  }
}

void Object::ObjectStep(World* map) {
  this->ComputeCollisions(map);
  this->ComputeNextState();
  this->ComputeNextPosition(map);
  this->ComputeNextSpeed();
}