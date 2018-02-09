// Class automatically generated by Dev-C++ New Class wizard

#include "fxhandler.h" // class's header file

sound :: sound(FMOD_SYSTEM *sys, char *n, bool destroy, bool _loop)
{
    assert(sys != NULL );
    
    enabled = false;
    
    system = sys;
    channel = 0;
    strcpy(name, n);
    destroyable = destroy;
    loop = _loop;
    if( !destroyable )
    {
        this->init(); 
    }
    else
    {
        fmodSound = false;
    }
}
sound :: ~sound() 
{
    if( !destroyable )
    {
        this->destroy();     
    }
}

void sound :: init()
{   
    if( !loop )
    {
        if( FMOD_System_CreateStream(system, name, FMOD_HARDWARE | FMOD_IGNORETAGS | FMOD_CREATESAMPLE | FMOD_2D, 0, &fmodSound) )
            allegro_message(name);    //FIXME 
    }
    else
    {
        if( FMOD_System_CreateStream(system, name, FMOD_HARDWARE | FMOD_IGNORETAGS | FMOD_LOOP_NORMAL | FMOD_CREATESAMPLE | FMOD_2D, 0, &fmodSound) )
            allegro_message(name);    //FIXME         
    }
}

void sound :: destroy()
{
    if( FMOD_Sound_Release(fmodSound) )
        allegro_message(name);   // FIXME   
}

void sound :: play()
{   
    FMOD_BOOL is_playing = false;
    if( destroyable )
    { 
        this->init();
    }
    if( FMOD_System_PlaySound(system, FMOD_CHANNEL_FREE, fmodSound, 0, &channel) )    
            allegro_message(" Error playing sound ");

    if( destroyable )
    {
        this->destroy();
    }
}

// class constructor
fxHandler::fxHandler()
{
    system = NULL;
    channel = NULL;       

    if( (result = FMOD_System_Create(&system)) )
        allegro_message(" Error creating sound system ");

    if( (result = FMOD_System_GetVersion(system, &version)) )
        allegro_message(" Error getting sound system version ");

    if( (result = FMOD_System_Init(system, 1, FMOD_INIT_NORMAL, NULL)) )
         allegro_message(" Error initializing sound system ");

    numSounds = 0;
}

// class destructor
fxHandler::~fxHandler()
{
    if( (result = FMOD_System_Close(system)) )
        allegro_message(" Error closing sound system ");
    if( (result = FMOD_System_Release(system)) )
        allegro_message(" Error realising sound system ");
                       
}

int fxHandler :: addSound(char *name, bool destroyable, bool loop)
{
    sound* newSound = new sound(system, name, destroyable, loop);
    assert( newSound != NULL );
    sounds.push_back(newSound);
    numSounds++;
    
    return numSounds - 1;
}

void fxHandler :: playSound(int numSound)
{
    FMOD_Channel_SetPaused(sounds[numSound]->getChannel(), false);     
    sounds[numSound]->play();
}

void fxHandler :: stopSound(int numSound)
{
    FMOD_Channel_SetPaused(sounds[numSound]->getChannel(), true);
}

bool fxHandler :: getEnabled(int numSound)
{     
    sounds[numSound]->getEnabled();
}

void fxHandler :: setEnabled(int numSound, bool s)
{
    sounds[numSound]->setEnabled(s);
}
