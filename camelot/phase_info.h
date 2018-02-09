// Class automatically generated by Dev-C++ New Class wizard

#ifndef PHASE_INFO_H
#define PHASE_INFO_H

#include <stdlib.h>
#include "entradasortida.h"

/*
 * This class has the information about the phases
 */
class phase_info
{
    private:
      entradaSortida io;
    
      int x_ini;
      int y_ini;
      int warrior_form;
      int warrior_prev_state;
      int warrior_ini_state;      
      int warrior_dir;
      char song[80];
      char world_description[80];
      char enemies_description[80];
      char chk_description[80];
      char scripts[80];
      char objects[80];
      char scrolls[80];
      char anims[80];
      
	public:
		phase_info();      		// class constructor
		~phase_info();          // class destructor
		
		void readPhaseInfo(char* file);
		
		int   GetXIni() { return x_ini; }
		int   GetYIni() { return y_ini; }

		int   GetWarriorPrevState() { return warrior_prev_state;  }
		int   GetWarriorIniState()  { return warrior_ini_state;   }		
		int   GetWarriorDir()       { return warrior_dir;         }
    	int   GetWarriorForm()      { return warrior_form;        }
		
		char* GetSongFile()               { return song;                }
		char* GetWorldDescriptionFile()   { return world_description;   }
		char* GetEnemiesDescriptionFile() { return enemies_description; }
		char* GetChkDescriptionFile()     { return chk_description;     }
		char* GetScriptsFile()            { return scripts;             }
		char* GetObjectsFile()            { return objects;             }
		char* GetScrollsFile()            { return scrolls;             }		
   	    char* GetAnimsFile()              { return anims;               }		
};

#endif // PHASE_INFO_H
