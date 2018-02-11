#ifndef WORLD_H
#define WORLD_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <fstream>
#include <string>
#include <tgmath.h>
#include "pugixml.hpp"

#include <iostream>

using namespace std;

class Tile
{
    private:
        int left_up_x;
        int left_up_y;
        int right_down_x;
        int right_down_y;
        int value;
        int collisionable;
        int pintado;
        int position;

    public:
        Tile()
        {
            value = 0;
            collisionable = 0;
            pintado = 0;
            position = 0;
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
        int  GetCollisionable()        { return collisionable; }
        void SetCollisionable(int col) { collisionable = col;  }
        int  GetPosition()             { return position;      }
        void SetPosition(int pos)      { position = pos;       }                        
        int  GetPintado()              { return pintado;       }
        void SetPintado(int pint)      { pintado = pint;       }
};

class World
{
    private:       
        int     map_width;       // In number of tiles
        int     map_height;      // In number of tiles
        int     tileset_width;   // In pixels
        int     tileset_height;  // In pixels
        int     tileset_count;
        int     tileset_columns;        

        Tile    ***world_tiles;

        ALLEGRO_BITMAP* world_image;

        pugi::xml_document world_file;
        
	public:		
		World();                        // class constructor		
    	World(char *file, bool b);      // class constructor		
		~World();                       // class destructor
		
        int   GetMapWidth()              { return map_width;            }
        int   GetMapHeight()             { return map_height;           }
        int   GetTilesetWidth()          { return tileset_width;        }
        int   GetTilesetHeight()         { return tileset_height;       }
        int   GetTilesetCount()          { return tileset_count;        }
        int   GetTilesetColumns()        { return tileset_columns;      }

   	    Tile* GetTile(int x, int y)      { return world_tiles[x][y]; }
		int   GetTileValue(int x, int y) { return world_tiles[x][y]->GetValue(); }		
		
		ALLEGRO_BITMAP* GetWorldImage()  { return world_image;     }
		
		int   GetTileValueByCoord(int x, int y);
    	Tile* GetTileByCoord(int x, int y);

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
