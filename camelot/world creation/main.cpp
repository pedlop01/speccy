#include <allegro.h>
#include <iostream.h>

#include "defines.h"
#include "entradaSortida.h"

#include "mundo.h"
#include "objeto.h"
#include "script.h"
#include "enemigo.h"

#include "symboltable.h"

#include "objecthandler.h"
#include "checkpointhandler.h"
#include "scripthandler.h"
#include "enemyhandler.h"

#define  ABRIR_TODO        0
#define  ABRIR_OBJETOS     1
#define  ABRIR_CHECKPOINTS 2
#define  ABRIR_SCRIPTS     3
#define  ABRIR_ENEMIGOS    4
#define  ABRIR_MAPA        5
#define  ABRIR_MAPA_TE     24

#define  SALVAR_TODO        6
#define  SALVAR_OBJETOS     7
#define  SALVAR_CHECKPOINTS 8
#define  SALVAR_SCRIPTS     9
#define  SALVAR_ENEMIGOS   10
#define  SALVAR_MAPA       11

#define  INSERTAR_OBJETO     12
#define  INSERTAR_CHECKPOINT 13
#define  INSERTAR_SCRIPT     14
#define  INSERTAR_ENEMIGO    15
#define  INSERTAR_INFO_MUNDO 16
#define  INSERTAR_COLISIONES 17

#define  VER_OBJETO          18
#define  VER_CHECKPOINT      19
#define  VER_SCRIPT          20
#define  VER_ENEMIGO         21
#define  VER_INFO_MUNDO      22
#define  VER_COLISIONES      23

// --------------------------------
 
#define MODO_IDLE                    0
#define MODO_INSERTAR_COLISIONES     1
#define MODO_INSERTAR_OBJETO         2
#define MODO_INSERTAR_CHK_1          3
#define MODO_INSERTAR_CHK_2          4
#define MODO_MOVER_CHK               5
#define MODO_INSERTAR_SCRIPT_1       6
#define MODO_INSERTAR_SCRIPT_2       7
#define MODO_MOVER_SCRIPT            8
#define MODO_INSERTAR_ENEMIGO_1      9
#define MODO_INSERTAR_ENEMIGO_2     10
#define MODO_MOVER_ENEMIGO          11

#define SUBMODO_VER_COLISIONES       0x0001
#define SUBMODO_VER_OBJETOS          0x0002
#define SUBMODO_VER_CHECKPOINTS      0x0004
#define SUBMODO_VER_SCRIPTS          0x0008
#define SUBMODO_VER_ENEMIGOS         0x0010

#define SUBMODO_IDLE                 0


// Global variables
fxHandler* fx_handler;
unsigned  current_buffer_num;
BITMAP    *buffer[2];
BITMAP    *screen_buffer[2];
BITMAP* sprite_image;      // FIXME: buscar la forma más eficiente en base a performance!
                           // La mejor solución pasa por tener todos los sprites almacenados
                           // previamente y tirar de ellos cuando sea necesario.

int     correct_scroll_x;
int     correct_scroll_y;

bool     show_cuadricula = false;
bool     insert_col_pintado = false;

bool mapa_cargado = false;

int x = 60;       // posicion de un personaje cuadrado
int y = 300;
int screen_x = 0;
int screen_y = 0;
int tile_x = 0;
int tile_y = 0;
char text_buff[200];
int mouse_b_ant = 0;
int mouse_x_ant = 0;
int mouse_y_ant = 0;
int num_objects = 0;
int num_enemies = 0;

int estado     = MODO_IDLE;
int sub_estado = SUBMODO_IDLE;

////////////////////////////////////////////////////////////////////////////////
// Current variables
mundo      *world   = NULL;
objeto     *object  = NULL;
checkpoint *chk     = NULL;
script     *scrpt   = NULL;
enemigo    *enemy   = NULL;

vector<checkpoint*>::iterator current_chk_it;     // Only valid in MODO_MOVER_CHECKPOINT
list<script*>::iterator       current_script_it;  // Only valid in MODO_MOVER_SCRIPT
vector<enemigo*>::iterator    current_enemy_it;   // Only valid in MODO_MOVER_ENEMIGO

int rect_up_x, rect_up_y;
int rect_down_x, rect_down_y;

objectHandler     *objHandler    = NULL;
checkpointHandler *checkHandler  = NULL;
scriptHandler     *scrptHandler  = NULL;
EnemyHandler      *enemyHandler  = NULL;

symbolTable globalSymbolTable;

////////////////////////////////////////////////////////////////////////////////
// Variable for popups menus
char current_chk_num[16];
char current_chk_warrior_x[16];
char current_chk_warrior_y[16];
char current_chk_next_chk[80];
char current_object_name[80];
char current_script_input_vars[250];
char current_script_output_vars[250];
char current_enemy_id[8];
char current_enemy_type[8];
char current_enemy_estado[8];
char current_enemy_sentido[8];
char current_enemy_sentido_vertical[8];
char current_enemy_vel[8];

////////////////////////////////////////////////////////////////////////////////
// Function protoypes
void init();
void deinit();
void GameMain(int tecla);
int  readKeyboard();


////////////////////////////////////////////////////////////////////////////////
volatile unsigned int mseg;
volatile unsigned int mseg_global;

typedef struct{
    char nombre_menu[50];
    int  id;
} opcion_menu;

void tiempo()
{
   mseg++;
   mseg_global++;
} END_OF_FUNCTION(tiempo);

////////////////////////////////////////////////////////////////////////////////

int my_input_proc(int msg, DIALOG *d, int c)
{
    int tecla = readKeyboard();

    if( (tecla != 0) || 
        (mouse_x_ant != mouse_x) || (mouse_y_ant != mouse_y) || 
        (mouse_b != mouse_b_ant) )
    {
        GameMain(tecla);
    }

    mouse_b_ant = mouse_b;
    mouse_x_ant = mouse_x;
    mouse_y_ant = mouse_y;  
    poll_mouse();
}

