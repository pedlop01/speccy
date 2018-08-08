// Class automatically generated by Dev-C++ New Class wizard

#ifndef ENEMIGO_H
#define ENEMIGO_H

#include <stdlib.h>
#include <map>
#include <math.h>
#include <string>

#include "defines.h"

#include "character.h"
#include "mundo.h"
#include "static_image.h"
#include "animation.h"
#include "fxhandler.h"

#include "entradasortida.h"

#include "symboltable.h"

// Sentido en hacia el que est� orientado el personaje
#define ENEMIGO_NO_SENTIDO                 0
#define ENEMIGO_SENTIDO_IZQUIERDA          1
#define ENEMIGO_SENTIDO_DERECHA            2
#define ENEMIGO_NO_SENTIDO_VERTICAL        0
#define ENEMIGO_NO_SENTIDO_VERTICAL_ARRIBA 1
#define ENEMIGO_NO_SENTIDO_VERTICAL_ABAJO  2

// Estado actual del personaje. Estos son macro estados, internamente
// existen estados intermedios seg�n la animaci�n en la que se encuentre
#define ENEMIGO_PARADO         0
#define ENEMIGO_CAMINANDO      1
#define ENEMIGO_SALTANDO       2
#define ENEMIGO_CAYENDO        3
#define ENEMIGO_COGER_ESPADA   4
#define ENEMIGO_GOLPEANDO      5
#define ENEMIGO_EN_GUARDIA     6
#define ENEMIGO_AGACHADO       7
#define ENEMIGO_MUERTO         8

using namespace std;

/*
 * Enemigo del juego. De momento, y hasta que se haga el grafo de estados pasado
 * por fichero, toda la gesti�n se har� con esta clase. 
 */
class enemigo
{
    private:
        int id;
        
        char origFile[250];
            
        int enemyType;
            
        int activated;
        
        int current_state;
        int previous_state;
        int enemigo_sentido;   
        int enemigo_sentido_vertical;  
        
        bool contact_down;         // Actualmente hay colisi�n por debajo
        bool contact_up;           // Actualmente hay colisi�n por arriba
        unsigned int mseg_chk ;    // Checkpoint del tiempo. �til para las animaciones
        unsigned int pixel_y_chk;  // Checkpoint de la posicion del personaje en un
        unsigned int pixel_x_chk;  // momento determinado
        
        int current_x_ant;         // Last last valid position
        int current_y_ant;  
        int current_x;             // Last valid position
        int current_y;
       
        // animations
        string                     animationFile;
        entradaSortida             fitxerAnimacio;        
        static_image               animationBitmap;
        map<int, animation>        animacion;
        int                        subAnimacion;
        list<sprite_ptr>::iterator currentSprite;
        unsigned int               mseg_chk_anim;
        
        int ini_x;
        int ini_y;
        int ini_state;
        int ini_sentido;
        int ini_sentido_vertical;        
        int desp_x;
        int desp_y;   
        int vel;     
        
        bool         killed;
        unsigned int deadAtMseg;
        
        bool         enemigoEnRango;
        bool         reset;
        
        // drawing
        BITMAP* buffer;            // buffer actual (pantalla de juego)

        symbolTablePtr symbTable;
        fxHandler* soundHandler;        

	public:
		enemigo();                 // class constructor
		enemigo(symbolTablePtr _symbolTable,
		        fxHandler* _soundHandler,
                int id,
                int type,
                int initial_state, 
                int sentido, 
                int sentido_vertical,
                int x, int y, 
                int despx, int despy,
                int velocidad,
                const char *animationFile);
		~enemigo();      		   // class destructor
		
		void handleState(character* warrior, unsigned int mseg_global);
		void handleStateEnemyDespX(unsigned int mseg_global);
		void handleStateEnemyDespXAndY(unsigned int mseg_global);
		void handleStateEnemyDespYNoLoop(unsigned int mseg_global);		
		void handleStateEnemyDragon(character* warrior, unsigned int mseg_global);
		void computeNewPositionUsingCurrentState(mundo *world, int &x, int &y, unsigned int mseg);
		void computeNewPositionUsingCollisionDetection(mundo *world, unsigned int mseg_global);
		void computeNextAnimationSprite(unsigned int mseg_global);
		
		void  SetOrigFile(string f) { sprintf(origFile, "%s", f.c_str()); }
		inline char* GetOrigFile()        { return origFile; }
		
		// animations
		inline void SetAnimationFile(string n) { animationFile = n; }
		inline string GetAnimationFile()       { return animationFile; }
		
		void loadAnimations(const char* file);
		
		// drawing
		void drawCharacterInWorld(mundo* world, BITMAP* world_bmp, int posX, int posY);
				
        inline int GetEnemyType()    { return enemyType; }
				
		int GetCurrentSizeX();
		int GetCurrentSizeY();
		inline int GetVelocity()     { return vel;       }
		
		inline int GetId()                     { return id;            }
		inline int GetIniSentido()             { return ini_sentido;   }
		inline int GetIniSentidoVertical()     { return ini_sentido_vertical; }		
		inline int GetCurrentState()           { return current_state; }
		inline int GetIniState()               { return ini_state;     }
		
		inline int GetReset()                  { return reset;     }
		inline int GetActivated()              { return activated; }
		
		inline int GetCurrentX()               { return current_x; }
		inline int GetCurrentY()               { return current_y; }
		
		int GetCurrentXCompleteSprite();
		
		inline int GetEnemigoSentido()         { return enemigo_sentido;          }
		inline int GetEnemigoSentidoVertical() { return enemigo_sentido_vertical; }

        inline int GetDespX()                  { return desp_x; }
        inline int GetDespY()                  { return desp_y; }

		inline int GetIniX()                   { return ini_x; }
		inline int GetIniY()                   { return ini_y; }
		inline int GetVel()                    { return vel;   }
		
		inline void SetCurrentX(int x)                { current_x = x; }
		inline void SetCurrentY(int y)                { current_y = y; }
		
        inline void SetId(int _id)                    { id = _id;                     }
		inline void SetDespX(int x)                   { desp_x = x;                   }
		inline void SetDespY(int y)                   { desp_y = y;                   }
		
		inline void SetEnemyType(int t)               { enemyType = t;                }
		inline void SetCurrentState(int s)            { current_state = s;            }
		
		inline void SetIniSentido(int s)              { ini_sentido = s;              }
		inline void SetIniSentidoVertical(int s)      { ini_sentido_vertical = s;     }		
		inline void SetEnemigoSentido(int s)          { enemigo_sentido = s;          }
		inline void SetEnemigoSentidoVertical(int s)  { enemigo_sentido_vertical = s; }
		
		inline void SetIniX(int x)                    { ini_x = x;                    }
		inline void SetIniY(int y)                    { ini_y = y;                    }
		inline void SetVel(int v)                     { vel = v;                      }	
		inline void SetIniState(int s)                { ini_state = s;                }

		void SetStateToInitial(); 
		
		void KillEnemy()                  { killed = true;     }
		inline unsigned int GetDeadTime() { return deadAtMseg; }
        
        bool IsAlive() { return current_state != ENEMIGO_MUERTO; }       				
        
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

#endif // ENEMIGO_H
