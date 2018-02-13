// Class automatically generated by Dev-C++ New Class wizard

#include "objeto.h" // class's header file

// class constructor
objeto::objeto(
    symbolTablePtr _symbTable, 
    int direction, 
    int pos_x, 
    int pos_y, 
    unsigned int active,
    const char *_name, 
    char *animationFile)
{   
    string string_aux(_name);
//    string origFile_aux(animationFile);  // Si queremos guardar la ruta completa
    
    symbTable = _symbTable;
    sentido   = direction;  
    current_x = pos_x;
    current_y = pos_y;
    name = string_aux;
//    origFile = origFile_aux;
    
    activated = active;
    picked = false;
                         
    this->loadAnimations(animationFile);    
    currentSprite = animacion[0].sprites.begin();   // Only 1 animation in object class!
    
    // register variables
    string picked_name(name + "_picked");
    symbTable->addVarToSymbolTable((char*)picked_name.c_str(), (unsigned long*)&picked);
    string activated_name(name + "_activated");
    symbTable->addVarToSymbolTable((char*)activated_name.c_str(), (unsigned long*)&activated);
        
    subAnimacion = 0;
    numRepAnims = 0;
}

void objeto::reset()
{
    picked = false;               
    currentSprite = animacion[0].sprites.begin();   // Only 1 animation in object class!
    subAnimacion = 0;
    numRepAnims = 0;            
}

// class destructor
objeto::~objeto()
{
    name.clear();
    animacion.clear();
}

void objeto :: loadAnimations(char *file)
{
     char   aux_file[80];
     char   buffer[200], bitmapFile[200];
     int    bitmapSizeX, bitmapSizeY;
     int    numAnimaciones;     
     int    velocidad;     
     int    numSprites;

     sprintf(aux_file, "%s", file);

     // leer fichero animaciones del personaje
     fitxerAnimacio.obrirFitxer(aux_file, LECTURA);
     
     cout << "\t Reading object animation from file " << aux_file << endl;

     // cabecera
     fitxerAnimacio.llegirLinea(buffer);
     fitxerAnimacio.llegirLinea(buffer);
     fitxerAnimacio.llegirLinea(buffer);
     fitxerAnimacio.llegirLinea(buffer);         // Linea blanco
     // bitmap
     fitxerAnimacio.llegirParaula(buffer);
     fitxerAnimacio.llegirParaula(bitmapFile);
     fitxerAnimacio.llegirParaula(buffer);
     fitxerAnimacio.llegirEnter(&bitmapSizeX);
     fitxerAnimacio.llegirParaula(buffer);
     fitxerAnimacio.llegirEnter(&bitmapSizeY);
     fitxerAnimacio.llegirLinea(buffer);         // Linea blanco

     string completeBitmapFile = chopToDirectory(aux_file) + bitmapFile;
     animationBitmap.openImage(completeBitmapFile.c_str(), bitmapSizeX, bitmapSizeY);

     // animaciones
     fitxerAnimacio.llegirParaula(buffer);
     fitxerAnimacio.llegirEnter(&numAnimaciones);
     fitxerAnimacio.llegirLinea(buffer);         // Linea blanco
     assert(numAnimaciones == 1);    // Object don't have multiple animations

     for( int i = 0 ; i < numAnimaciones ; i++ )
     {
         int numFrames;
         fitxerAnimacio.llegirParaula(buffer);
         fitxerAnimacio.llegirParaula(buffer);   // nombre animacion

         fitxerAnimacio.llegirParaula(buffer);
         fitxerAnimacio.llegirEnter(&numFrames);

         fitxerAnimacio.llegirParaula(buffer);
         fitxerAnimacio.llegirEnter(&velocidad);

         animacion[i].numSprites = 0;
         animacion[i].velocidad  = velocidad;
         for( int j = 0; j < numFrames ; j++ )
         {
             int xUpLeft, yUpLeft, xDownRight, yDownRight;
             int estado;
             fitxerAnimacio.llegirParaula(buffer);
             fitxerAnimacio.llegirEnter(&xUpLeft);
             fitxerAnimacio.llegirEnter(&yUpLeft);
             fitxerAnimacio.llegirEnter(&xDownRight);
             fitxerAnimacio.llegirEnter(&yDownRight);
             fitxerAnimacio.llegirEnter(&estado);
             fitxerAnimacio.llegirEnter(&numSprites);

             sprite *spriteAux = new sprite();
             spriteAux->newSprite(&animationBitmap,
                                  xUpLeft, yUpLeft,
                                  xDownRight, yDownRight,
                                  estado, numSprites);    
             animacion[i].numSprites++;
             animacion[i].sprites.push_back(spriteAux);
                                               
             for( int k = 0 ; k < numSprites ; k++ )   // composed sprite
             {
                 fitxerAnimacio.llegirParaula(buffer);
                 fitxerAnimacio.llegirEnter(&xUpLeft);
                 fitxerAnimacio.llegirEnter(&yUpLeft);
                 fitxerAnimacio.llegirEnter(&xDownRight);
                 fitxerAnimacio.llegirEnter(&yDownRight);
                 
                 spriteAux->AddInternalSprite(&animationBitmap, xUpLeft, yUpLeft, xDownRight, yDownRight);
             }
         }
     }
}

