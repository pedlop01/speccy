#ifndef WORLD_H
#define WORLD_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <list>
#include <math.h>
#include <iostream>

#include "pugixml.hpp"
#include "rick_params.h"
#include "character.h"
#include "object.h"
#include "item.h"
#include "platform.h"
#include "laser.h"
#include "static_object.h"
#include "block.h"
#include "shoot.h"

using namespace std;

class Tile
{
    private:
        int left_up_x;
        int left_up_y;
        int right_down_x;
        int right_down_y;
        int value;
        int type;        

    public:
        Tile() {
          value = 0;
          type = 0;
        }
        ~Tile() { ; }
        
        int  SetLeftUpX(int x)         { left_up_x = x;        }
        int  GetLeftUpX()              { return left_up_x;     }
        int  SetLeftUpY(int y)         { left_up_y = y;        }
        int  GetLeftUpY()              { return left_up_y;     }
        int  SetRightDownX(int x)      { right_down_x = x;     }
        int  GetRightDownX()           { return right_down_x;  }
        int  SetRightDownY(int y)      { right_down_y = y;     }        
        int  GetRightDownY()           { return right_down_y;  }        
        int  GetValue()                { return value;         }
        void SetValue(int val)         { value = val;          }
        int  GetType()                 { return type;          }
        void SetType(int _type)        { type = _type;         }

        bool IsCollisionable()         { return ((type == TILE_COL) ||
                                                 (type == TILE_STAIRS_TOP)); }
};

class World
{
    private:       
        int     map_width;           // In number of tiles
        int     map_height;          // In number of tiles

        // REVISIT: make tileset a class
        int     tileset_width;       // In pixels
        int     tileset_height;      // In pixels
        int     tileset_count;
        int     tileset_columns;   
        int     tileset_tile_width;  // Tile width in pixels
        int     tileset_tile_height; // Tile height in pixels

        Tile    ***world_tiles;

        // Platforms belonging to this level
        vector<Platform*> platforms;
        // General objects belonging to this level
        list<Object*> objects;
        // Block belonging to this level
        list<Block*> blocks;

        bool shoot_exists;

        ALLEGRO_BITMAP* world_image;

        pugi::xml_document world_file;
        
	public:		
		World();                          // class constructor		
    World(const char *file, bool b);  // class constructor		
		~World();                         // class destructor
		
        int   GetMapWidth();
        int   GetMapHeight();
        int   GetTilesetWidth();
        int   GetTilesetHeight();
        int   GetTilesetCount();
        int   GetTilesetColumns();
        int   GetTilesetTileWidth();
        int   GetTilesetTileHeight();

        Tile* GetTile(int x, int y);
        int   GetTileValue(int x, int y);
		
		    ALLEGRO_BITMAP* GetWorldImage()  { return world_image;     }
		
		    int   GetTileValueByCoord(int x, int y);
        Tile* GetTileByCoord(int x, int y);

        vector<Platform*>* GetPlatforms() { return &platforms; }
        list<Object*>*     GetObjects()   { return &objects;   }
        list<Block*>*      GetBlocks()    { return &blocks;    }

        void  WorldStep(Character* player);

        void  CreateNewShoot(int x, int y, int direction);

    private:
            
      inline string chopToDirectory(char *file)
      {
        char copy_file[250];
        char *ptr;
        string token;
        string directory;
            
        sprintf(copy_file, "%s", file);    
        ptr = strtok(file, "/\\");
        while( ptr != NULL ) {
          token = ptr;                        
          ptr = strtok(NULL, "/\\"); 
          if( ptr != NULL ) {
            directory += (token + "/");
          }
        }
        return directory;
      }        
};

#endif // WORLD_H
