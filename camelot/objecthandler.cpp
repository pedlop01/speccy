// Class automatically generated by Dev-C++ New Class wizard

#include "objecthandler.h" // class's header file

// class constructor
objectHandler::objectHandler(symbolTablePtr _symbTable)
{
    symbTable = _symbTable;
}

// class destructor
objectHandler::~objectHandler()
{
    for(vector<objeto*>::iterator it = objects.begin() ; it != objects.end() ; it++ )
    {
        delete *it;
    }
    objects.clear();
}

void objectHandler :: ReadObjectsFromFile(char *file)
{
    char     aux_file[80];
    char     buffer[200];    
    objeto*  object;
    entradaSortida io;

    sprintf(aux_file, "%s", file);

    cout << "==============================================" << endl;       
    cout << " Reading object handler file " << aux_file << endl;
    
    io.obrirFitxer(aux_file, 0);
    
    io.llegirParaula(buffer);    // Objects
    io.llegirEnter(&numObjects);
    cout << " Num objects " << numObjects << endl;
    
    string directory = chopToDirectory(aux_file);
    
    for( unsigned int i = 0 ; i < numObjects ; i++ )
    {
          int  sentido = 0;
          int  pos_x   = 0;
          int  pos_y   = 0;
          int  activated = 0;
          char name[200];          
          char afile[200];
          char aux_afile[200];          

          io.llegirEnter(&sentido);
          io.llegirEnter(&pos_x);
          io.llegirEnter(&pos_y);
          io.llegirEnter(&activated);          
          io.llegirParaula(name);          
          io.llegirParaula(afile);
                   
          string aux(afile);
          string animationBitmap(directory + afile);
          sprintf(aux_afile, "%s", animationBitmap.c_str());
          object = new objeto(symbTable, sentido, pos_x, pos_y, 
                 activated, name, aux_afile);
          object->SetOrigFile(aux);
          objects.push_back(object);
          
          cout << "\t Object " << i 
               << " sentido " << sentido 
               << " x " << pos_x 
               << " y " << pos_y 
               << " animation_file " << file << endl;
    }
    cout << "==============================================" << endl;    
}

void objectHandler :: SaveObjectsToFile(char *file)
{
    char           buffer[200];    
    entradaSortida io;

    cout << "==============================================" << endl;       
    cout << " Saving object handler file " << file << endl;
    
    io.obrirFitxer(file, ESCRIPTURA);     
    
    io.escriureParaula("NumObjects ");
    io.escriureEnter(objects.size());
    io.escriureEspai();             
    io.escriureSaltDeLinia();
    
    for( int i = 0 ; i < objects.size() ; i++ )
    {
         objeto *object = objects[i];
        
         io.escriureEnter(i);
         io.escriureEspai();
         io.escriureEnter(object->GetCurrentX());
         io.escriureEspai();
         io.escriureEnter(object->GetCurrentY());
         io.escriureEspai();        
         sprintf(buffer, "%s", object->GetName().c_str());
         io.escriureParaula(buffer);          
         io.escriureEspai();         
         sprintf(buffer, "%s", object->GetOrigFile().c_str());
         io.escriureParaula(buffer);
         io.escriureEspai();                  
         io.escriureSaltDeLinia();         
    }
    
    io.tancarFitxer();
    cout << "==============================================" << endl;    
}

void objectHandler :: DrawObjectsInScreen(
     BITMAP* buffer, 
     mundo* world, 
     int screen_x, int screen_y, 
     int size_x, int size_y, 
     bool drawRect,
     unsigned int mseg)
{
    objeto *object = NULL;
       
    for( int i = 0 ; i < objects.size() ; i++ )
    {
        object = objects[i];
        
        object->handleState(mseg);   // FIXME: como los objetos solo se pintan, aqu� hay algunas acciones sobre ellos

        if( object->GetPicked() == true )
            continue;
        if( object->GetActivated() == 0 )
            continue;
        if( screen_x <= object->GetCurrentX() + 50 && screen_x + size_x >= object->GetCurrentX() &&
            screen_y <= object->GetCurrentY() + 50 && screen_y + size_y >= object->GetCurrentY() ) // FIXME
        {                     
            object->drawObjectInWorld(world, buffer, object->GetCurrentX() - screen_x, object->GetCurrentY() - screen_y, drawRect);
        }     
    }
}

objeto* objectHandler::getObject(string name)
{
    for( vector<objeto*>::iterator it = objects.begin() ; it != objects.end() ; it++ )
    {
         objeto* obj = *it;
         
         if( obj->GetName() == name )
         {
             return obj;
         }
    }
    
    return NULL;
}

vector<objeto*>::iterator objectHandler :: GetObjectInWorldByCoords(mundo* world, int x, int y, int screen_x, int screen_y)
{   
    for( vector<objeto*>::iterator it = objects.begin() ; it != objects.end() ; it++ )
    {
         objeto* obj = *it;
         
         if( (x + screen_x >= obj->GetCurrentX()) && (x + screen_x <= obj->GetCurrentX() + obj->GetCurrentSizeX()*world->GetTileSizeRatio()) &&
             (y + screen_y >= obj->GetCurrentY()) && (y + screen_y <= obj->GetCurrentY() + obj->GetCurrentSizeY()*world->GetTileSizeRatio()) )
         {
             return it;
         }
    }
    
    return objects.end();
}