void objeto :: handleState(unsigned int mseg)
{
    bool trobat = false;
    
    if( !activated )
        return;
    
    // gesti�n del tiempo para controlar el cambio de animacion (sprite)
    if( mseg - mseg_chk_anim > animacion[0].velocidad )
    {        
         subAnimacion++;
         if( subAnimacion == animacion[0].sprites.size() )
         {
                subAnimacion = 0;
         }
         else if( subAnimacion == (animacion[0].sprites.size() - 1) )
         {
               numRepAnims++;
         }
         
         if( animacion[0].sprites.size() == 1 )
         {
              numRepAnims++;
         }
         
         mseg_chk_anim = mseg;
    }
       
    // Seleccion del sprite de la animacion
    int i = 0;
    for( currentSprite = animacion[0].sprites.begin() ;
         currentSprite != animacion[0].sprites.end() && !trobat; 
       )
    {        
         if( i == subAnimacion )
         {
             trobat = true;       
         }       
         else
         {
             i++;    
             currentSprite++;
         }    
    }
     
    ASSERT( trobat == true );
}

void objeto :: drawObjectInWorld(mundo* world, BITMAP* buffer, int posXWorld, int posYWorld, bool drawRect)
{
    BITMAP* sprite;
    BITMAP* internal_sprite;

    if( !activated )
        return;

    // Informacion para pintar el sprite en el mundo
    int xUpLeft    = (*currentSprite)->xUpLeft;    
    int yUpLeft    = (*currentSprite)->yUpLeft;
    int xDownRight = (*currentSprite)->xDownRight;
    int yDownRight = (*currentSprite)->yDownRight;
    int xSize      = (*currentSprite)->xSize;
    int ySize      = (*currentSprite)->ySize;
    int nComp      = (*currentSprite)->numComponents;

    // Procedemos a pintar
    sprite = create_bitmap(world->GetTileSizeRatio()*xSize, world->GetTileSizeRatio()*ySize); 

    stretch_blit(animationBitmap.getImage(), sprite, xUpLeft, yUpLeft,
                   xSize, ySize,
                   0, 0, world->GetTileSizeRatio()*xSize, world->GetTileSizeRatio()*ySize);   

    if( drawRect == true )
    {
        rect(buffer, posXWorld, posYWorld, posXWorld + xSize*world->GetTileSizeRatio(), posYWorld + ySize*world->GetTileSizeRatio(), 0xABCDE);
    }
    if( sentido == OBJETO_SENTIDO_DERECHA )
    {
        draw_sprite(buffer, sprite, posXWorld, posYWorld);        // DERECHA
    }
    else
    {
        draw_sprite_h_flip(buffer, sprite, posXWorld, posYWorld); // IZQUIERDA
    }
    
    // Composed sprite to draw
    for( int i = 0 ; i < nComp ; i++ )
    {
         xSize      = (*currentSprite)->internalSprites[i]->xSize;
         ySize      = (*currentSprite)->internalSprites[i]->ySize;
         xUpLeft    = (*currentSprite)->internalSprites[i]->xUpLeft;    
         yUpLeft    = (*currentSprite)->internalSprites[i]->yUpLeft;

         internal_sprite = create_bitmap(world->GetTileSizeRatio()*xSize, world->GetTileSizeRatio()*ySize);    
         stretch_blit(animationBitmap.getImage(), internal_sprite, xUpLeft, yUpLeft,
                      xSize, ySize,
                      0, 0, world->GetTileSizeRatio()*xSize, world->GetTileSizeRatio()*ySize);   
         
         if( sentido == OBJETO_SENTIDO_DERECHA )
         {
             draw_sprite(buffer, internal_sprite, posXWorld + (i + 1)*world->GetTileSizeRatio()*xSize, posYWorld);        // DERECHA
         }
         else
         {
             draw_sprite_h_flip(buffer, internal_sprite, posXWorld - (i + 1)*world->GetTileSizeRatio()*xSize, posYWorld); // IZQUIERDA
         }         
         destroy_bitmap(internal_sprite);
    }
    
    destroy_bitmap(sprite);       
}