int menu_callback()
{
     opcion_menu *op_menu;
     
     op_menu = (opcion_menu *)active_menu->dp;

     cout << "MENU Activated; option = " << active_menu->text << " real action name " << op_menu->nombre_menu << " with id " << op_menu->id << endl;
     
     if( strcmp("Salir", active_menu->text) == 0 )
     {
         exit(0);  // A saco!
     }
     
     if( op_menu->id == INSERTAR_COLISIONES )
     {
         estado = MODO_INSERTAR_COLISIONES;
     }
     else if( op_menu->id == ABRIR_MAPA )
     {
         char path[480];
         if( world != NULL )
         {
             delete world;
             delete objHandler;
             delete checkHandler;
             delete scrptHandler;
             delete enemyHandler;
         }
         sprintf(path, "%s", "../../game/data/maps/");
         if( file_select_ex("Abrir mapa", path, "TXT", 480, 400, 200) != 0 )
         {             
             globalSymbolTable.ClearSymbolTable();
             world = new mundo(path, false);
             objHandler = new objectHandler(&globalSymbolTable);                // Objects info for this world
             checkHandler = new checkpointHandler();                            // Checkpoints info
             scrptHandler = new scriptHandler(&globalSymbolTable);
             enemyHandler = new EnemyHandler(&globalSymbolTable, fx_handler);
             mapa_cargado = true;
         }
     }
     else if( op_menu->id == ABRIR_MAPA_TE )
     {
         char path[480];
         if( world != NULL )
         {
             delete world;
             delete objHandler;
             delete checkHandler;
             delete scrptHandler;
             delete enemyHandler;
         }
         sprintf(path, "%s", "./mapas/");
         if( file_select_ex("Abrir mapa from TE", path, "TXT", 480, 400, 200) != 0 )
         {             
             globalSymbolTable.ClearSymbolTable();
             world = new mundo(path, true);
             objHandler = new objectHandler(&globalSymbolTable);                // Objects info for this world
             checkHandler = new checkpointHandler();                            // Checkpoints info
             scrptHandler = new scriptHandler(&globalSymbolTable);
             enemyHandler = new EnemyHandler(&globalSymbolTable, fx_handler);
             mapa_cargado = true;
         }
     }
     else if( op_menu->id == ABRIR_OBJETOS )
     {
         if( mapa_cargado )
         {
             char path[480];
             sprintf(path, "%s", "../../game/data/objects/");
             if( file_select_ex("Abrir objetos", path, "TXT", 480, 400, 200) != 0 )
             {             
                 delete objHandler;
                 objHandler = new objectHandler(&globalSymbolTable);
                 objHandler->ReadObjectsFromFile(path);
             }             
         }
         else
         {
              alert("Error", "", "Mapa no cargado", "Ok", 0, 0, 0);             
         }
     }    
     else if( op_menu->id == ABRIR_CHECKPOINTS )
     {
         if( mapa_cargado )
         {
             char path[480];
             sprintf(path, "%s", "../../game/data/checkpoints/");
             if( file_select_ex("Abrir checkpoints", path, "TXT", 480, 400, 200) != 0 )
             {             
                 delete checkHandler;
                 checkHandler = new checkpointHandler();  
                 checkHandler->loadCheckpointFile(path);
             }             
         }
         else
         {
              alert("Error", "", "Mapa no cargado", "Ok", 0, 0, 0);             
         }
     }                          
     else if( op_menu->id == ABRIR_SCRIPTS )
     {
         if( mapa_cargado )
         {
             char path[480];
             sprintf(path, "%s", "../../game/data/scripts/");
             if( file_select_ex("Abrir scripts", path, "TXT", 480, 400, 200) != 0 )
             {             
                 delete scrptHandler;
                 scrptHandler = new scriptHandler(&globalSymbolTable);
                 scrptHandler->ReadScriptsFile(path);
             }             
         }
         else
         {
              alert("Error", "", "Mapa no cargado", "Ok", 0, 0, 0);             
         }
     }               
     else if( op_menu->id == ABRIR_ENEMIGOS )
     {
         if( mapa_cargado )
         {
             char path[480];
             sprintf(path, "%s", "../../game/data/characters/");
             if( file_select_ex("Abrir enemigos", path, "TXT", 480, 400, 200) != 0 )
             {             
                 delete enemyHandler;
                 enemyHandler = new EnemyHandler(&globalSymbolTable, fx_handler);
                 enemyHandler->ReadEnemiesFile(path);      
             }             
         }
         else
         {
              alert("Error", "", "Mapa no cargado", "Ok", 0, 0, 0);             
         }
     }     
     else if( op_menu->id == SALVAR_MAPA )
     {
         char path[480]; 
         sprintf(path, "%s", "./mapas/");
         if( mapa_cargado )
         {
             if( file_select_ex("Grabar mapa", path, "TXT", 480, 400, 200) != 0 )
             {       
                 cout << " WARNING: output file is not readable by this program!" << endl;
                 world->SaveWorldInFile(path);
             }
         }
     }
     else if( op_menu->id == SALVAR_OBJETOS )
     {
         char path[480]; 
         sprintf(path, "%s", "./objects/");
         if( mapa_cargado )
         {
             if( file_select_ex("Grabar objects", path, "TXT", 480, 400, 200) != 0 )
             {       
                 objHandler->SaveObjectsToFile(path);
             }
         }
     }     
     else if( op_menu->id == SALVAR_CHECKPOINTS )
     {
         char path[480]; 
         sprintf(path, "%s", "./checkpoints/");
         if( mapa_cargado )
         {
             if( file_select_ex("Grabar checkpoints", path, "TXT", 480, 400, 200) != 0 )
             {       
                 checkHandler->saveCheckpointFile(path);
             }
         }
     }        
     else if( op_menu->id == SALVAR_SCRIPTS )
     {
         char path[480]; 
         sprintf(path, "%s", "./scripts/");
         if( mapa_cargado )
         {
             if( file_select_ex("Grabar scripts", path, "TXT", 480, 400, 200) != 0 )
             {       
                scrptHandler->SaveScriptsFile(path);
             }
         }
     }    
     else if( op_menu->id == SALVAR_ENEMIGOS )
     {
         char path[480]; 
         sprintf(path, "%s", "./enemies/");
         if( mapa_cargado )
         {
             if( file_select_ex("Grabar enemies", path, "TXT", 480, 400, 200) != 0 )
             {       
                 enemyHandler->SaveEnemiesFile(path);
             }
         }
     }                    
     else if( op_menu->id == INSERTAR_OBJETO )
     {
          if( !mapa_cargado )
          {
              alert("Error", "", "Mapa no cargado", "Ok", 0, 0, 0);
          }
          else if( estado == MODO_INSERTAR_OBJETO )
          {
              alert("Error", "", "Ya estas insertando un objeto", "Ok", 0, 0, 0);
          }          
          else
          {
              char path[480]; 
              sprintf(path, "%s", "../../game/data/objects/");          
              
              if( file_select_ex("Abrir objeto", path, "TXT", 480, 400, 200) != 0 )
              {
                  char aux[255];
                  sprintf(aux, "object_n_%d", num_objects);
                  string objName(aux);  
                  num_objects++;
                  estado = MODO_INSERTAR_OBJETO;
    
                  // Abrir objeto y crear uno nuevo
                  object = new objeto(&globalSymbolTable, 0, mouse_x, mouse_y, true, objName.c_str(), path);                  
              }
          }
     }
     else if( op_menu->id == INSERTAR_ENEMIGO )
     {
          if( estado == MODO_INSERTAR_ENEMIGO_1 || estado == MODO_INSERTAR_ENEMIGO_2 )
          {
              alert("Error", "", "Ya estas insertando un enemigo", "Ok", 0, 0, 0);
          }          
          else
          {
              char path[480]; 
              sprintf(path, "%s", "../../game/data/characters/");          
              
              if( file_select_ex("Abrir enemigo", path, "TXT", 480, 400, 200) != 0 )
              {
                  num_enemies++;
                  estado = MODO_INSERTAR_ENEMIGO_1;
    
                  // Abrir objeto y crear uno nuevo
                  enemy = new enemigo(&globalSymbolTable, fx_handler, 0, 0, 0, 0, 0, mouse_x, mouse_y, 0, 0, 0, path);
              }
          }
     }     
     else if( op_menu->id == INSERTAR_CHECKPOINT )
     {
          estado = MODO_INSERTAR_CHK_1;
     }
     else if( op_menu->id == INSERTAR_SCRIPT )
     {
          estado = MODO_INSERTAR_SCRIPT_1;
     }
     else if( op_menu->id == VER_COLISIONES )
     {
         sub_estado ^= SUBMODO_VER_COLISIONES;
         active_menu->flags ^= D_SELECTED;               
     }          
     else if( op_menu->id == VER_OBJETO )
     {         
         sub_estado ^= SUBMODO_VER_OBJETOS;  
         active_menu->flags ^= D_SELECTED;         
     }
     else if( op_menu->id == VER_CHECKPOINT )
     {
         sub_estado ^= SUBMODO_VER_CHECKPOINTS;  
         active_menu->flags ^= D_SELECTED;                  
     }
     else if( op_menu->id == VER_SCRIPT )
     {
         sub_estado ^= SUBMODO_VER_SCRIPTS;  
         active_menu->flags ^= D_SELECTED;                           
     }
     else if( op_menu->id == VER_ENEMIGO )
     {
         sub_estado ^= SUBMODO_VER_ENEMIGOS;
         active_menu->flags ^= D_SELECTED;               
     }          
     else
     {
         estado = MODO_IDLE;
     }
     
     return 0;     
}

