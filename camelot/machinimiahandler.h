// Class automatically generated by Dev-C++ New Class wizard

#ifndef MACHINIMIAHANDLER_H
#define MACHINIMIAHANDLER_H

#include <allegro.h>
#include <iostream.h>
#include <string.h>

#include "entradasortida.h"
#include "symboltable.h"
#include "objecthandler.h"

#define MAX_ANIMS            10
#define MAX_SEQUENTIAL_ANIMS 10
#define MAX_PARALEL_ANIMS    10

using namespace std;

class animationDescriptor
{
    private:       
        string name;
        unsigned int numRepeticiones;
        
    public:
        animationDescriptor();
        ~animationDescriptor();
        
        animationDescriptor(char* _name, unsigned int _numRep);

        string getName() { return name; }
        unsigned int getNumRepeticiones() { return numRepeticiones; }
};

/*
 * No description
 */
class machinimiaHandler
{
    private:
        unsigned int running;
        unsigned int numActiveAnimation;
        unsigned int numActiveSubAnimation;
        animationDescriptor* animations[MAX_ANIMS][MAX_SEQUENTIAL_ANIMS][MAX_PARALEL_ANIMS];
        
        // Pointer to symbol table
        symbolTablePtr symbTable;    
        
        objectHandler* objHandler;    
            
	public:
		machinimiaHandler(symbolTablePtr _symbTable, objectHandler* _objHandler);		// class constructor
		~machinimiaHandler();	                          	// class destructor
		
		inline void setRunning(unsigned int _r) { running = _r; }
		inline void setNumActiveAnimation(unsigned int _n) { numActiveAnimation = _n; }
		inline void setNumActiveSubAnimation(unsigned int _n) { numActiveSubAnimation = _n; }
		
		inline unsigned int getRunning() { return running; }
		inline unsigned int getNumActiveAnimation() { return numActiveAnimation; }
		inline unsigned int getNumActiveSubAnimation() { return numActiveSubAnimation; }
		
		void loadAnimations(char *file);
		
		void processAnimation(unsigned int anim);
};

#endif // MACHINIMIAHANDLER_H
