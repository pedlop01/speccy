#include <allegro.h>

#include "defines.h"
#include "game.h"
#include "timer.h"


void highcolor_fade_in(BITMAP *bmp_orig, int speed)
{
   BITMAP *bmp_buff;

    if ((bmp_buff = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG)))
    {
        int a;
        if (speed <= 0) speed = 16;
        
        for (a = 0; a < 256; a+=speed)
        {
            clear(bmp_buff);
            set_trans_blender(0,0,0,a);
            draw_trans_sprite(bmp_buff, bmp_orig, 0, 0);
            vsync();
            stretch_blit(bmp_buff, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);            
        }
        destroy_bitmap(bmp_buff);
    } 

    stretch_blit(bmp_orig, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);                
}

void highcolor_fade_out(int speed)
{
    BITMAP *bmp_orig, *bmp_buff;

    if ((bmp_orig = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG)))
    {
        if ((bmp_buff = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG)))
        {
            int a;
            blit(screen, bmp_orig, 0,0, 0,0, SCREEN_X, SCREEN_Y);
            if (speed <= 0) speed = 16;
        
            for (a = 255-speed; a > 0; a-=speed)
            {
                clear(bmp_buff);
                set_trans_blender(0,0,0,a);
                draw_trans_sprite(bmp_buff, bmp_orig, 0, 0);
                vsync();        
                stretch_blit(bmp_buff, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);                                
            }
            destroy_bitmap(bmp_buff);
        }
        destroy_bitmap(bmp_orig);
    }

    rectfill(screen, 0,0, SCREEN_X, SCREEN_Y, makecol(0,0,0));
}


void allegroInit()
{
	allegro_init();

	install_timer();
	install_keyboard();
	install_mouse();
    install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, "./CamelotRemake.exe");
}

int main()
{
    FONT *myfontAndalus;
    FONT *myfontCandara;    
    PALETTE palette;    
    BITMAP *buffer;
    BITMAP *buffer2;    
    BITMAP *final;     
    char   texto[100];
    bool end_game = false;
    Game game;
    unsigned int last_phase, esc_from_game;
    unsigned int msgChk, msg;
    bool pintaLetras = false;
    
    allegroInit();   
     
    game.init();
    
    buffer = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG);
    buffer2 = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG);    
    final = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG);    
    final = load_bmp("./data/intro/final.bmp", NULL);  
    if( final == NULL )
    {
        printf(" Error loading bitmap\n");
        exit(-1);
    }    
    
    myfontAndalus = load_font("./data/fonts/andalus.pcx", palette, NULL);
    if( !myfontAndalus )
    {
        printf("Couldn't load font!\n");
        exit(-1);
    }
    myfontCandara = load_font("./data/fonts/candara.pcx", palette, NULL);
    if( !myfontCandara )
    {
        printf("Couldn't load font!\n");
        exit(-1);
    }    

    sprintf(texto, "Adrenaline Sofware");
    unsigned int i = 1;
    while( i <= strlen(texto) )
    {
        string aux_text(texto);
        string aux_text2;
                
        clear_bitmap(buffer);

        aux_text2 = aux_text.substr(0, i);
        textout_ex(buffer, myfontAndalus, aux_text2.c_str(), 420, 640, 0xFFFF, -1);        
   	    stretch_blit(buffer, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
   	    i++;
   	    msgChk = game.getTimer()->getMsegGlobal();
   	    while( (game.getTimer()->getMsegGlobal()) - msgChk < 20 );   	       	    
    }
    msgChk = game.getTimer()->getMsegGlobal();
 	while( (game.getTimer()->getMsegGlobal()) - msgChk < 1000 );   	       	    

    clear_bitmap(buffer);
    textout_ex(buffer, myfontAndalus, "PRESENTS", 350, 330, 0xFFFF, -1);        
    stretch_blit(buffer, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
    msgChk = game.getTimer()->getMsegGlobal();
 	while( (game.getTimer()->getMsegGlobal()) - msgChk < 1000 );

    clear_bitmap(buffer);
    textout_ex(buffer, myfontAndalus, "A remake of the original...", 150, 330, 0xFFFF, -1);        
    stretch_blit(buffer, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
    msgChk = game.getTimer()->getMsegGlobal();
 	while( (game.getTimer()->getMsegGlobal()) - msgChk < 1000 );

    buffer = load_bmp("./data/intro/menu0.bmp", NULL);  
    if( buffer == NULL )
    {
        printf(" Error loading bitmap\n");
        exit(-1);
    }
    stretch_blit(buffer, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
    msgChk = game.getTimer()->getMsegGlobal();
 	while( (game.getTimer()->getMsegGlobal()) - msgChk < 2000 );
    destroy_bitmap(buffer);

    // Fundido a blanco !
    highcolor_fade_out(10);
    
    buffer = load_bmp("./data/intro/menu.bmp", NULL);  
    if( buffer == NULL )
    {
        printf(" Error loading bitmap\n");
        exit(-1);
    }
    highcolor_fade_in(buffer, 10);    
        
    // Menu & Game
    while( !key[KEY_ESC] )
    {
        if( key[KEY_SPACE] )
        {
            last_phase = 0;
            game.setCurrentPhase(last_phase);
            while( !end_game )
            {
                esc_from_game = game.gameMain();
                if( esc_from_game == ESC_FROM_LEVEL )
                    end_game = true;
                else if( last_phase == 3 )
                    end_game = true;
                
                last_phase = game.getCurrentPhase();
            }
            while( key[KEY_ESC] );
            end_game = false;
            if( last_phase == 4 )  // final del juego!
            {
                 stretch_blit(final, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
                 while( !key[KEY_ESC] );
                 while( key[KEY_ESC] );                 
            }
        }
        while( key[KEY_SPACE] );
        blit(buffer, buffer2, 0, 0, 0, 0, SCREEN_X_ORIG, SCREEN_Y_ORIG);        
    	if( ((game.getTimer()->getMsegGlobal()) - msgChk) > 400 )
        {
             pintaLetras = !pintaLetras;
             msgChk = game.getTimer()->getMsegGlobal();
        }
        if( pintaLetras )
            textout_ex(buffer2, myfontCandara, "Press SPACE to play", 310, 520, makecol(255, 0, 0), -1);
        stretch_blit(buffer2, screen, 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);
    }
    
    game.deinit();
    
	return 0;
}
END_OF_MAIN();