opcion_menu opciones_menu[] =
{
    {"Abrir_Todo", 0},
    {"Abrir_Objectos", 1},
    {"Abrir_Checkpoints", 2},
    {"Abrir_Scripts", 3},
    {"Abrir_Enemigos", 4},
    {"Abrir_Mapa", 5},
    {"Salvar_Todo", 6},
    {"Salvar_Objectos", 7},
    {"Salvar_Checkpoints", 8},
    {"Salvar_Scripts", 9},
    {"Salvar_Enemigos", 10},
    {"Salvar_Mapa", 11},  
    {"Insertar_Objeto", 12},
    {"Insertar_Checkpoint", 13},      
    {"Insertar_Script", 14},      
    {"Insertar_Enemigo", 15},      
    {"Insertar_Info_Mundo", 16},                  
    {"Insertar_Colisiones", 17}, 
    {"Ver_Objeto", 18},
    {"Ver_Checkpoint", 19},      
    {"Ver_Script", 20},      
    {"Ver_Enemigo", 21},      
    {"Ver_Info_Mundo", 22},                  
    {"Ver_Colisiones", 23},             
    {"Abrir_Mapa_T_E", 24},
    {"Salir", 25},
};

MENU menu_abrir[] =
{
     { "Todo",         menu_callback, NULL, 0, &(opciones_menu[0])},
     { "Objetos",      menu_callback, NULL, 0, &(opciones_menu[1])},
     { "Checkpoints",  menu_callback, NULL, 0, &(opciones_menu[2])},
     { "Scripts",      menu_callback, NULL, 0, &(opciones_menu[3])},
     { "Enemigos",     menu_callback, NULL, 0, &(opciones_menu[4])},     
     { "Mapa",         menu_callback, NULL, 0, &(opciones_menu[5])},          
     { "Mapa_TE",      menu_callback, NULL, 0, &(opciones_menu[24])},          
     { NULL,           NULL, NULL, 0, NULL}     
};

MENU menu_salvar[] =
{
     { "Todo",         menu_callback, NULL, 0, &(opciones_menu[6])},
     { "Objetos",      menu_callback, NULL, 0, &(opciones_menu[7])},
     { "Checkpoints",  menu_callback, NULL, 0, &(opciones_menu[8])},
     { "Scripts",      menu_callback, NULL, 0, &(opciones_menu[9])},
     { "Enemigos",     menu_callback, NULL, 0, &(opciones_menu[10])},     
     { "Mapa",         menu_callback, NULL, 0, &(opciones_menu[11])},          
     { NULL,           NULL, NULL, 0, NULL}     
};

MENU menu_archivo[] =
{
     { "Abrir",  NULL, menu_abrir,  0, NULL},
     { "Salvar", NULL, menu_salvar, 0, NULL},
     { "Salir",  menu_callback, NULL, 0, &(opciones_menu[24])},
     { NULL,     NULL, NULL, 0, NULL}     
};

MENU menu_insertar[] =
{
     { "Objeto",     menu_callback, NULL, 0, &(opciones_menu[12])},
     { "Checkpoint", menu_callback, NULL, 0, &(opciones_menu[13])},
     { "Script",     menu_callback, NULL, 0, &(opciones_menu[14])},
     { "Enemigo",    menu_callback, NULL, 0, &(opciones_menu[15])},     
     { "Info Mundo", menu_callback, NULL, 0, &(opciones_menu[16])}, 
     { "Colisiones", menu_callback, NULL, 0, &(opciones_menu[17])},
     { NULL,         NULL, NULL, 0, NULL}         
};

MENU menu_ver[] =
{
     { "Objectos",    menu_callback, NULL, 0, &(opciones_menu[18])},
     { "Checkpoints", menu_callback, NULL, 0, &(opciones_menu[19])},
     { "Scripts",     menu_callback, NULL, 0, &(opciones_menu[20])},
     { "Enemigos",    menu_callback, NULL, 0, &(opciones_menu[21])},     
     { "Info Mundo",  menu_callback, NULL, 0, &(opciones_menu[22])},   
     { "Colisiones",  menu_callback, NULL, 0, &(opciones_menu[23])},       
     { NULL,          NULL, NULL, 0, NULL}          
};

/* the main menu-bar */
MENU the_menu[] =
{
   { "&Archivo",  NULL,   menu_archivo,   0,      NULL  },
   { "&Insertar", NULL,   menu_insertar,  0,      NULL  },
   { "&Ver",      NULL,   menu_ver,       0,      NULL  },
   { "&Ayuda",    NULL,   NULL,           0,      NULL  },   
   { NULL,        NULL,   NULL,           0,      NULL  }
};

DIALOG the_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */       
   
   /* this element just clears the screen, therefore it should come before the others */
   { d_clear_proc,        0,   0,    0,    0,   0,  0,    0,      0,       0,   0,    NULL,                   NULL, NULL  },
   
   /* note how we don't fill in the dp field yet, because we first need to load the bitmap */
   { d_bitmap_proc,       0,   0,   SCREEN_X,  SCREEN_Y,   0,  0,    0,      0,       0,      0,    NULL,                               NULL, NULL  },   
   { d_menu_proc,         0,   0,          0,         0,   0,  0,    0,      0,       0,      0,    the_menu,                           NULL, NULL  },
   { my_input_proc,       0,   0,          0,         0,   0,  0,    0,      0,       0,      0,    NULL,                               NULL, NULL  },      
   { d_yield_proc,        0,   0,          0,         0,   0,  0,    0,      0,       0,      0,    NULL,                               NULL, NULL  },
   { NULL,                0,   0,          0,         0,   0,  0,    0,      0,       0,      0,    NULL,                               NULL, NULL  }
};

