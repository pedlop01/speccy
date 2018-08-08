// Class automatically generated by Dev-C++ New Class wizard

#include "messageboxhandler.h" // class's header file

// class constructor
messageBoxHandler::messageBoxHandler(symbolTablePtr symb, char* file)
{
    reason = 0;
    activated = 0;                                                    
    messageWindow = new objeto(symb, 0, 0, 0, 1, "messageBox", file);
    
    symbTable = symb;
    symbTable->addVarToSymbolTable("msgBoxActivated", (unsigned long*)&activated);     
    symbTable->addVarToSymbolTable("msgBoxReason", (unsigned long*)&reason);     
    
    wakeUpByKey = false;
    counting = false;
    
    myfont = load_font("./data/fonts/andalus.pcx", palette, NULL);
    if( !myfont )
    {
        printf("Couldn't load font!\n");
        exit(-1);
    }
}

// class destructor
messageBoxHandler::~messageBoxHandler()
{
    destroy_font(myfont);
}

// S� que esta funci�n no est� bien acabada, con m�s tiempo lo intentar� arreglar
// pero ahora no quiero perder tiempo buscando como justificar el texto correctamente.
// Tampoco es importante para acabar esta primera version del juego y prefiero
// centrarme en aspectos m�s importantes.

void messageBoxHandler::drawMessageBox(
    BITMAP* buffer, 
    mundo* world, 
    unsigned int mseg,
    unsigned int entrada_teclado)
{
     int i = 0;
     char line[3][50];
     unsigned int pos_x, pos_y;
     int size_msg_x = messageWindow->GetCurrentSizeX() - 64;     
         
     pos_x = (SCREEN_X_ORIG >> 2) - (messageWindow->GetCurrentSizeX() >> 2);
     pos_y = (SCREEN_Y_ORIG >> 2) - (messageWindow->GetCurrentSizeY() >> 2);
     
     messageWindow->drawObjectInWorld(world, buffer, pos_x, pos_y, false);
     if( messageWindow->GetNumRepAnims() < 1 )
     {
         messageWindow->handleState(mseg);
     }
     else
     {
         if( strlen(text)*8 <= size_msg_x )
         {
             textout_ex(buffer, myfont, text, pos_x + 100, pos_y + 100, 0x00, -1);       
         }
         else
         {
             char text_cp[100];
             char *tok;
             
             sprintf(text_cp, "%s", text);
             tok = strtok(text_cp, " ");
             for( i = 0 ; i < 3 ; i++ )
             {
                 sprintf(line[i], "");
             }
             i = 0;
             while( tok != NULL )
             {
                 if( strlen(line[i])*8 <= size_msg_x )
                 {
                      sprintf(line[i], "%s %s ", line[i], tok);
                 }
                 else
                 {
                     i++;
                     if( i == 3 )
                     {
                         printf(" ERROR: text too large!\n");
                         system("pause");
                         exit(-1);   
                     }   
                     else
                     {
                         sprintf(line[i], "%s %s ", line[i], tok);                     
                     }                      
                 }
                 tok = strtok(NULL, " ");
             }
             textout_ex(buffer, myfont, line[0], pos_x + 32, pos_y + 64, 0x00, -1);       
             textout_ex(buffer, myfont, line[1], pos_x + 32, pos_y + 128, 0x00, -1);       
             textout_ex(buffer, myfont, line[2], pos_x + 32, pos_y + 192, 0x00, -1);       
         }    
     }
     
     
    if( mseg - timeChk > 1000 )
    {                
        counting = false;
        if( !wakeUpByKey )
            activated = 0;
        else
            if( entrada_teclado != NO_KEY )
                activated = 0;         
    }    
    
    if( !activated )
    {
        messageWindow->reset();    
    }
}
