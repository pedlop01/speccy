// Class automatically generated by Dev-C++ New Class wizard

#ifndef OBJETO_H
#define OBJETO_H

#include <stdlib.h>
#include <map.h>
#include <math.h>
#include <string>

#include "defines.h"

#include "mundo.h"
#include "static_image.h"
#include "animation.h"

#include "entradasortida.h"

#include "symboltable.h"

// Sentido hacia el que est� orientado el objeto
#define OBJETO_SENTIDO_DERECHA            0
#define OBJETO_SENTIDO_IZQUIERDA          1

using namespace std;

/*
 * No description
 */
class objeto
{
  private:
    string name;
    string origFile;
    
    int sentido;
          
    unsigned int activated;
    unsigned int picked;
    unsigned int numRepAnims; 
          
    int current_x_ant;         // Last last valid position
    int current_y_ant;  
    int current_x;             // Last valid position
    int current_y;
       
    // animations
    entradaSortida             fitxerAnimacio;        
    static_image               animationBitmap;
    map<int, animation>        animacion;
    int                        subAnimacion;
    list<sprite_ptr>::iterator currentSprite;
    unsigned int               mseg_chk_anim;          

    // drawing
    BITMAP* buffer;            // buffer actual (pantalla de juego)
    
    // Pointer to symbol table
    symbolTablePtr symbTable;
       
  public:
    objeto(symbolTablePtr _symbTable,
           int sentido, 
           int pos_x, 
           int pos_y, 
           unsigned int active,
           const char *name, 
           char *animationFile);  // class constructor		
    ~objeto();                                             // class destructor
    
    void reset();
    inline void resetNumRepAnims() { numRepAnims = 0; }
    
    inline string GetName()       { return name; }
    inline void SetName(string n) { name = n;    }
    
    inline string GetOrigFile()       { return origFile; }
    inline void SetOrigFile(string n) { origFile = n;    }
    
    inline int GetCurrentX()      { return current_x; }
    inline int SetCurrentX(int x) { current_x = x;    }
    inline int GetCurrentY()      { return current_y; }
    inline int SetCurrentY(int y) { current_y = y;    }
    
    inline int GetCurrentSizeX()  { assert( currentSprite != NULL ); return (*currentSprite)->xSize; }
    inline int GetCurrentSizeY()  { assert( currentSprite != NULL ); return (*currentSprite)->ySize; }    
    
    inline void SetActivated(unsigned int a) { activated = a; }
    inline unsigned int GetActivated()       { return activated; }
    
    inline void SetPicked(bool _picked) { picked = _picked;   }
    inline int  GetPicked()             { return picked;      }
    inline int  GetNumRepAnims()        { return numRepAnims; }
    
    // animations
	void loadAnimations(char* file);
    
    void handleState(unsigned int mseg);
    
    // drawing
	void drawObjectInWorld(mundo* world, BITMAP* world, int posX, int posY, bool drawRect);
	
	private:
            
            inline string chopToDirectory(char *file)
            {
                 char *ptr;
                 string token;
                 string directory;
                 
                 ptr = strtok(file, "/\\");
                 while( ptr != NULL )
                 {
                        token = ptr;                        
                        ptr = strtok(NULL, "/\\"); 
                        if( ptr != NULL )
                        {
                            directory += (token + "/");
                        }
                 }
                 return directory;
            }
};

#endif // OBJETO_H