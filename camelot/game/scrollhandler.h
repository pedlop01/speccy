// Class automatically generated by Dev-C++ New Class wizard

#ifndef SCROLLHANDLER_H
#define SCROLLHANDLER_H

#include <assert.h>

#include "defines.h"
#include "entradasortida.h"

#define NORMAL         0
#define FAST_ADVANCE   1
#define NORMAL_ADVANCE 2

#define RECOMPUTE_SCROLL_X   0
#define FAST_ADVANCE_LEFT    1
#define FAST_ADVANCE_RIGHT   2
#define NORMAL_ADVANCE_LEFT  3
#define NORMAL_ADVANCE_RIGHT 4

#define RECOMPUTE_SCROLL_Y  0
#define FAST_ADVANCE_UP     1
#define FAST_ADVANCE_DOWN   2
#define NORMAL_ADVANCE_UP   3
#define NORMAL_ADVANCE_DOWN 4

using namespace std;

class scrollInfo
{
  public:
    unsigned int scrollStartX;
    unsigned int scrollStartY;
    unsigned int worldSizeX;
    unsigned int worldSizeY;
    unsigned int screenSizeX;
    unsigned int screenSizeY;    
    
  public:
    scrollInfo();
    ~scrollInfo();
};

class ScrollHandler
{
  private:            
    scrollInfo scrollLimitsInfo[20];
    unsigned int currentScrollInfo;
    unsigned int currentScrollZones;
    
    // Last saved screen and warrior positions
    unsigned int screenCoordX;
    unsigned int screenCoordY;
    unsigned int warriorCoordX;
    unsigned int warriorCoordY;
    unsigned int warriorSentido;
    
    unsigned int savedScreenCoordX;
    unsigned int savedScreenCoordY;
    
    unsigned int stateX;
    unsigned int stateY;    
    unsigned int state;    
    
    unsigned int fastVelX;
    unsigned int fastVelY;          
    
  public:		
    ScrollHandler();    // class constructor		
	~ScrollHandler();   // class destructor
	
	void init(char *file, 
	          unsigned int ssx, unsigned int ssy,
              unsigned int wcx, unsigned int wcy, unsigned int ws,
              unsigned int fvx, unsigned int fvy);
	
	
    void supervisor(unsigned int wcx, unsigned int wcy, unsigned int ww, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);              

    void normalScrollX(unsigned int wcx, unsigned int wcy, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);
    void normalScrollY(unsigned int wcx, unsigned int wcy, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);    
    void normalScroll(unsigned int wcx, unsigned int wcy, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);
    void fastScroll(unsigned int wcx, unsigned int wcy, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);    
    
    //void newHandler(unsigned int wcx, unsigned int wcy, unsigned int ws, int &scx, int &scy, int &wscx, int &wscy);
};

#endif // SCROLLHANDLER_H
