// Class automatically generated by Dev-C++ New Class wizard

#include "machinimiahandler.h" // class's header file

animationDescriptor::animationDescriptor()
{
    name.clear();
    numRepeticiones = 0;                                          
}

animationDescriptor::~animationDescriptor()
{
}

animationDescriptor::animationDescriptor(char* _name, unsigned int _numRep)
{
    string aux_name(_name);        

    name = aux_name;
    numRepeticiones = _numRep;
}

// class constructor
machinimiaHandler::machinimiaHandler(symbolTablePtr _symbTable, objectHandler* _objHandler)
{
    running = 0;
    numActiveAnimation = 0;
    numActiveSubAnimation = 0;    
    
    symbTable = _symbTable;
    objHandler = _objHandler;
    
    symbTable->addVarToSymbolTable("animHandlerRunning", (unsigned long*)&running);
    symbTable->addVarToSymbolTable("numActiveAnimation", (unsigned long*)&numActiveAnimation);    
    symbTable->addVarToSymbolTable("numActiveSubAnimation", (unsigned long*)&numActiveSubAnimation);        

    for( int a = 0 ; a < MAX_ANIMS ; a++ )
    {
        for( int i = 0 ; i < MAX_SEQUENTIAL_ANIMS ; i++ )
        {
            for( int j = 0 ; j < MAX_PARALEL_ANIMS ; j++  )
            {
                 animations[a][i][j] = NULL;
            }
        }
    }
}

// class destructor
machinimiaHandler::~machinimiaHandler()
{
    for( int a = 0 ; a < MAX_ANIMS ; a++ )
    {
        for( int i = 0 ; i < MAX_SEQUENTIAL_ANIMS ; i++ )
        {
            for( int j = 0 ; j < MAX_PARALEL_ANIMS ; j++  )
            {
                animationDescriptor* animation;
                
                animation = animations[a][i][j];
                
                if( animation != NULL )
                    delete animations[a][i][j];
            }
        }
    }
}

void machinimiaHandler :: loadAnimations(char *file)
{     
     char buffer[250];
     entradaSortida fitxerAnimacions;
     int numAnimaciones = 0;
     int animationId = 0;      
     int numRep = 0;        
     int numSecAnims = 0;
     int numParAnims = 0;     
     animationDescriptor* animation;
     bool lastSec = false;
     bool lastPar = false;

     // leer fichero animaciones del personaje
     fitxerAnimacions.obrirFitxer(file, LECTURA);

     printf("==============================================\n");     
     printf(" Reading machinimia from file %s\n", file);

     fitxerAnimacions.llegirParaula(buffer);         // total_animations
     fitxerAnimacions.llegirEnter(&numAnimaciones);
  
     assert( numAnimaciones <= MAX_SEQUENTIAL_ANIMS );

     for( int i = 0 ; i < numAnimaciones ; i++ )
     {         
         fitxerAnimacions.llegirParaula(buffer);     // animation
         assert( strcmp(buffer, "animation") == 0 );
         fitxerAnimacions.llegirEnter(&animationId);           
         
         printf("  Animation %d\n", i);
         
         fitxerAnimacions.llegirParaula(buffer);     // animation
         while( strcmp(buffer, "end_animation") != 0 )
         {
             if( strcmp(buffer, "sec") == 0 )
             {        
                 if( lastPar )
                 {
                     numParAnims = 0;
                     numSecAnims++;   // He de contar la paralela anterior
                 }
                              
                 fitxerAnimacions.llegirParaula(buffer);     // name
                 fitxerAnimacions.llegirEnter(&numRep); 
                 
                 printf("\t SEC_SubAnimation = %d; reps = %d; numSecAnims %d; numParAnims %d\n",
                        buffer, numRep, numSecAnims, numParAnims);
                 
                 animation = new animationDescriptor(buffer, numRep);
                 animations[i][numSecAnims][0] = animation;

                 numSecAnims++;
                 lastSec = true;
                 lastPar = false;
             }
             else if( strcmp(buffer, "par") == 0 )
             {
                 if( lastSec )   // Self code. Not really necessary
                 {
                     numParAnims = 0;
                 }
                 
                 fitxerAnimacions.llegirParaula(buffer);     // name
                 fitxerAnimacions.llegirEnter(&numRep); 
                 
                 printf("\t PAR_SubAnimation = %s; reps = %d; numSecAnims %d; numParAnims %d\n",
                        buffer, numRep, numSecAnims, numParAnims);
                 
                 animation = new animationDescriptor(buffer, numRep);
                 animations[i][numSecAnims][numParAnims] = animation;                 
                 numParAnims++;
                 lastSec = false;
                 lastPar = true;                 
             }
             else
             {
                  assert(false);
             }
             
             fitxerAnimacions.llegirParaula(buffer);     // animation     
             assert( numSecAnims <= MAX_PARALEL_ANIMS );    
         }
     }
     printf("==============================================\n");      
}

void machinimiaHandler::processAnimation(unsigned int anim)
{
    bool oneActive = false;
    animationDescriptor* animation;
    
    if( this->animations[anim][numActiveSubAnimation][0] == NULL ) // No more sequential animations
    {
        this->running = 0;
        return;
    }
    
    for( int i = 0 ; (i < MAX_PARALEL_ANIMS) && (this->animations[anim][numActiveSubAnimation][i] != NULL) ; i++ )
    {        
        animation = this->animations[anim][numActiveSubAnimation][i];
/*      FIXME: use printf instead of cout
        cout << animation->getName() << " - " << animation->getNumRepeticiones() << " - " 
             << objHandler->getObject(animation->getName())->GetActivated() << " - " 
             << objHandler->getObject(animation->getName())->GetNumRepAnims() << endl;
*/    
        if( objHandler->getObject(animation->getName())->GetNumRepAnims() < 
            animation->getNumRepeticiones() )
        {
            objHandler->getObject(animation->getName())->SetActivated(1);
            oneActive = true;
        }
        else
        {
            objHandler->getObject(animation->getName())->SetActivated(0);
        }
    }
    // Si llego aqu� es porque he procesado toda una tira
    if( !oneActive ) // Todas est�n paradas
        numActiveSubAnimation++;
}