DIALOG edit_object_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */       
   
   /* this element just clears the screen, therefore it should come before the others */   
   { d_box_proc,          0,   0,  252,   52,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { d_text_proc,        10,  10,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Name = ",  NULL, NULL  },   
   { d_edit_proc,        66,  10,  160,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_object_name,      NULL, NULL  },   
   { d_button_proc,      46,  30,  160,   16,   0,  0,    0, D_EXIT,       0,   0,    (void*)"OK",            NULL, NULL  },   
   { d_yield_proc,        0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { NULL,                0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  }       
};

DIALOG edit_checkpoint_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */       
   
   /* this element just clears the screen, therefore it should come before the others */   
   { d_box_proc,          0,   0,  252,   120,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { d_text_proc,        10,  10,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Number = ",  NULL, NULL  },   
   { d_edit_proc,        86,  10,  140,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_chk_num,      NULL, NULL  },   
   { d_text_proc,        10,  30,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"WarriorX = ",  NULL, NULL  },   
   { d_edit_proc,        106, 30,  140,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_chk_warrior_x,      NULL, NULL  },   
   { d_text_proc,        10,  50,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"WarriorY = ",  NULL, NULL  },   
   { d_edit_proc,        106, 50,  140,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_chk_warrior_y,      NULL, NULL  },   
   { d_text_proc,        10,  70,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"NextChecks = ",  NULL, NULL  },   
   { d_edit_proc,        126, 70,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_chk_next_chk,      NULL, NULL  },   
   { d_button_proc,      60,  90,  140,   16,   0,  0,    0, D_EXIT,       0,   0,    (void*)"OK",            NULL, NULL  },   
   { d_yield_proc,        0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { NULL,                0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  }       
};

DIALOG edit_script_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */       
   
   /* this element just clears the screen, therefore it should come before the others */   
   { d_box_proc,          0,   0,  652,   100,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { d_text_proc,        10,  20,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"InputVars = ",  NULL, NULL  },   
   { d_edit_proc,        126, 20,  500,    8,         0,  0,   0, D_EXIT,    250,       0,    (void*)current_script_input_vars,      NULL, NULL  },   
   { d_text_proc,        10,  40,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"OutputVars = ",  NULL, NULL  },   
   { d_edit_proc,        126, 40,  500,    8,         0,  0,   0, D_EXIT,     250,       0,    (void*)current_script_output_vars,      NULL, NULL  },   
   { d_button_proc,      270,  60,  140,   16,   0,  0,    0, D_EXIT,       0,   0,    (void*)"OK",            NULL, NULL  },   
   { d_yield_proc,        0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { NULL,                0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  }       
};

DIALOG edit_enemy_dialog[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */       
   
   /* this element just clears the screen, therefore it should come before the others */   
   { d_box_proc,          0,   0,  252,   150,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { d_text_proc,        10,  10,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Id      = ",  NULL, NULL  },   
   { d_edit_proc,        116, 10,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_id,      NULL, NULL  },      
   { d_text_proc,        10,  30,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Type      = ",  NULL, NULL  },   
   { d_edit_proc,        116, 30,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_type,      NULL, NULL  },   
   { d_text_proc,        10,  50,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Estado    = ",  NULL, NULL  },   
   { d_edit_proc,        116, 50,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_estado,      NULL, NULL  },   
   { d_text_proc,        10,  70,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Sentido   = ",  NULL, NULL  },   
   { d_edit_proc,        116, 70,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_sentido,      NULL, NULL  },   
   { d_text_proc,        10,  90,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"SentidoV  = ",  NULL, NULL  },   
   { d_edit_proc,        116, 90,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_sentido_vertical,      NULL, NULL  },   
   { d_text_proc,        10,  110,    0,    0,   0,  0,    0,      0,       0,   0,    (void*)"Velocidad = ",  NULL, NULL  },   
   { d_edit_proc,        116, 110,  110,    8,         0,  0,   0, D_EXIT,      64,       0,    (void*)current_enemy_vel,      NULL, NULL  },      
   { d_button_proc,      60, 130,  126,   16,   0,  0,    0, D_EXIT,       0,   0,    (void*)"OK",            NULL, NULL  },   
   { d_yield_proc,        0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  },
   { NULL,                0,   0,    0,    0,         0,  0,   0,      0,       0,       0,                 NULL,      NULL, NULL  }       
};



////////////////////////////////////////////////////////////////////////////////

void scrollHandler(mundo *world, int entrada_teclado, int &screen_x, int &screen_y)
{        
    if( entrada_teclado & 0x0001 )   // LEFT
    {
        screen_x--;        
    }
    if( entrada_teclado & 0x0002 )   // RIGHT
    {
        screen_x++;
    }
    if( entrada_teclado & 0x0004 )   // UP
    {
        screen_y--;
    }
    if( entrada_teclado & 0x0008 )   // DOWN
    {
        screen_y++;
    }
    
    if( screen_x <= 0 )
    {
        screen_x = 0;
    }
    else if( screen_x > (world->GetScreenNewX() - (SCREEN_X_ORIG)) ) // FIXME
    {
        screen_x = world->GetScreenNewX() - (SCREEN_X_ORIG);        
    }

    if( screen_y <= 0 )
    {
        screen_y = 0;
    }
    else if( screen_y > (world->GetScreenNewY() - (SCREEN_Y_ORIG)) ) // FIXME
    {
        screen_y = world->GetScreenNewY() - (SCREEN_Y_ORIG);
    }
}

void drawWorld(mundo *world, BITMAP *buffer, int screenIniX, int screenIniY)
{
     int     i2, j2;
     int     tiles_in_screen_x;
     int     tiles_in_screen_y;
     int     tiles_in_tiles_image_x;
     int     first_tile_screen_x;
     int     first_tile_screen_y;

     tiles_in_tiles_image_x = world->GetScreenTilesX() / world->GetTilesSizeX();
     tiles_in_screen_x = (SCREEN_X_ORIG / world->GetTilesSizeX()) + (screenIniX < world->GetScreenNewX() - SCREEN_X_ORIG);
     tiles_in_screen_y = (SCREEN_Y_ORIG / world->GetTilesSizeY()) + (screenIniY < world->GetScreenNewY() - SCREEN_Y_ORIG);
     if( tiles_in_screen_x > world->GetTilesWorldX() )
     {
         tiles_in_screen_x = world->GetTilesWorldX();
     }
     if( tiles_in_screen_y > world->GetTilesWorldY() )
     {
         tiles_in_screen_y = world->GetTilesWorldY();
     }

     first_tile_screen_x = screenIniX / world->GetTilesSizeX();
     first_tile_screen_y = screenIniY / world->GetTilesSizeY();
     correct_scroll_x    = screenIniX % world->GetTilesSizeX();
     correct_scroll_y    = screenIniY % world->GetTilesSizeY();
#if 0
     // Scrolls
     for( int i = 0 ; i < world->GetNumScrollPlanes() ; i++ )
     {
            int tiles_scroll_i_x   = (SCREEN_X_ORIG / world->GetScrollSizeX(i)) + 1;
            int tiles_scroll_i_y   = (SCREEN_Y_ORIG / world->GetScrollSizeY(i)) + 1;
            int correct_scroll_i_x = (screenIniX / (i + 2)) % world->GetScrollSizeX(i);
            int correct_scroll_i_y = (screenIniY / (i + 2)) % world->GetScrollSizeY(i);
            for( int r = 0 ; r < tiles_scroll_i_x ; r++ )
            {
                for( int s = 0 ; s < tiles_scroll_i_y ; s++ )
                {
                    blit(world->GetScrollImage(i), buffer,
                         0, 0,
                         r * world->GetScrollSizeX(i) - correct_scroll_i_x, s * world->GetScrollSizeY(i) - correct_scroll_i_y ,
                         world->GetScrollSizeX(i), world->GetScrollSizeY(i));
                }
            }
     }
#endif
     i2 = 0;
     for( int i = first_tile_screen_y ; i < first_tile_screen_y + tiles_in_screen_y ; i++ )
     {
         j2 = 0;
         for( int j = first_tile_screen_x ; j < first_tile_screen_x + tiles_in_screen_x ; j++ )
         {
                int x = (world->GetTileValue(j, i)- 1) % tiles_in_tiles_image_x;
                int y = (world->GetTileValue(j, i) - 1)/ tiles_in_tiles_image_x;

                if( world->GetTileValue(j, i) != 29 ) //FIXME erase this and put transparent colors!
                {
                     blit(world->GetWorldImage(), sprite_image, x * world->GetTilesSizeX(), y * world->GetTilesSizeY(), 0, 0, world->GetTilesSizeX(), world->GetTilesSizeY());
                     blit(sprite_image, buffer,
                          0, 0,
                          j2 * world->GetTilesSizeX() - correct_scroll_x, i2 * world->GetTilesSizeY() - correct_scroll_y,
                          world->GetTilesSizeX(), world->GetTilesSizeY());
                }
                if( (world->GetTile(j, i)->GetCollisionable() || world->GetTile(j, i)->GetPintado()) && ((sub_estado & SUBMODO_VER_COLISIONES) || estado == MODO_INSERTAR_COLISIONES) )
                {
                     int red = makecol(255,0,0);
                     char text[50];
                     sprintf(text, "%d", world->GetTile(j, i)->GetCollisionable());
                     textout_ex(buffer, font, text, j2 * world->GetTilesSizeX() - correct_scroll_x, i2 * world->GetTilesSizeY() - correct_scroll_y, 0xABCDE, -1);  
                     sprintf(text, "%d", world->GetTile(j, i)->GetPintado());
                     textout_ex(buffer, font, text, j2 * world->GetTilesSizeX() - correct_scroll_x + 20, i2 * world->GetTilesSizeY() - correct_scroll_y, red, -1);                       
                }
                j2++;
         }
         i2++;
     }

     // cuadricula
     if( show_cuadricula )
     {
         for( int l = 0 ; l < tiles_in_screen_x ; l++ )
         {
             line(buffer, l * world->GetTilesSizeX(), 0, l * world->GetTilesSizeX(), SCREEN_Y, 0xFFFF);
         }
         for( int l = 0 ; l < tiles_in_screen_x ; l++ )
         {
             line(buffer, 0, l * world->GetTilesSizeY(), SCREEN_X, l * world->GetTilesSizeX(), 0xFFFF);            
         }
    }
}

int readKeyboard()
{
    int entrada_teclado = 0x0000;

    if( key[KEY_LEFT] )
    {
        entrada_teclado |= 0x0001;
    }
    if( key[KEY_RIGHT] )
    {
        entrada_teclado |= 0x0002;
    }
    if( key[KEY_UP] )
    {
        entrada_teclado |= 0x0004;
    }
    if( key[KEY_DOWN] )
    {
        entrada_teclado |= 0x0008;
    }
    if( key[KEY_S] )
    {
        entrada_teclado |= 0x0010;        
    }
    if( key[KEY_M] )
    {
        entrada_teclado |= 0x0020;
    }
    if( key[KEY_1] )
    {
        entrada_teclado |= 0x0040;        
    }
    if( key[KEY_2] )
    {
        entrada_teclado |= 0x0080;        
    }    
    if( key[KEY_3] )
    {
        entrada_teclado |= 0x0100;        
    }        
    if( key[KEY_C] )
    {
        if( show_cuadricula )
            show_cuadricula = false;
        else
            show_cuadricula = true;
        while( key[KEY_C] );
    }
    if( key[KEY_LCONTROL] )
    {
        entrada_teclado |= 0x1000;
        if( insert_col_pintado )
            insert_col_pintado = false;
        else
            insert_col_pintado = true;
        while( key[KEY_LCONTROL] );        
    }

    return entrada_teclado;
}

void GameMain(int entrada_teclado)
{            
        if( world == NULL )
        {
            return;
        }   

        mseg = 0;
        acquire_bitmap(buffer[current_buffer_num]);
        acquire_bitmap(screen_buffer[current_buffer_num]);
        clear_bitmap(buffer[current_buffer_num]);
        clear_bitmap(screen_buffer[current_buffer_num]);
                
        ///////////////////////////////////////////////////////////////////////
        // PROCESAMIENTO
        for( int i = 0 ; i < world->GetTilesSizeX() ; i++ )  // VELOCIDAD SCROLL
            scrollHandler(world, entrada_teclado, screen_x, screen_y);
            
        if( estado == MODO_INSERTAR_COLISIONES )
        {
            if( (mouse_b & 0x0001) && (entrada_teclado == 0x0000) )
            {
                if( insert_col_pintado )
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetPintado(1);
                }
                else
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetCollisionable(1);
                }
            }
            if( (mouse_b & 0x0001) && (entrada_teclado == 0x0040) )
            {
                if( insert_col_pintado )
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetPintado(2);                    
                }
                else
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetCollisionable(2);
                }
            }
            if( (mouse_b & 0x0001) && (entrada_teclado == 0x0080) )
            {
                if( insert_col_pintado )
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetPintado(3);  
                }
                else
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetCollisionable(3);
                }
            }            
            if( mouse_b & 0x0002 )
            {
                if( insert_col_pintado )
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetPintado(0);                    
                }
                else
                {
                    world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->SetCollisionable(0);            
                }
            }
            if( entrada_teclado & 0x1000 )
            {
                printf("Cambiando de modo en insertar colisiones");
            }
        }      
        else if( estado == MODO_INSERTAR_OBJETO )
        {
            if( mouse_b & 0x0001 )
            {
                estado = MODO_IDLE;
                objHandler->AddNewObject(object);
            }
            if( mouse_b & 0x0002 )
            {
                estado = MODO_IDLE;
                globalSymbolTable.EraseElement(object->GetName() + "_picked");
                delete object;
            }
        }
        else if( estado == MODO_INSERTAR_CHK_1 )
        {
             if( mouse_b & 0x0001 )
             {
                 estado = MODO_INSERTAR_CHK_2;
                 rect_up_x = mouse_x + screen_x;
                 rect_up_y = mouse_y + screen_y;
             }
        }
        else if( estado == MODO_INSERTAR_CHK_2 )
        {
             if( mouse_b & 0x0001 )
             {                 
                 estado = MODO_IDLE;
                 rect_down_x = mouse_x + screen_x;
                 rect_down_y = mouse_y + screen_y;   
                 chk = new checkpoint();                   // Current checkpoint       
                 chk->num = checkHandler->GetNumCheckpoints();
                 chk->x_izq = rect_up_x;
                 chk->y_izq = rect_up_y;
                 chk->x_der = rect_down_x;
                 chk->y_der = rect_down_y;
                 chk->warrior_ini_x = rect_up_x;
                 chk->warrior_ini_y = rect_up_y;
                 checkHandler->AddNewCheckpoint(chk);
             }
             else if( mouse_b & 0x0002 )
             {
                 estado = MODO_INSERTAR_CHK_1;
             }
        }  
        else if( estado == MODO_INSERTAR_SCRIPT_1 )
        {
             if( mouse_b & 0x0001 )
             {
                 estado = MODO_INSERTAR_SCRIPT_2;
                 rect_up_x = mouse_x + screen_x;
                 rect_up_y = mouse_y + screen_y;
             }
        }
        else if( estado == MODO_INSERTAR_SCRIPT_2 )
        {
             if( mouse_b & 0x0001 )
             {                 
                 estado = MODO_IDLE;
                 rect_down_x = mouse_x + screen_x;
                 rect_down_y = mouse_y + screen_y;   
                 scrpt = new script(&globalSymbolTable);
                 scrpt->SetX(rect_up_x);
                 scrpt->SetY(rect_up_y);
                 scrpt->SetSizeX(abs(rect_up_x - rect_down_x));
                 scrpt->SetSizeY(abs(rect_up_y - rect_down_y));
                 scrptHandler->AddNewScript(scrpt);                 
             }
             else if( mouse_b & 0x0002 )
             {
                 estado = MODO_INSERTAR_SCRIPT_1;
             }
        }         
        else if( estado == MODO_INSERTAR_ENEMIGO_1 )
        {
            if( mouse_b & 0x0001 )
            {
                rect_up_x = mouse_x + screen_x;
                rect_up_y = mouse_y + screen_y;
                estado    = MODO_INSERTAR_ENEMIGO_2;
            }
            if( mouse_b & 0x0002 )
            {
                estado = MODO_IDLE;                
                delete enemy;
            }
        }        
        else if( estado == MODO_INSERTAR_ENEMIGO_2 )
        {
             if( mouse_b & 0x0001 )
             {                                 
                 estado = MODO_IDLE;
                 rect_down_x = mouse_x + screen_x;
                 rect_down_y = mouse_y + screen_y;   
                                  
                 enemy->SetIniX(rect_up_x);
                 enemy->SetIniY(rect_up_y);
                 enemy->SetCurrentX(rect_up_x);
                 enemy->SetCurrentY(rect_up_y);
                 enemy->SetDespX(rect_down_x - rect_up_x);
                 enemy->SetDespY(rect_down_y - rect_up_y);             
                 enemy->SetIniSentido(1);
                 enemy->SetIniSentidoVertical(1);

                 enemy->SetStateToInitial();
               
                 enemyHandler->AddNewEnemy(enemy);                 
             }
             else if( mouse_b & 0x0002 )
             {
                 estado = MODO_INSERTAR_SCRIPT_1;
             }
        }                  
        else if( estado == MODO_MOVER_CHK )
        {
             chk->x_izq += (mouse_x - mouse_x_ant);
             chk->x_der += (mouse_x - mouse_x_ant);
             chk->y_izq += (mouse_y - mouse_y_ant);
             chk->y_der += (mouse_y - mouse_y_ant);
             if( mouse_b & 0x0001 )
             {
                 estado = MODO_IDLE;
             }
             else if( mouse_b & 0x0002 )
             {
                  checkpoint* aux = *current_chk_it;
                  estado = MODO_IDLE;
                  checkHandler->GetCheckpoints()->erase(current_chk_it);
                  delete aux;                  
             }
        }              
        else if( estado == MODO_MOVER_SCRIPT )
        {
             scrpt->SetX(scrpt->GetX() + (mouse_x - mouse_x_ant));
             scrpt->SetY(scrpt->GetY() + (mouse_y - mouse_y_ant));             
             if( mouse_b & 0x0001 )
             {
                 estado = MODO_IDLE;
             }
             else if( mouse_b & 0x0002 )
             {
                  script* aux = *current_script_it;
                  estado = MODO_IDLE;
                  scrptHandler->GetScripts()->erase(current_script_it);
                  delete aux;                  
             }
        }                      
        else if( estado == MODO_MOVER_ENEMIGO )
        {
             enemy->SetIniX(enemy->GetIniX() + (mouse_x - mouse_x_ant));
             enemy->SetIniY(enemy->GetIniY() + (mouse_y - mouse_y_ant));             
             enemy->SetCurrentX(enemy->GetIniX() + (mouse_x - mouse_x_ant));
             enemy->SetCurrentY(enemy->GetIniY() + (mouse_y - mouse_y_ant));                          
             enemy->SetStateToInitial();
             if( mouse_b & 0x0001 )
             {
                 estado = MODO_IDLE;
             }
             else if( mouse_b & 0x0002 )
             {
                  enemigo* aux = *current_enemy_it;
                  estado = MODO_IDLE;
                  enemyHandler->GetEnemies()->erase(current_enemy_it);
                  delete aux;                  
             }
        }                              
        else if( (sub_estado & SUBMODO_VER_OBJETOS) || (sub_estado & SUBMODO_VER_CHECKPOINTS) || (sub_estado & SUBMODO_VER_SCRIPTS) || (sub_estado & SUBMODO_VER_ENEMIGOS) )
        {
             bool object_management = false;
             bool check_management  = false;
             bool script_management = false;
             if( (mouse_b & 0x0001) && (sub_estado & SUBMODO_VER_OBJETOS) )
             {
                  vector<objeto*>::iterator obj_it = objHandler->GetObjectInWorldByCoords(world, mouse_x, mouse_y, screen_x, screen_y);
                  if( obj_it !=  objHandler->GetObjects()->end() )
                  {
                      object = *obj_it;
                      estado = MODO_INSERTAR_OBJETO;
                      objHandler->GetObjects()->erase(obj_it);                      
                      object_management = true;
                  }
             }
             if( (mouse_b & 0x0002) && (sub_estado & SUBMODO_VER_OBJETOS) )
             {
                 vector<objeto*>::iterator obj_it = objHandler->GetObjectInWorldByCoords(world, mouse_x, mouse_y, screen_x, screen_y);
                 if( obj_it !=  objHandler->GetObjects()->end() )
                 {        
                     set_dialog_color(edit_object_dialog, makecol(0, 0, 0), makecol(255, 255, 255));
                     sprintf(current_object_name, "%s", (*obj_it)->GetName().c_str());
                     centre_dialog(edit_object_dialog);
                     gui_mg_color = makecol(128, 128, 128);             
                     do_dialog(edit_object_dialog, 2);
                     unsigned long* ptr = globalSymbolTable.GetPointerByName((*obj_it)->GetName() + "_picked");
                     globalSymbolTable.EraseElement((*obj_it)->GetName() + "_picked");  
                     string new_name(current_object_name);
                     (*obj_it)->SetName(new_name);
                     new_name += "_picked";
                     globalSymbolTable.addVarToSymbolTable(new_name.c_str(), ptr);
                     object_management = true;                     
                 }
             }
             if( (!object_management) && (sub_estado & SUBMODO_VER_CHECKPOINTS) )
             {
                bool trobat = false;
                checkpoint* check;
                for( vector<checkpoint*>::iterator it = checkHandler->GetCheckpoints()->begin() ; (it != checkHandler->GetCheckpoints()->end() && !trobat) ; it++ )
                {
                     check = *it;
                     current_chk_it = it;
                     
                     if( (mouse_x >= check->x_izq - screen_x) && (mouse_x <= check->x_der - screen_x) && 
                         (mouse_y >= check->y_izq - screen_y) && (mouse_y <= check->y_der - screen_y) )
                     {
                         trobat = true;
                     }
                }             
                if( trobat == true )
                {
                    if( mouse_b & 0x0001 )
                    {
                        estado = MODO_MOVER_CHK;
                        chk = check;
                        check_management = true;
                    }
                    else if( mouse_b & 0x0002 )
                    {
                         chk = check;
                         
                         char *chk_ptr;
                         set_dialog_color(edit_checkpoint_dialog, makecol(0, 0, 0), makecol(255, 255, 255));
                         sprintf(current_chk_num, "%d", chk->num);
                         sprintf(current_chk_warrior_x, "%d", chk->warrior_ini_x);  
                         sprintf(current_chk_warrior_y, "%d", chk->warrior_ini_y);
                         sprintf(current_chk_next_chk, "");
                         for( list<int>::iterator it = chk->nextCheckpoint.begin() ; it != chk->nextCheckpoint.end() ; it++ )
                         {
                              sprintf(current_chk_next_chk, "%s%d,", current_chk_next_chk, *it);                          
                         }
                         centre_dialog(edit_checkpoint_dialog);
                         gui_mg_color = makecol(128, 128, 128);             
                         do_dialog(edit_checkpoint_dialog, 2);                     
                         chk->warrior_ini_x = atoi(current_chk_warrior_x);
                         chk->warrior_ini_y = atoi(current_chk_warrior_y);
                         chk->nextCheckpoint.clear();
                         chk_ptr = strtok(current_chk_next_chk, ",");
                         while(chk_ptr != NULL)
                         {
                             chk->nextCheckpoint.push_back(atoi(chk_ptr));
                             chk_ptr = strtok(NULL, ",");
                         }
                         check_management = true;
                    }
                 } 
             }
             if( (!object_management) && (!check_management) && (sub_estado & SUBMODO_VER_SCRIPTS) )
             {
                bool trobat = false;
                script* scrpt_aux;
                for( list<script*>::iterator it = scrptHandler->GetScripts()->begin() ; (it != scrptHandler->GetScripts()->end() && !trobat) ; it++ )
                {
                     scrpt_aux = *it;
                     current_script_it = it;
                     
                     if( (mouse_x >= scrpt_aux->GetX() - screen_x) && (mouse_x <= (scrpt_aux->GetX() + scrpt_aux->GetSizeX()) - screen_x) && 
                         (mouse_y >= scrpt_aux->GetY() - screen_y) && (mouse_y <= (scrpt_aux->GetY() + scrpt_aux->GetSizeY()) - screen_y)  )
                     {
                         trobat = true;
                     }
                }             
                if( trobat == true )
                {
                    if( mouse_b & 0x0001 )
                    {
                        estado = MODO_MOVER_SCRIPT;
                        scrpt = scrpt_aux;
                        script_management = true;
                    }
                    else if( mouse_b & 0x0002 )
                    {
                         char* char_ptr;
                         char  input_vars[250];
                         char  output_vars[250];
                         scrpt = scrpt_aux;
                         set_dialog_color(edit_script_dialog, makecol(0, 0, 0), makecol(255, 255, 255));
                         centre_dialog(edit_script_dialog);
                         gui_mg_color = makecol(128, 128, 128);                                      
                         // Obtener input and output vars from current_script
                         sprintf(input_vars, "");
                         for( map<string, unsigned int>::iterator it = scrpt->GetInputVars()->begin() ; it != scrpt->GetInputVars()->end() ; it++ )
                         {
                              sprintf(input_vars, "%s%s %d;", input_vars, it->first.c_str(), it->second);
                         }
                         sprintf(current_script_input_vars, "%s", input_vars); // viene a ser un strcpy
                         sprintf(output_vars, "");
                         for( map<string, unsigned int>::iterator it = scrpt->GetOutputVars()->begin() ; it != scrpt->GetOutputVars()->end() ; it++ )
                         {
                              sprintf(output_vars, "%s%s %d;", output_vars, it->first.c_str(), it->second);
                         }
                         sprintf(current_script_output_vars, "%s", output_vars); // viene a ser un strcpy                         
                         do_dialog(edit_script_dialog, 2);
                         // restaurar script input and output vars
                         scrpt->GetInputVars()->clear();
                         char_ptr = strtok(current_script_input_vars, " ;");
                         while(char_ptr != NULL)
                         {
                             // tengo el nombre
                             string name(char_ptr);                             
                             char_ptr = strtok(NULL, " ;");
                             // tengo el valor
                             scrpt->addNewInputVar(name, atoi(char_ptr));
                             char_ptr = strtok(NULL, " ;");
                         }
                         scrpt->GetOutputVars()->clear();
                         char_ptr = strtok(current_script_output_vars, " ;");
                         while(char_ptr != NULL)
                         {
                             // tengo el nombre
                             string name(char_ptr);                             
                             char_ptr = strtok(NULL, " ;");
                             // tengo el valor
                             scrpt->addNewOutputVar(name, atoi(char_ptr));
                             char_ptr = strtok(NULL, " ;");
                         }
                         script_management = true;
                    }
                 }   
             }
             if( (!object_management) && (!check_management) && (!script_management) && (sub_estado & SUBMODO_VER_ENEMIGOS) )
             {
                bool trobat = false;
                enemigo* enemy_aux;
                
                for( vector<enemigo*>::iterator it = enemyHandler->GetEnemies()->begin() ; (it != enemyHandler->GetEnemies()->end()) && !trobat ; it++ )
                {
                    enemy_aux = *it;
                    current_enemy_it = it;
                    int x1, x2;
                    int y1, y2;
                    int desp_x = enemy_aux->GetDespX();
                    int desp_y = enemy_aux->GetDespY();
                
                    if( desp_x > 0 )
                    {
                        x1 = enemy_aux->GetIniX();
                        x2 = enemy_aux->GetIniX() + enemy_aux->GetDespX();
                    }
                    else
                    {
                        x1 = enemy_aux->GetIniX() + enemy_aux->GetDespX();
                        x2 = enemy_aux->GetIniX();
                    }
                    if( desp_y > 0 )
                    {
                        y1 = enemy_aux->GetIniY();
                        y2 = enemy_aux->GetIniY() + enemy_aux->GetDespY();
                    }
                    else
                    {
                        y1 = enemy_aux->GetIniY() + enemy_aux->GetDespY();
                        y2 = enemy_aux->GetIniY();
                    }

                                    
                
                    if(  (mouse_x >= x1 - screen_x) && (mouse_x <= (x2 - screen_x)) && 
                         (mouse_y >= y1 - screen_y) && (mouse_y <= (y2 - screen_y))  )
                    { 
                         trobat = true;
                    }
                    else // también lo podemos seleccionar picando sobre el enemigo!
                    {
                         if( (mouse_x >= enemy_aux->GetCurrentX() - screen_x) && (mouse_x <= ((enemy_aux->GetCurrentX() - screen_x) + enemy_aux->GetCurrentSizeX()*world->GetTileSizeRatio())) &&
                             (mouse_y >= enemy_aux->GetCurrentY() - screen_y) && (mouse_y <= ((enemy_aux->GetCurrentY() - screen_y) + enemy_aux->GetCurrentSizeY()*world->GetTileSizeRatio()))  )
                         {
                             trobat = true;
                         }
                    }
                }

                if( trobat == true )
                {
                    if( mouse_b & 0x0001 )
                    {
                        estado = MODO_MOVER_ENEMIGO;
                        enemy = enemy_aux;
                        cout << " A mover enemigo!" << endl;
                    }
                    else if( mouse_b & 0x0002 )
                    {
                        enemy = enemy_aux;
                        
                        set_dialog_color(edit_enemy_dialog, makecol(0, 0, 0), makecol(255, 255, 255));
                        centre_dialog(edit_enemy_dialog);
                        gui_mg_color = makecol(128, 128, 128);                                      

                        assert( enemy != NULL );
                        sprintf(current_enemy_id, "%d", enemy->GetId());                        
                        sprintf(current_enemy_type, "%d", enemy->GetEnemyType());
                        sprintf(current_enemy_estado, "%d", enemy->GetIniState());
                        sprintf(current_enemy_sentido, "%d", enemy->GetIniSentido());
                        sprintf(current_enemy_sentido_vertical, "%d", enemy->GetIniSentidoVertical());
                        sprintf(current_enemy_vel, "%d", enemy->GetVel());
                        do_dialog(edit_enemy_dialog, 2);                        
                        
                        enemy->SetId(atoi(current_enemy_id));
                        enemy->SetEnemyType(atoi(current_enemy_type));
                        enemy->SetIniState(atoi(current_enemy_estado));
                        enemy->SetIniSentido(atoi(current_enemy_sentido));
                        enemy->SetIniSentidoVertical(atoi(current_enemy_sentido_vertical));
                        enemy->SetVel(atoi(current_enemy_vel));
                        
                        enemy->SetCurrentState(enemy->GetIniState());
                        enemy->SetStateToInitial();
                    }
                 }                                                  
             }        
        }        
       
        if( entrada_teclado & 0x0020 )
        {
            enemyHandler->HandleEnemiesState(buffer[current_buffer_num], world, NULL, mseg_global, screen_x, screen_y);
        }
       

        ///////////////////////////////////////////////////////////////////////
        // SALIDA
        drawWorld(world, buffer[current_buffer_num], screen_x, screen_y);   // pintamos scrolls y pantalla

        // pintamos el mouse
        // y pintamos el recuadro del tile en el que se encuentra el mouse
      
        if( estado == MODO_INSERTAR_COLISIONES )
        {        
            tile_x = (mouse_x / world->GetTilesSizeX())*world->GetTilesSizeX();
            tile_y = (mouse_y / world->GetTilesSizeY())*world->GetTilesSizeY();       
            rect(buffer[current_buffer_num], tile_x, tile_y, tile_x + world->GetTilesSizeX(), tile_y + world->GetTilesSizeY(), 0xABCDE);
            sprintf(text_buff, "value = %d    ", world->GetTileValueByCoord(screen_x + mouse_x, screen_y + mouse_y));
            textout_ex(buffer[current_buffer_num], font, text_buff, tile_x + world->GetTilesSizeX() + 1, tile_y, 0xABCDE, -1);  
            sprintf(text_buff, "col = %d    ", world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->GetCollisionable());
            textout_ex(buffer[current_buffer_num], font, text_buff, tile_x + world->GetTilesSizeX() + 1, tile_y + 8, 0xABCDE, -1);
            if( insert_col_pintado == true )
            {
                sprintf(text_buff, "pint = %d    ", world->GetTileByCoord(screen_x + mouse_x, screen_y + mouse_y)->GetPintado());
                textout_ex(buffer[current_buffer_num], font, text_buff, tile_x + world->GetTilesSizeX() + 1, tile_y + 16, 0xABCDE, -1);            
            }
        }
        else if( estado == MODO_INSERTAR_OBJETO )
        {
            object->SetCurrentX(screen_x + mouse_x);
            object->SetCurrentY(screen_y + mouse_y);
            object->drawObjectInWorld(world, buffer[current_buffer_num], object->GetCurrentX() - screen_x, object->GetCurrentY() - screen_y, true);    
        } 
        else if( estado == MODO_INSERTAR_ENEMIGO_1 )
        {
            enemy->SetIniX(screen_x + mouse_x);
            enemy->SetIniY(screen_y + mouse_y);
            enemy->SetCurrentX(screen_x + mouse_x);
            enemy->SetCurrentY(screen_y + mouse_y);            
            enemy->drawCharacterInWorld(world, buffer[current_buffer_num], enemy->GetIniX() - screen_x, enemy->GetIniY() - screen_y);    
        }        
        
               
        if( (sub_estado & SUBMODO_VER_OBJETOS) || estado == MODO_INSERTAR_OBJETO )
        {
            objHandler->DrawObjectsInScreen(buffer[current_buffer_num], world, screen_x, screen_y, SCREEN_X_ORIG, SCREEN_Y_ORIG, false, 0);
        }          
        if( estado == MODO_INSERTAR_CHK_2 || estado == MODO_INSERTAR_SCRIPT_2 || estado == MODO_INSERTAR_ENEMIGO_2 )
        {
            rect(buffer[current_buffer_num], rect_up_x - screen_x, rect_up_y - screen_y, mouse_x, mouse_y, 0xABCDE);
        }  
        if( estado == MODO_INSERTAR_CHK_1 || estado == MODO_INSERTAR_CHK_2 || (sub_estado & SUBMODO_VER_CHECKPOINTS) || estado == MODO_MOVER_CHK )
        {
            int i = 0;
            for( vector<checkpoint*>::iterator it = checkHandler->GetCheckpoints()->begin() ; it != checkHandler->GetCheckpoints()->end() ; it++ )
            {
                 checkpoint* check = *it;
                 
                 check->num = i;
                 i++;
                 if( (mouse_x >= check->x_izq - screen_x) && (mouse_x <= check->x_der - screen_x) && 
                     (mouse_y >= check->y_izq - screen_y) && (mouse_y <= check->y_der - screen_y) && 
                     ((estado != MODO_MOVER_CHK) || (estado == MODO_MOVER_CHK && check != chk)) )
                 {
                     rectfill(buffer[current_buffer_num], check->x_izq - screen_x, check->y_izq - screen_y, check->x_der - screen_x, check->y_der - screen_y, 0xABCDE);
                 }
                 else
                 {
                     rect(buffer[current_buffer_num], check->x_izq - screen_x, check->y_izq - screen_y, check->x_der - screen_x, check->y_der - screen_y, 0xABCDE);
                 }
            }
        }
        if( estado == MODO_INSERTAR_SCRIPT_1 || estado == MODO_INSERTAR_SCRIPT_2 || (sub_estado & SUBMODO_VER_SCRIPTS) || estado == MODO_MOVER_SCRIPT )
        {
            for( list<script*>::iterator it = scrptHandler->GetScripts()->begin() ; it != scrptHandler->GetScripts()->end() ; it++ )
            {
                 script* script_aux = *it;
                 
                 if( (mouse_x >= script_aux->GetX() - screen_x) && (mouse_x <= (script_aux->GetX() + script_aux->GetSizeX()) - screen_x) && 
                     (mouse_y >= script_aux->GetY() - screen_y) && (mouse_y <= (script_aux->GetY() + script_aux->GetSizeY()) - screen_y) && 
                     ((estado != MODO_MOVER_SCRIPT) || (estado == MODO_MOVER_SCRIPT && script_aux != scrpt)) )
                 {
                      rectfill(buffer[current_buffer_num], 
                               script_aux->GetX() - screen_x, 
                               script_aux->GetY() - screen_y, 
                               (script_aux->GetX() + script_aux->GetSizeX()) - screen_x,
                               (script_aux->GetY() + script_aux->GetSizeY()) - screen_y,
                               0x0FBF);
                 }
                 else
                 {
                      rect(buffer[current_buffer_num], 
                           script_aux->GetX() - screen_x, 
                           script_aux->GetY() - screen_y, 
                           (script_aux->GetX() + script_aux->GetSizeX()) - screen_x,
                           (script_aux->GetY() + script_aux->GetSizeY()) - screen_y,
                           0x0FBF);
                 }
            }
        }
        if( estado == MODO_INSERTAR_ENEMIGO_1 || estado == MODO_INSERTAR_ENEMIGO_2 || estado == MODO_MOVER_ENEMIGO || (sub_estado & SUBMODO_VER_ENEMIGOS) )
        {            
            for( vector<enemigo*>::iterator it = enemyHandler->GetEnemies()->begin() ; it != enemyHandler->GetEnemies()->end() ; it++ )
            {
                enemigo* enemy_aux = *it;
                int x1, x2;
                int y1, y2;
                int desp_x = enemy_aux->GetDespX();
                int desp_y = enemy_aux->GetDespY();
                   
                if( desp_x > 0 )
                {
                    x1 = enemy_aux->GetIniX();
                    x2 = enemy_aux->GetIniX() + enemy_aux->GetDespX();
                }
                else
                {
                    x1 = enemy_aux->GetIniX() + enemy_aux->GetDespX();
                    x2 = enemy_aux->GetIniX();
                }
                if( desp_y > 0 )
                {
                    y1 = enemy_aux->GetIniY();
                    y2 = enemy_aux->GetIniY() + enemy_aux->GetDespY();
                }
                else
                {
                    y1 = enemy_aux->GetIniY() + enemy_aux->GetDespY();
                    y2 = enemy_aux->GetIniY();
                }                                                        
                
                if(  (mouse_x >= x1 - screen_x) && (mouse_x <= (x2 - screen_x)) && 
                     (mouse_y >= y1 - screen_y) && (mouse_y <= (y2 - screen_y))  && 
                     ((estado != MODO_MOVER_ENEMIGO) || (estado == MODO_MOVER_ENEMIGO && enemy_aux != enemy)) )
                 {
                      rectfill(buffer[current_buffer_num], 
                               enemy_aux->GetIniX() - screen_x, 
                               enemy_aux->GetIniY() - screen_y, 
                               (enemy_aux->GetIniX() + enemy_aux->GetDespX()) - screen_x,
                               (enemy_aux->GetIniY() + enemy_aux->GetDespY()) - screen_y,
                               0xAFDA);
                 }
                 else
                 {
                      rect(buffer[current_buffer_num], 
                           enemy_aux->GetIniX() - screen_x, 
                           enemy_aux->GetIniY() - screen_y, 
                           (enemy_aux->GetIniX() + enemy_aux->GetDespX()) - screen_x,
                           (enemy_aux->GetIniY() + enemy_aux->GetDespY()) - screen_y,
                           0xAFDA);
                      sprintf(text_buff, "INI");                           
                      textout_ex(buffer[current_buffer_num], font, text_buff, enemy_aux->GetIniX() - screen_x, enemy_aux->GetIniY() - screen_y, 0xAFDA, -1);
                 }
            }
            
            enemyHandler->DrawEnemiesInScreen(buffer[current_buffer_num], world, screen_x, screen_y, SCREEN_X_ORIG, SCREEN_Y_ORIG);  // FIXME
        }                  


        // escribimos las coordenadas del raton
        sprintf(text_buff, "(x, y) = (%d, %d)", mouse_x + screen_x, mouse_y + screen_y);
        textout_ex(buffer[current_buffer_num], font, text_buff, SCREEN_X - 150, SCREEN_Y - 16, 0xABCDE, -1);
       
        release_bitmap(buffer[current_buffer_num]);
        release_bitmap(screen_buffer[current_buffer_num]);        
        current_buffer_num = 1-current_buffer_num;
        
        // adaptamos a la pantalla        
        stretch_blit(buffer[current_buffer_num], screen_buffer[current_buffer_num], 0, 0, SCREEN_X_ORIG,  SCREEN_Y_ORIG, 0, 0, SCREEN_X, SCREEN_Y);         

        the_dialog[1].dp = screen_buffer[current_buffer_num];
        object_message(&the_dialog[1], MSG_DRAW, 0);
        object_message(&the_dialog[2], MSG_DRAW, 0);        
}

int main()
{
   init();

   fx_handler = new fxHandler();

   /* shift the dialog 2 pixels away from the border */
   position_dialog(the_dialog, 1, 1);
   the_dialog[1].dp = screen_buffer[0]; // BITMAP_OBJECT      

   GameMain(0);  // First call to fill screen_buffer

   do_dialog(the_dialog, -1);
   
   deinit();
   return 0;
}
END_OF_MAIN();

void init()
{
	int depth, res;

	allegro_init();
	depth = 32;
	set_color_depth(depth);
	res = set_gfx_mode(GFX_AUTODETECT_WINDOWED, SCREEN_X, SCREEN_Y, 0, 0);
	if (res != 0)
    {
		allegro_message(allegro_error);
		exit(-1);
	}

	install_timer();
	install_keyboard();
	install_mouse();

	/* add other initializations here */
    // Rutinas temporales
    LOCK_VARIABLE(mseg);
    LOCK_FUNCTION(tiempo);
    mseg = 0;
    install_int(tiempo, 1);

	buffer[0] = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG);
    buffer[1] = create_bitmap(SCREEN_X_ORIG, SCREEN_Y_ORIG);
	screen_buffer[0] = create_bitmap(SCREEN_X, SCREEN_Y);
    screen_buffer[1] = create_bitmap(SCREEN_X, SCREEN_Y);
    
    current_buffer_num = 0;
    
    sprite_image = create_bitmap(32, 32);//world->GetTilesSizeX(), world->GetTilesSizeY());  // FIXME
}

void deinit() {
	clear_keybuf();
	/* add other deinitializations here */
}
