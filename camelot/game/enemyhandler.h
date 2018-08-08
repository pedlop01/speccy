// Class automatically generated by Dev-C++ New Class wizard

#ifndef ENEMYHANDLER_H
#define ENEMYHANDLER_H

#include <math.h>
#include <vector>
#include "entradaSortida.h"
#include "defines.h"

#include "enemigo.h"
#include "character.h"
#include "mundo.h"

#include "symboltable.h"
#include "fxhandler.h"

using namespace std;

/*
 * No description
 */
class EnemyHandler
{
    private:
        int    numEnemies;
        vector<enemigo*> enemies;
        
        symbolTablePtr symbTable;
        fxHandler* soundHandler;    
        
	public:		
		EnemyHandler(symbolTablePtr _symbolTable, fxHandler* _soundHandler);         // class constructor
		~EnemyHandler();                                    // class destructor
		
		void ReadEnemiesFile(char *file);		
		void SaveEnemiesFile(char *file);
		void HandleEnemiesState(BITMAP* buffer, mundo* world, character* warrior, unsigned int mseg, int x, int y);
		void DrawEnemiesInScreen(BITMAP* buffer, mundo* world, int screen_x, int screen_y, int size_x, int size_y);

		void KillEnemy(mundo* world, BITMAP* buffer, character* warrior, enemigo* enemy, int correct_x, int correct_y);
		void EnemyRessurection(unsigned int mseg, enemigo* enemy);		
		
		bool EnemyKillWarrior(BITMAP* buffer, mundo* world, character* warrior, int x, int y);
		
		void AllEnemiesToInitialState();
		
		inline void AddNewEnemy(enemigo* enemy) { enemies.push_back(enemy); numEnemies = enemies.size(); }
		
		inline vector<enemigo*>* GetEnemies()   { return &enemies; }

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

#endif // ENEMYHANDLER_H
