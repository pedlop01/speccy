// Class automatically generated by Dev-C++ New Class wizard

#ifndef MUNDO_H
#define MUNDO_H

#include <allegro.h>
#include <iostream.h>

#include "entradasortida.h"

class tile 
{
    private:
        int value;
        int collisionable;
        int position;

    public:
        tile()
        {
            value = 0;
            collisionable = 0;
            position = 0;
        }
        ~tile() { ; }
        
        int  GetValue()                { return value;         }
        void SetValue(int val)         { value = val;          }
        int  GetCollisionable()        { return collisionable; }
        void SetCollisionable(int col) { collisionable = col;  }
        int  GetPosition()             { return position;      }
        void SetPosition(int pos)      { position = pos;       }                        
};

class mundo
{
    private:
        entradaSortida io;
        
        int     screen_orig_x;
        int     screen_orig_y;
        int     screen_tiles_x;
        int     screen_tiles_y;
        int     screen_new_x;
        int     screen_new_y;
        int     tiles_orig_x;
        int     tiles_orig_y;
        int     tiles_m_x;
        int     tiles_m_y;
        int     tiles_size_x;
        int     tiles_size_y;
        int     tiles_in_tiles_image_x;
        tile    ***tiles_mundo;
        int     num_scroll_planes;
        int     *scroll_size_x;
        int     *scroll_size_y;
        RGB     pal[256];
        char    buffer[200], path_world_image[300];
        BITMAP* world_image;
        BITMAP** scroll_planes;
        
	public:		
		mundo();                          // class constructor		
    	mundo(char *file, bool fte);      // class constructor		
		~mundo();                         // class destructor
		
		int GetScreenOrigX()  { return screen_orig_x;  }
		int GetSreenOrigY()   { return screen_orig_y;  }
		int GetScreenTilesX() { return screen_tiles_x; }
		int GetScreenTilesY() { return screen_tiles_y; }
		int GetTilesOrigX()   { return tiles_orig_x;   }
		int GetTilesOrigY()   { return tiles_orig_y;   }
		int GetTilesWorldX()  { return tiles_m_x;      }
		int GetTilesWorldY()  { return tiles_m_y;      }
		int GetTilesSizeX()   { return tiles_size_x;   }
		int GetTilesSizeY()   { return tiles_size_y;   }
		int GetScreenNewX()   { return screen_new_x;   }
		int GetScreenNewY()   { return screen_new_y;   }

		int   GetNumScrollPlanes()       { return num_scroll_planes; }		
   	    tile* GetTile(int x, int y)      { return tiles_mundo[y][x]; }
		int   GetTileValue(int x, int y) { return tiles_mundo[y][x]->GetValue(); }
		int   GetScrollSizeX(int i)      { return scroll_size_x[i];  }
		int   GetScrollSizeY(int i)      { return scroll_size_y[i];  }
		
		BITMAP* GetWorldImage()        { return world_image; }
		BITMAP* GetScrollImage(int i)  { return scroll_planes[i]; }
		
		int   GetTileValueByCoord(int x, int y);
    	tile* GetTileByCoord(int x, int y);
    	
    	void SaveWorldInFile(char *file);
		
		
		
};

#endif // MUNDO_H