#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdio.h>
#include <vector>

#include "rick_params.h"
#include "pugixml.hpp"
#include "world.h"
#include "platform.h"
#include "block.h"
#include "keyboard.h"
#include "colbox.h"
#include "animation.h"

// REVISIT: state will be provided from a file
#define RICK_STATE_STOP      0
#define RICK_STATE_JUMPING   1
#define RICK_STATE_RUNNING   2
#define RICK_STATE_CLIMBING  3
#define RICK_STATE_CROUCHING 4
#define RICK_STATE_SHOOTING  5
#define RICK_STATE_BOMBING   6
#define RICK_STATE_HITTING   7
#define RICK_STATE_DYING     8
#define RICK_STATE_DEAD      9

// REVISIT: direction will be provided from a file
#define RICK_DIR_STOP  0b0000
#define RICK_DIR_RIGHT 0b0001
#define RICK_DIR_LEFT  0b0010
#define RICK_DIR_UP    0b0100
#define RICK_DIR_DOWN  0b1000

#define HOR_SPEED_MAX   2.0
#define HOR_SPEED_MIN   1.0
#define HOR_SPEED_STEP  0.1
#define VERT_SPEED_MAX  3.0
#define VERT_SPEED_MIN  1.0
#define VERT_SPEED_STEP 0.1

class Camera;
class Character {
  private:
    int pos_x;
    int pos_y;

    int pos_x_chk;
    int pos_y_chk;

    int height_orig;
    int height;
    int width_orig;
    int width;

    int height_internal;
    int width_internal;

    int prevState;
    int state;
    int direction;
    int face;

    float speed_x;
    float speed_y;

    int stepsInState;
    int stepsInDirectionX;
    int stepsInDirectionY;

    // Collisions
    // - bounding boxes
    Colbox extColInt;
    Colbox extColExt;    
    Colbox extHeightColExt;
    Colbox extHeightColExtOrig;
    Colbox extWidthColExt;
    Colbox heightColInt;
    Colbox heightColExt;
    Colbox widthColInt;
    Colbox widthColExt;
    // - status
    bool inAir;
    bool inAirInt;
    bool inStairs;
    bool inFloor;
    bool inPlatform;
    bool overStairs;
    bool collisionHead;
    bool collisionHeadOrig;
    bool overStairsLeft;
    bool overStairsRight;
    bool blockCollisionLeft;
    bool blockCollisionRight;
    // Initial values for main variables
    int  initial_x;
    int  initial_y;
    int  initial_direction;
    int  initial_speed_x;
    int  initial_speed_y;
    int  initial_state;

    float animation_scaling_factor;

    Platform* inPlatformPtr;
    Block*    blockCollisionPtr;

    pugi::xml_document character_file;

    // Animations
    vector<Animation*> animations;

    // Camera pointer
    // - Needed to localize rick in the screen. Used for some animations.
    Camera* camera;

  public:    
	  Character();    // class constructor
    Character(const char* file);    

    ~Character();   // class desructor

    void Reset();

    void SetPosX(World* map, int x);
    void SetPosY(World* map, int y, bool all);

    int  GetPosX()       { return pos_x;     }
    int  GetPosY()       { return pos_y;     }
    int  GetHeight()     { return height;    }
    int  GetWidth()      { return width;     }
    int  GetState()      { return state;     }
    int  GetDirection()  { return direction; }

    int  GetOrigHeight() { return height_orig; }
    int  GetOrigWidth()  { return width_orig;  }

    int  GetCorrectedPosX();
    int  GetCorrectedPosY();

    void GetCollisionsByCoords(World* map, Colbox &mask_col, int left_up_x, int left_up_y, int width, int height);
    void GetCollisionsExternalBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalWidthBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalHeightBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsExternalHeightBoxExtOrig(World* map, Colbox &mask_col);
    void GetCollisionsExternalBoxInt(World* map, Colbox &mask_col);
    void GetCollisionsInternalWidthBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsInternalWidthBoxInt(World* map, Colbox &mask_col);
    void GetCollisionsInternalHeightBoxExt(World* map, Colbox &mask_col);
    void GetCollisionsInternalHeightBoxInt(World* map, Colbox &mask_col);

    bool ComputeCollisionBlocks(World* map);

    void ComputeCollisions(World* map);
    void ComputeNextState(Keyboard& keyboard);
    void ComputeNextPosition(World* map);
    void ComputeNextPositionBasedOnBlocks(World* map, Keyboard& keyboard);   // REVISIT: using keyboard to test block destruction
    void ComputeNextSpeed();

    void CharacterStep(World* map, Keyboard& keyboard);

    ALLEGRO_BITMAP* GetCurrentAnimationBitmap();
    int GetCurrentAnimationBitmapAttributes();
    int GetCurrentAnimationWidth();
    int GetCurrentAnimationHeight();
    float GetCurrentAnimationScalingFactor();

    void RegisterCamera(Camera* _camera) { camera = _camera; }

  private:
    void FixHorizontalDirection(Keyboard& keyboard);
};

#endif // CHARACTER_H
