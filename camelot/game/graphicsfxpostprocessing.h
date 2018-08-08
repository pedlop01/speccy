// Class automatically generated by Dev-C++ New Class wizard

#ifndef GRAPHICSFXPOSTPROCESSING_H
#define GRAPHICSFXPOSTPROCESSING_H

#include <allegro.h>
#include <stdlib.h>
#include <map.h>
#include <math.h>
#include <string>

#include "defines.h"

/*
 * No description
 */
class graphicsFxPostProcessing
{
  private:
    unsigned int saved_time;
    
  public:
    graphicsFxPostProcessing();  		// class constructor	
	~graphicsFxPostProcessing();        // class destructor
	
	void initialize(); 
	void handler(BITMAP* input_buffer, BITMAP* output_buffer, unsigned int time);
};

#endif // GRAPHICSFXPOSTPROCESSING_H