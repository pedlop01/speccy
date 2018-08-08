/***************************************************************************/
/* Programa: Tetris by Falcon Soft                          Verano 2000    */
/* Autor: Pedro L�pez Mu�oz                                                */
/***************************************************************************/

/*************************** Declaraci�n de librerias **********************/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

/*************************** Declaraci�n de tipos **************************/

#define IN_GAME  0
#define IN_PAUSE 1
#define IN_QUIT  2

#define SCREEN_X 640
#define SCREEN_Y 480
#define MAX_Y_TABLERO 25
#define MAX_X_TABLERO 11
#define KEY_LEFT  0x001
#define KEY_RIGHT 0x002
#define KEY_UP    0x004
#define KEY_DOWN  0x008
#define KEY_SPACE 0x010
#define KEY_Q     0x020
#define KEY_S     0x040
#define KEY_N     0x080
#define KEY_M     0x100
#define KEY_P     0x200
#define KEY_ESC   0x400

typedef struct {
  char nombre[50];
  int  lineas;
  int  puntos;
} record;

typedef struct{
   int casilla[4][4];
   }pos_ficha;

typedef struct{
    pos_ficha posicion[4];
    int n_posiciones;
    }ficha;

typedef struct{
   int x, y;
   pos_ficha *puntero_ficha;
   int pos, n_ficha, color;
   }f_actual;

typedef struct{
   int matriz[MAX_Y_TABLERO][MAX_X_TABLERO + 1];
   int puntos;
   int lineas;
   }tablero;

int ficha_1_1[4][4] = { {1,0,0,0},{1,0,0,0},{1,0,0,0},{1,0,0,0} };
int ficha_1_2[4][4] = { {0,0,0,0},{0,0,0,0},{0,0,0,0},{1,1,1,1} };

int ficha_2_1[4][4] = { {0,0,0,0},{0,0,0,0},{0,1,1,0},{1,1,0,0} };
int ficha_2_2[4][4] = { {0,0,0,0},{1,0,0,0},{1,1,0,0},{0,1,0,0} };

int ficha_3_1[4][4] = { {0,0,0,0},{0,0,0,0},{1,1,0,0},{0,1,1,0} };
int ficha_3_2[4][4] = { {0,0,0,0},{0,1,0,0},{1,1,0,0},{1,0,0,0} };

int ficha_4_1[4][4] = { {0,0,0,0},{0,0,0,0},{1,1,1,0},{0,1,0,0} };
int ficha_4_2[4][4] = { {0,0,0,0},{0,1,0,0},{1,1,0,0},{0,1,0,0} };
int ficha_4_3[4][4] = { {0,0,0,0},{0,0,0,0},{0,1,0,0},{1,1,1,0} };
int ficha_4_4[4][4] = { {0,0,0,0},{1,0,0,0},{1,1,0,0},{1,0,0,0} };

int ficha_5_1[4][4] = { {0,0,0,0},{0,0,0,0},{1,1,0,0},{1,1,0,0} };

int ficha_6_1[4][4] = { {0,0,0,0},{0,0,0,0},{1,0,0,0},{1,1,1,0} };
int ficha_6_2[4][4] = { {0,0,0,0},{0,1,0,0},{0,1,0,0},{1,1,0,0} };
int ficha_6_3[4][4] = { {0,0,0,0},{0,0,0,0},{1,1,1,0},{0,0,1,0} };
int ficha_6_4[4][4] = { {0,0,0,0},{1,1,0,0},{1,0,0,0},{1,0,0,0} };

int ficha_7_1[4][4] = { {0,0,0,0},{0,0,0,0},{0,0,1,0},{1,1,1,0} };
int ficha_7_2[4][4] = { {0,0,0,0},{1,0,0,0},{1,0,0,0},{1,1,0,0} };
int ficha_7_3[4][4] = { {0,0,0,0},{0,0,0,0},{1,1,1,0},{1,0,0,0} };
int ficha_7_4[4][4] = { {0,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,0,0} };

record records[10];

/*************************** Declaracion de variables **********************/
// Allegro variables
ALLEGRO_DISPLAY*       display     = NULL;
ALLEGRO_BITMAP*        bitmap      = NULL;
ALLEGRO_BITMAP*        bitmap_aux  = NULL;
ALLEGRO_EVENT_QUEUE*   event_queue = NULL;
ALLEGRO_SAMPLE*        sample      = NULL;

ALLEGRO_SAMPLE *the_music[10];
ALLEGRO_SAMPLE_INSTANCE* the_music_instance[10];
ALLEGRO_SAMPLE_INSTANCE* music_intro_instance;
ALLEGRO_SAMPLE_INSTANCE* music_final_instance;
ALLEGRO_SAMPLE_INSTANCE* last_music_instance;
ALLEGRO_SAMPLE *intro;
ALLEGRO_SAMPLE *music_final;

ALLEGRO_SAMPLE *rotar;
ALLEGRO_SAMPLE *colision;
ALLEGRO_SAMPLE *linea;
ALLEGRO_SAMPLE *s_combo;
ALLEGRO_SAMPLE *mega;

ALLEGRO_BITMAP *fondo[10];
ALLEGRO_BITMAP *the_image;
ALLEGRO_BITMAP *objetos;

ALLEGRO_FONT *font;

ficha todas_las_fichas[7];
f_actual aux;
tablero tetris;
int baja, tics = 0, sig;
int antdif, dif = 1000, antheight, height = 1, antvisual, visual = 3, combo = 0;
int pasos_ficha;
int keys;
int keys_pressed_down;
int steps;
int pasos_dir;
bool already_drawn;
bool printed_dead;
int state = IN_GAME;

// Timer
double PCFreq;
__int64 CounterStart;

/************************** Funciones ************************************/

// ----- Helper functions
void blit(ALLEGRO_BITMAP* _bitmap, int orig_x, int orig_y, int dest_x, int dest_y, int orig_width, int orig_height) {
  al_draw_bitmap_region(_bitmap, orig_x, orig_y, orig_width, orig_height, dest_x, dest_y, 0);
}

// Only used for in-game
void draw_screen() {
  al_set_target_bitmap(al_get_backbuffer(display));
  al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 40, SCREEN_X, SCREEN_Y, 0);
  al_flip_display();

  al_set_target_bitmap(bitmap);
  al_draw_bitmap(fondo[(tetris.lineas /10) % 10], 0, 0, 0);
}

void start_counter() {
  LARGE_INTEGER li;
  if(!QueryPerformanceFrequency(&li))
    printf("QueryPerformanceFrequency failed!\n");

  PCFreq = ((double)(li.QuadPart))/1000.0;

  QueryPerformanceCounter(&li);
  CounterStart = li.QuadPart;
}

double get_counter() {
  LARGE_INTEGER li;
  QueryPerformanceCounter(&li);

  return ((double)(li.QuadPart-CounterStart))/PCFreq;
}

void read_keyboard(ALLEGRO_EVENT_QUEUE *event_queue) {
  bool keyEvent;
  bool keyDown;
  ALLEGRO_EVENT ev;

  keys_pressed_down = 0;

  while(!al_is_event_queue_empty(event_queue)) {
    keyEvent = false;
    keyDown = false;

    al_wait_for_event(event_queue, &ev);

    if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
      keyEvent = true;
      keyDown  = true;
    } else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
      keyEvent = true;
      keyDown  = false;
    }

    if (keyEvent) {
      switch (ev.keyboard.keycode) {
        case ALLEGRO_KEY_LEFT   : if(keyDown) { keys |= KEY_LEFT;  keys_pressed_down |= KEY_LEFT;  } else { keys &= ~KEY_LEFT;  } break;
        case ALLEGRO_KEY_RIGHT  : if(keyDown) { keys |= KEY_RIGHT; keys_pressed_down |= KEY_RIGHT; } else { keys &= ~KEY_RIGHT; } break;
        case ALLEGRO_KEY_UP     : if(keyDown) { keys |= KEY_UP;    keys_pressed_down |= KEY_UP;    } else { keys &= ~KEY_UP;    } break;
        case ALLEGRO_KEY_DOWN   : if(keyDown) { keys |= KEY_DOWN;  keys_pressed_down |= KEY_DOWN;  } else { keys &= ~KEY_DOWN;  } break;
        case ALLEGRO_KEY_SPACE  : if(keyDown) { keys |= KEY_SPACE; keys_pressed_down |= KEY_SPACE; } else { keys &= ~KEY_SPACE; } break;
        case ALLEGRO_KEY_Q      : if(keyDown) { keys |= KEY_Q;     keys_pressed_down |= KEY_Q;     } else { keys &= ~KEY_Q;     } break;
        case ALLEGRO_KEY_P      : if(keyDown) { keys |= KEY_P;     keys_pressed_down |= KEY_P;     } else { keys &= ~KEY_P;     } break;
        case ALLEGRO_KEY_M      : if(keyDown) { keys |= KEY_M;     keys_pressed_down |= KEY_M;     } else { keys &= ~KEY_M;     } break;
        case ALLEGRO_KEY_N      : if(keyDown) { keys |= KEY_N;     keys_pressed_down |= KEY_N;     } else { keys &= ~KEY_N;     } break;
        case ALLEGRO_KEY_S      : if(keyDown) { keys |= KEY_S;     keys_pressed_down |= KEY_S;     } else { keys &= ~KEY_S;     } break;
        case ALLEGRO_KEY_ESCAPE : if(keyDown) { keys |= KEY_ESC;   keys_pressed_down |= KEY_ESC;   } else { keys &= ~KEY_ESC;   } break;
      }
    }
  }
}

bool pressed_left() {
  return keys & KEY_LEFT;
}

bool pressed_right() {
  return keys & KEY_RIGHT;
}

bool pressed_up() {
  return (keys & KEY_UP) && (keys_pressed_down & KEY_UP);
}

bool pressed_down() {
  return keys & KEY_DOWN;
}

bool pressed_space() {
  return keys & KEY_SPACE;
}

bool pressed_n() {
  return keys & KEY_N;
}

bool pressed_m() {
  return keys & KEY_M;
}

bool pressed_q() {
  return keys & KEY_Q;
}

bool pressed_s() {
  return keys & KEY_S;
}

bool pressed_p() {
  return (keys & KEY_P) && (keys_pressed_down & KEY_P);
}

// --- End helper functions

pos_ficha copiar_ficha(int ent[4][4]) {
  int i,j;
  pos_ficha sort;

  for(i = 0; i < 4; i++) {
    for(j = 0; j < 4; j++) {
      sort.casilla[i][j] = ent[i][j];
	  }
  }

  return sort;
}

void visualizar_ficha(int ent[4][4]) {
  int i,j;

  for(i = 0; i < 4; i++) {
    for(j = 0; j < 4; j++) {
      if (ent[i][j]) {
        blit(objetos, 22*sig, 0, j*10 + 250, i*10 + 80, 10, 10);
	    } else {
        blit(objetos, 0, 0, j*10 + 250, i*10  + 80, 10, 10);
      }
	  }
  }
}

f_actual crear_ficha(int n) {
  f_actual fa;

  fa.x = 5; fa.y = 1;
  fa.puntero_ficha = &todas_las_fichas[n - 1].posicion[0];
  fa.pos = 0;
  fa.n_ficha = n;
  fa.color = n;

  return fa;
}

f_actual rotar_derecha(f_actual aux) {
  aux.pos = abs((aux.pos + 1)) % todas_las_fichas[aux.n_ficha - 1].n_posiciones;
  aux.puntero_ficha = &todas_las_fichas[aux.n_ficha - 1].posicion[aux.pos];

  return aux;
}

f_actual rotar_izquierda(f_actual aux) {
  int a;

  a = abs((aux.pos - 1)) % todas_las_fichas[aux.n_ficha - 1].n_posiciones;
  aux.pos = (aux.pos - 1) % todas_las_fichas[aux.n_ficha - 1].n_posiciones;
  aux.puntero_ficha = &todas_las_fichas[aux.n_ficha - 1].posicion[a];

  return aux;
}

tablero rellenar_tablero(tablero t) {
  int i, j, tope, aleatorio;

  switch(height) {
    case 2: tope = MAX_Y_TABLERO - 6; break;
    case 3: tope = MAX_Y_TABLERO - 8; break;
    case 4: tope = MAX_Y_TABLERO - 10; break;
    case 5: tope = MAX_Y_TABLERO - 12; break;
  }

  for(i = MAX_Y_TABLERO - 4; i >= tope; i--) {
    for(j = 1; j <=6; j++) {
      aleatorio = rand()%10 + 1;
      t.matriz[i][aleatorio] = (rand()%7) + 1;
    }
  }

  return t;
}

tablero crear_tablero() {
  int i,j;
  tablero t;

  t.puntos = 0;
  t.lineas = 0;
  for(i = 0; i < MAX_Y_TABLERO - 2; i++) {
    for(j = 0; j < 11; j++) {
      t.matriz[i][j] = 0;
	  }
  }

  for(i = 0; i < MAX_Y_TABLERO - 2; i++) {
    t.matriz[i][0] = 8;
  }

  for(i = 0; i < MAX_Y_TABLERO - 2; i++) {
    t.matriz[i][MAX_X_TABLERO] = 8;
  }

  for(i = 0; i < MAX_X_TABLERO; i++) {
    t.matriz[MAX_Y_TABLERO - 3][i] = 8;
  }

  if(height != 1)
    t = rellenar_tablero(t);

  return t;
}

void visualizar_tablero() {
  int i,j;
  char s[50];
  
  sprintf(s, "Lineas:%d", tetris.lineas);
  al_draw_text(font, al_map_rgb(255, 255, 255), 235, 27, ALLEGRO_ALIGN_LEFT, s);
  sprintf(s, "Ptos:%d", tetris.puntos);
  al_draw_text(font, al_map_rgb(255, 255, 255), 235, 37, ALLEGRO_ALIGN_LEFT, s);

  for(i = 4; i < MAX_Y_TABLERO - 2; i++) {
    for(j = 0; j < MAX_X_TABLERO + 1; j++) {
      if (tetris.matriz[i][j]) {
        blit(objetos, 22*tetris.matriz[i][j], 0, j*10 + 100, i*10 - 35, 10, 10);
	    } else {
        blit(objetos, 0, 0, j*10 + 100, i*10  - 35, 10, 10);
	    }
	  }
  }
}

int hay_colision(f_actual aux, int x, int y) {
  int i,j;
  int trobat, acaba, trobat2;

  trobat = FALSE;

  for(i = 0; i < 4 && !trobat; i++) {
    for(j = 0; j < 4 && !trobat; j++) {
      if(aux.puntero_ficha -> casilla[i][j]) {
        if(tetris.matriz[y + i][x + j]) {
	        trobat = TRUE;
	      }
	    }
    }
  }

  return trobat;
}

void posicionar_ficha(f_actual aux, int x, int y, int c) {
  int i,j;

  if (c)
    c = aux.color;

  for(i = 0; i <= 3; i++) {
    for(j = 0; j <= 3; j++) {
      if(aux.puntero_ficha->casilla[i][j]) {
        tetris.matriz[y + i - 1][x + j] = c;
	    }
	  }
  }
}

void puntuacion(int lineas) {
  switch(lineas) {
    case 4: tetris.puntos = tetris.puntos + 50;
    case 3: tetris.puntos = tetris.puntos + 30;
    case 2: tetris.puntos = tetris.puntos + 20;
    case 1: tetris.puntos = tetris.puntos + 10;
    default: break;
  }

  if(combo > 1) {
    tetris.puntos = (50*lineas*combo) + tetris.puntos;
    if(combo < 4) {
      al_play_sample(s_combo, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    } else {
      al_play_sample(mega, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
    }
  }
}

int mirar_lineas() {
  int i, j, k, lineas;
  int trobat, hay_linea;

  hay_linea = FALSE; lineas = 0;
  for(i = MAX_Y_TABLERO - 4; i >= 0; i--) {
    trobat = FALSE;
    for(j = MAX_X_TABLERO - 1; j >= 1 && !trobat; j--) {
      if(tetris.matriz[i][j] == 0) {
	      trobat = TRUE;
	    }
	  }
    if(!trobat) {
      lineas++;
      visualizar_tablero();
      visualizar_ficha(todas_las_fichas[sig - 1].posicion[0].casilla);
      blit(objetos, 0, 22, j*10 + 111, i*10  - 34, 99, 9);
      draw_screen();
      Sleep(50);
      already_drawn = true;

      al_play_sample(linea, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
      hay_linea = TRUE;
      for(j = MAX_X_TABLERO - 1; j >= 1; j--) {
        tetris.matriz[i][j] = 0;
	    }


      if((++tetris.lineas % 10) == 0) {

        if(dif > 400) {
          dif = dif - 75;
        } else {
          if(dif > 50) {
            dif = dif - 20;
          }
        }

        int level = (tetris.lineas / 10) % 10;
        al_stop_sample_instance(last_music_instance);
        al_play_sample_instance(the_music_instance[level]);
        last_music_instance = the_music_instance[level];
        al_draw_bitmap(fondo[level], 0, 0, 0);
      }

      for(k = i; k >= 1; k--) {
        for(j = 1; j <= MAX_X_TABLERO - 1; j++) {
          tetris.matriz[k][j] = tetris.matriz[k - 1][j];
	      }
	    }
      for(j = 1; j <= MAX_X_TABLERO - 1; j++) {
          tetris.matriz[0][j] = 0;
	    }
	  i++;
    }
  }

  if(lineas) {
    combo++;
    puntuacion(lineas);
  } else {
    combo = 0;
  }

  return hay_linea;
}

void muerte() {
  int i, j;

  if (printed_dead) return;

  for(i = MAX_Y_TABLERO - 4; i >= 4; i--) {
    al_play_sample(linea, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);

    visualizar_tablero();
    for (j = MAX_Y_TABLERO - 4; j >= i; j--)
      blit(objetos, 0, 22, 111, j*10  - 34, 99, 9);

    draw_screen();
    Sleep(50);
  }
  printed_dead = true;
}

void draw_muerte_completa(bool pausa, bool record) {
  int i;

  al_set_target_bitmap(bitmap_aux);
  visualizar_tablero();
  for(i = MAX_Y_TABLERO - 4; i >= 4; i--) {
    blit(objetos, 0, 22, 111, i*10  - 34, 99, 9);
  }

  if (pausa) {
    blit(objetos, 85, 42, 132, 80, 62, 24);
  } else if (!record) {
    blit(objetos, 85, 66, 132, 70, 62, 37);
  }

  al_set_target_bitmap(bitmap);
  al_draw_bitmap(bitmap_aux, 0, 0, 0);
}

void inicializar() {

  tetris = crear_tablero();

  srand(time(0));

  todas_las_fichas[0].n_posiciones = 2;
  todas_las_fichas[0].posicion[0] = copiar_ficha(ficha_1_1);
  todas_las_fichas[0].posicion[1] = copiar_ficha(ficha_1_2);

  todas_las_fichas[1].n_posiciones = 2;
  todas_las_fichas[1].posicion[0] = copiar_ficha(ficha_2_1);
  todas_las_fichas[1].posicion[1] = copiar_ficha(ficha_2_2);

  todas_las_fichas[2].n_posiciones = 2;
  todas_las_fichas[2].posicion[0] = copiar_ficha(ficha_3_1);
  todas_las_fichas[2].posicion[1] = copiar_ficha(ficha_3_2);

  todas_las_fichas[3].n_posiciones = 4;
  todas_las_fichas[3].posicion[0] = copiar_ficha(ficha_4_1);
  todas_las_fichas[3].posicion[1] = copiar_ficha(ficha_4_2);
  todas_las_fichas[3].posicion[2] = copiar_ficha(ficha_4_3);
  todas_las_fichas[3].posicion[3] = copiar_ficha(ficha_4_4);

  todas_las_fichas[4].n_posiciones = 1;
  todas_las_fichas[4].posicion[0] = copiar_ficha(ficha_5_1);

  todas_las_fichas[5].n_posiciones = 4;
  todas_las_fichas[5].posicion[0] = copiar_ficha(ficha_6_1);
  todas_las_fichas[5].posicion[1] = copiar_ficha(ficha_6_2);
  todas_las_fichas[5].posicion[2] = copiar_ficha(ficha_6_3);
  todas_las_fichas[5].posicion[3] = copiar_ficha(ficha_6_4);

  todas_las_fichas[6].n_posiciones = 4;
  todas_las_fichas[6].posicion[0] = copiar_ficha(ficha_7_1);
  todas_las_fichas[6].posicion[1] = copiar_ficha(ficha_7_2);
  todas_las_fichas[6].posicion[2] = copiar_ficha(ficha_7_3);
  todas_las_fichas[6].posicion[3] = copiar_ficha(ficha_7_4);

  rotar    = al_load_sample("../fx/rotar.wav");
  colision = al_load_sample("../fx/colision.wav");
  linea    = al_load_sample("../fx/linea.wav");
  s_combo  = al_load_sample("../fx/combo.wav");
  mega     = al_load_sample("../fx/mega.wav");

  the_music[0] = al_load_sample("../music/manic.ogg");
  the_music[1] = al_load_sample("../music/prov1.ogg");  // (camelot)
  the_music[2] = al_load_sample("../music/prov2.ogg");  // (mario)
  the_music[3] = al_load_sample("../music/prov3.ogg");  // (sonic)
  the_music[4] = al_load_sample("../music/prov4.ogg");  // (prince)
  the_music[5] = al_load_sample("../music/monk.ogg");
  the_music[6] = al_load_sample("../music/indy.ogg");
  the_music[7] = al_load_sample("../music/riu.ogg");
  the_music[8] = al_load_sample("../music/progre.ogg");  // (doom)
  the_music[9] = al_load_sample("../music/gprix.ogg");
  for (int i = 0; i < 10; i++) {
    the_music_instance[i] = al_create_sample_instance(the_music[i]);
    al_set_sample_instance_playmode(the_music_instance[i], ALLEGRO_PLAYMODE_LOOP);
    al_attach_sample_instance_to_mixer(the_music_instance[i], al_get_default_mixer());
  }

  fondo[0] = al_load_bitmap("../graphic/fondo1.pcx");
  fondo[1] = al_load_bitmap("../graphic/fondo2.pcx");
  fondo[2] = al_load_bitmap("../graphic/fondo3.pcx");
  fondo[3] = al_load_bitmap("../graphic/fondo4.pcx");
  fondo[4] = al_load_bitmap("../graphic/fondo5.pcx");
  fondo[5] = al_load_bitmap("../graphic/fondo6.pcx");
  fondo[6] = al_load_bitmap("../graphic/fondo7.pcx");
  fondo[7] = al_load_bitmap("../graphic/fondo8.pcx");
  fondo[8] = al_load_bitmap("../graphic/fondo9.pcx");
  fondo[9] = al_load_bitmap("../graphic/fondo10.pcx");
}

void populate_records() {
  size_t len = 0;
  ssize_t read;
  char* line = NULL;
  char seps[] = ",\t\n";
  char* token;
  char buf[20];

  FILE* file = fopen("records.txt", "r");
  if (file == NULL) {
    printf("Error: No record file!\n");
    exit(-1);
  }

  int num_record = 0;
  while ((read = getline(&line, &len, file)) != -1) {
    token = strtok(line, seps);
    sscanf(token, "%s", buf);
    sprintf(records[num_record].nombre, "%s", buf);
    token = strtok(NULL, seps);
    sscanf(token, "%s", buf);
    records[num_record].lineas = atoi(buf);    
    token = strtok(NULL, seps);
    sscanf(token, "%s", buf);
    records[num_record].puntos = atoi(buf);
    num_record++;
  }

  fclose(file);
  if (line)
    free(line);
}

bool check_record() {
  int puntos = tetris.puntos;
  int lineas = tetris.lineas;
  bool trobat = false;
  bool exit = false;
  int new_record_position = 0;
  char new_name[50];

  populate_records();

  for (int i = 0; (i < 10) && !trobat; i++) {
    if (tetris.puntos > records[i].puntos) {
      trobat = true;
      new_record_position = i;
    }
  }

  // no new record!
  if (!trobat) {
    return false;
  }

  // new record!

  al_set_target_bitmap(bitmap);
  al_draw_bitmap(fondo[(tetris.lineas/10) % 10], 0, 0, 0);
  draw_muerte_completa(false, true);
  blit(objetos, 146, 128, 90, 43, 146, 43);
  draw_screen();

  ALLEGRO_EVENT ev;
  int num_chars = 0;
  char buf[50] = "\0";
  while (!exit) {
    while(!al_is_event_queue_empty(event_queue)) {

      al_wait_for_event(event_queue, &ev);

      // draw screen makes bitmap to be ready for drawing
      draw_muerte_completa(false, true);
      blit(objetos, 146, 128, 90, 43, 146, 43);

      if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
        char c = ev.keyboard.unichar;
        if (c == '\r') {
          exit = true;
        } else if (c == 0x08) {
          num_chars--;
          buf[0] = '\0';
          for (int i = 0; i < num_chars; i++) {
            sprintf(buf, "%s%c", buf, new_name[i]);
          }
        } else {
          sprintf(buf, "%s%c", new_name, c);
          num_chars++;
        }

        if (num_chars > 10)
          exit = true;
      }
      sprintf(new_name, "%s", buf);
      al_draw_text(font, al_map_rgb(255, 255, 255),
                   120, 64, ALLEGRO_ALIGN_LEFT, new_name);
      draw_screen();
    }
  }
  

  // Now we have the new_name
  for(int j = 8; j >= new_record_position; j--) {
    sprintf(records[j+1].nombre, "%s", records[j].nombre);
    records[j+1].puntos = records[j].puntos;
    records[j+1].lineas = records[j].lineas;
  }
  sprintf(records[new_record_position].nombre, "%s", new_name);
  records[new_record_position].puntos = puntos;
  records[new_record_position].lineas = lineas;

  FILE* file = fopen("records.txt", "w"); 
  for (int j=0; j < 10; j++){
    fprintf(file,"%s,%d,%d\n", records[j].nombre, records[j].lineas, records[j].puntos);
  }
  fclose(file);

  return true;
}

/*************************** Programa principal ***************************/

void juego() {
  char tecla;
  int final;
  bool pausa;
  bool salir;
  bool pressing_dir;

  inicializar();
  
  aux = crear_ficha((rand()%7) + 1);
  sig = ((rand()%7) + 1);
  posicionar_ficha(aux, aux.x, aux.y, 1);

  final = FALSE;
  baja = FALSE;
  pausa = false;
  salir = false;
  pasos_ficha = 0;
  already_drawn = false;
  printed_dead = false;
  pressing_dir = false;
  keys = 0;

  al_play_sample_instance(the_music_instance[0]);
  last_music_instance = the_music_instance[0];

  start_counter();
  while(!final) {
    baja = false;
    al_set_target_bitmap(bitmap);
    al_draw_bitmap(fondo[(tetris.lineas /10) % 10], 0, 0, 0);

    read_keyboard(event_queue);

    if (!salir && !pausa) {
      if (pasos_ficha*100 > dif) {
        baja = true;
        pasos_ficha = 0;
      }

      posicionar_ficha(aux, aux.x, aux.y, 0);
      if (baja || pressed_down()) {
        if (hay_colision(aux, aux.x, aux.y)) {
          al_play_sample(colision, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
          posicionar_ficha(aux, aux.x, aux.y, 1);
		      mirar_lineas();

		      aux = crear_ficha(sig);
		      sig = (rand()%7) + 1;		              

		      if (hay_colision(aux, aux.x, aux.y)) {
            muerte();
            al_stop_sample_instance(last_music_instance);
            al_play_sample_instance(music_final_instance);
            last_music_instance = music_final_instance;
            check_record();
		        final = TRUE;        
          }
		    } else {
          aux.y++;
		    }
      }
      if (pressed_left()) {
        
        if (!pressing_dir || (pressing_dir && ((pasos_dir % 2) == 0))) {
          aux.x--;
          if (hay_colision(aux, aux.x, aux.y - 1))
            aux.x++;
        }

        pressing_dir = true;
      } else if (pressed_right()) {
        
        if (!pressing_dir || (pressing_dir && ((pasos_dir % 2) == 0))) {
          aux.x++;
          if (hay_colision(aux, aux.x, aux.y - 1))
            aux.x--;
        }

        pressing_dir = true;
      } else {
        pressing_dir = false;
      }

      if (pressed_up() || pressed_n()) {
        aux = rotar_izquierda(aux);
        if(hay_colision(aux, aux.x, aux.y - 1)) {
          aux = rotar_derecha(aux);
        } else {
          al_play_sample(rotar, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        }
      } else if (pressed_m()) {
        aux = rotar_derecha(aux);
        if(hay_colision(aux, aux.x, aux.y - 1)) {
          aux = rotar_izquierda(aux);
        } else {
          al_play_sample(rotar, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
        }
      }
      posicionar_ficha(aux, aux.x, aux.y, 1);
    }

    if (pressed_p() && !pausa && !salir) {
      pausa = true;
    } else if (pausa && pressed_p()) {
      pausa = false;
      printed_dead = false;
    }
    if (pressed_q() && !salir && !pausa) {
      salir = true;
    } else if (salir && pressed_q()) {
      salir = false;
      printed_dead = false;
    }    

    if (pausa) {
      muerte();
      draw_muerte_completa(true, false);
    } else if (salir) {
      muerte();
      draw_muerte_completa(false, false);
      if (pressed_s()) {
        final = true;
        printed_dead = false;
      } else if (pressed_n()) {
        salir = false;
        printed_dead = false;
      }
    } else {
      if (!already_drawn)
        visualizar_tablero();
    }

    if (!salir && !pausa) {
      visualizar_ficha(todas_las_fichas[sig - 1].posicion[0].casilla);
      pasos_ficha++;
    }

    steps++;
    if (pressing_dir)
      pasos_dir++;
    else
      pasos_dir = 0;

    if (!already_drawn) {      
      draw_screen();
    } else {
      already_drawn = false;
    }

    // Check counter value for adding waiting time
    int wait_time = 50;
    double delay = get_counter();
    if(delay < wait_time) {
      Sleep(wait_time - delay);
    }

    start_counter();

  }
  al_stop_sample_instance(music_final_instance);
}

void opciones() {
  ALLEGRO_MOUSE_STATE mouse;

  al_destroy_bitmap(the_image);
  the_image = al_load_bitmap("../graphic/menuop.pcx");

  antdif = 0; antheight = 0; antvisual = 0;
  while(!(mouse.buttons & 2)) {

    al_set_target_bitmap(bitmap);
    al_draw_bitmap(the_image, 0, 0, 0);

    al_get_mouse_state(&mouse);

    int mouse_x = mouse.x/2;
    int mouse_y = (mouse.y - 40)/2;

    if((mouse.buttons & 1) && mouse_y >= 53 && mouse_y <= 65) {
      if(mouse_x >= 104 && mouse_x <= 117)
        dif = 1000;
      if(mouse_x >= 123 && mouse_x <= 136)
        dif = 925;
      if(mouse_x >= 143 && mouse_x <= 156)
        dif = 850;
      if(mouse_x >= 164 && mouse_x <= 177)
        dif = 775;
      if(mouse_x >= 183 && mouse_x <= 196)
        dif = 700;
      if(mouse_x >= 203 && mouse_x <= 216)
        dif = 625;
      if(mouse_x >= 223 && mouse_x <= 236)
        dif = 550;
      if(mouse_x >= 242 && mouse_x <= 255)
        dif = 475;
      if(mouse_x >= 262 && mouse_x <= 275)
        dif = 400;
    }
    if((mouse.buttons & 1) && mouse_y >= 69 && mouse_y <= 81) {
      if(mouse_x >= 104 && mouse_x <= 117)
        height = 1;
      if(mouse_x >= 123 && mouse_x <= 136)
        height = 2;
      if(mouse_x >= 143 && mouse_x <= 156)
        height = 3;
      if(mouse_x >= 164 && mouse_x <= 177)
        height = 4;
      if(mouse_x >= 183 && mouse_x <= 196)
        height = 5;
    }
    if((mouse.buttons & 1) && mouse_x >= 95 && mouse_x <= 104) {
      if(mouse_y >= 110 && mouse_y <= 118)
        visual = 1;
      if(mouse_y >= 122 && mouse_y <= 130)
        visual = 2;
      if(mouse_y >= 135 && mouse_y <= 143)
        visual = 3;
      if(mouse_y >= 147 && mouse_y <= 155)
        visual = 4;
      if(mouse_y >= 159 && mouse_y <= 167)
        visual = 5;
    }

    switch(dif) {
      case 1000: blit(the_image, 104, 53, 104, 53, 216, 13);
                 blit(objetos, 0, 106, 104, 53, 13, 13);                   
                 break;
      case 925:  blit(the_image, 104, 53, 104, 53, 216, 13);                   
                 blit(objetos, 14, 106, 123, 53, 14, 13);
                 break;
      case 850:  blit(the_image, 104, 53, 104, 53, 216, 13);                   
                 blit(objetos, 28, 106, 143, 53, 14, 13);
                 break;
      case 775:  blit(the_image, 104, 53, 104, 53, 216, 13);                   
                 blit(objetos, 42, 106, 164, 53, 14, 13);
                 break;
      case 700:  blit(the_image, 104, 53, 104, 53, 216, 13);                   
                 blit(objetos, 56, 106, 183, 53, 14, 13);
                 break;
      case 625:  blit(the_image, 104, 53, 104, 53, 216, 13);
                 blit(objetos, 70, 106, 203, 53, 14, 13);
                 break;
      case 550:  blit(the_image, 104, 53, 104, 53, 216, 13);                   
                 blit(objetos, 84, 106, 223, 53, 14, 13);
                 break;
      case 475:  blit(the_image, 104, 53, 104, 53, 216, 13);
                 blit(objetos, 98, 106, 242, 53, 14, 13);
                 break;
      case 400:  blit(the_image, 104, 53, 104, 53, 216, 13);
                 blit(objetos, 112, 106, 262, 53, 14, 13);
                 break;
    }
    
    switch(height) {
      case 1: blit(the_image, 104, 69, 104, 69, 216, 13);                
              blit(objetos, 0, 106, 104, 69, 13, 13);
              break;
      case 2: blit(the_image, 104, 69, 104, 69, 216, 13);
              blit(objetos, 14, 106, 123, 69, 14, 13);
              break;
      case 3: blit(the_image, 104, 69, 104, 69, 216, 13);
              blit(objetos, 28, 106, 143, 69, 14, 13);
              break;
      case 4: blit(the_image, 104, 69, 104, 69, 216, 13);
              blit(objetos, 42, 106, 164, 69, 14, 13);
              break;
      case 5: blit(the_image, 104, 69, 104, 69, 216, 13);
              blit(objetos, 56, 106, 183, 69, 14, 13);
              break;
    }
    
    switch(visual) {
      case 1: blit(the_image, 95, 110, 95, 110, 9, 57);
              blit(objetos, 10, 119, 95, 110, 9, 8);
              break;
      case 2: blit(the_image, 95, 110, 95, 110, 9, 57);
              blit(objetos, 10, 119, 95, 122, 9, 8);
              break;
      case 3: blit(the_image, 95, 110, 95, 110, 9, 57);
              blit(objetos, 10, 119, 95, 135, 9, 8);
              break;
      case 4: blit(the_image, 95, 110, 95, 110, 9, 57);                
              blit(objetos, 10, 119, 95, 147, 9, 8);
              break;
      case 5: blit(the_image, 95, 110, 95, 110, 9, 57);                
              blit(objetos, 10, 119, 95, 159, 9, 8);
              break;
    }

    if(antvisual != visual) {
      antvisual = visual;
      switch(visual) {
        case 1: al_destroy_bitmap(objetos);
                objetos = al_load_bitmap("../graphic/objetos2.pcx");
                break;
        case 2: al_destroy_bitmap(objetos);
                objetos = al_load_bitmap("../graphic/objetos3.pcx");
                break;
        case 3: al_destroy_bitmap(objetos);
                objetos = al_load_bitmap("../graphic/objetos1.pcx");
                break;
        case 4: al_destroy_bitmap(objetos);
                objetos = al_load_bitmap("../graphic/objetos.pcx");
                break;
        case 5: al_destroy_bitmap(objetos);
                objetos = al_load_bitmap("../graphic/objetos4.pcx");
                break;
      }

      antvisual = visual;
    }

    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 40, SCREEN_X, SCREEN_Y, 0);
    al_flip_display();
  }

  al_destroy_bitmap(the_image);
  the_image = al_load_bitmap("../graphic/menu.pcx");
}

void tabla_records() {
  char s[50];
  ALLEGRO_MOUSE_STATE mouse;

  populate_records();

  al_destroy_bitmap(the_image);
  the_image = al_load_bitmap("../graphic/record.pcx");

  while(!(mouse.buttons & 2)) {
    al_set_target_bitmap(bitmap);
    al_draw_bitmap(the_image, 0, 0, 0);

    al_get_mouse_state(&mouse);

    for (int num_record = 0; num_record < 10; num_record++) {
      sprintf(s, "%d.", num_record + 1);
      al_draw_text(font, al_map_rgb(255, 255, 255), 36, 55 + num_record*11, ALLEGRO_ALIGN_LEFT, s);
      sprintf(s, "%s", records[num_record].nombre);
      al_draw_text(font, al_map_rgb(255, 255, 255), 60, 55 + num_record*11, ALLEGRO_ALIGN_LEFT, s);
      sprintf(s, "%d", records[num_record].lineas);
      al_draw_text(font, al_map_rgb(255, 255, 255), 150, 55 + num_record*11, ALLEGRO_ALIGN_LEFT, s);
      sprintf(s, "%d", records[num_record].puntos);
      al_draw_text(font, al_map_rgb(255, 255, 255), 215, 55 + num_record*11, ALLEGRO_ALIGN_LEFT, s);
    }

    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 40, SCREEN_X, SCREEN_Y, 0);
    al_flip_display();
  }

  al_destroy_bitmap(the_image);
  the_image = al_load_bitmap("../graphic/menu.pcx");
}



int main(int argc, char *argv[])
{
  ALLEGRO_MOUSE_STATE    mouse_state;
  // other variables
  int final, dif_anterior;
  char a;
  bool start_game;
  bool options;
  bool records;

 // Check arguments
  if(argc != 1) {
    printf("Error: wrong parameters. Usage: XXXX\n");
    exit(-1);
  }

  // allegro initializations
  if(!al_init()) {
    printf("Error: failed to initialize allegro!\n");
    return -1;
  }

  if(!al_install_keyboard()) {
    printf("Error: failed to initialize keyboard!\n");
    return -1;
  }

  if(!al_install_mouse()) {
    printf("Error: failed to initialize keyboard!\n");
    return -1;
  }

  al_set_new_display_flags(ALLEGRO_WINDOWED);
  display = al_create_display(SCREEN_X, SCREEN_Y);
  if(!display) {
    printf("Error: failed to create display!\n");
    return -1;
  }

  if(!al_init_image_addon()) {
    printf("Error: failed to load image addon!\n");
    return -1;
  }

  bitmap = al_create_bitmap(320, 240);
  if(!bitmap) {
    printf("Error: failed to create bitmap!\n");
    al_destroy_display(display);
    return -1;
  }

  bitmap_aux = al_create_bitmap(320, 240);
  if(!bitmap_aux) {
    printf("Error: failed to create bitmap!\n");
    al_destroy_display(display);
    return -1;
  }

  if(!al_init_primitives_addon()) {
    printf("Error: failed to initialize allegro primitives!\n");
    return -1;
  }

  al_init_font_addon();       // initialize the font addon
  al_init_ttf_addon();        // initialize the ttf (True Type Font) addon

  font = al_load_ttf_font("../fonts/verdana.ttf", 10,0 );

  if(!al_install_audio()) {
    printf("Error: failed to initialize audio!\n");
    return -1;
  }

  if(!al_init_acodec_addon()) {
    printf("Error: failed to initialize audio codecs!\n");
    return -1;
  }

  if(!al_reserve_samples(4)) {
    printf("Error: failed to reserve samples!\n");
    return -1;
  }

  event_queue = al_create_event_queue();
  if(!event_queue) {
    printf("Error: failted to create event_queue!\n");
    return -1;
  }

  al_register_event_source(event_queue, al_get_keyboard_event_source());

  intro       = al_load_sample("../music/roki.ogg");
  music_final = al_load_sample("../music/roki.ogg");
  objetos     = al_load_bitmap("../graphic/objetos1.pcx");
  the_image   = al_load_bitmap("../graphic/menu.pcx");

  final = FALSE;

  music_intro_instance = al_create_sample_instance(intro);
  music_final_instance = al_create_sample_instance(music_final);
  al_attach_sample_instance_to_mixer(music_intro_instance, al_get_default_mixer());

  al_set_sample_instance_playmode(music_intro_instance, ALLEGRO_PLAYMODE_LOOP);
  al_play_sample_instance(music_intro_instance);

  al_set_target_bitmap(bitmap);
  al_clear_to_color(al_map_rgb(0, 0, 0));
  al_set_target_bitmap(al_get_backbuffer(display));
  al_flip_display();

  al_show_mouse_cursor(display);   

  while(!final) {
    start_game = false;
    options = false;
    records = false;

    al_set_target_bitmap(bitmap);
    al_draw_bitmap(the_image, 0, 0, 0);

    al_get_mouse_state(&mouse_state);

    int mouse_x = mouse_state.x/2;
    int mouse_y = (mouse_state.y - 40)/2;

    if(((mouse_state.buttons & 1) && (mouse_x >= 122) && (mouse_x <= 206))) {
      if((mouse_y >= 75 && mouse_y <= 90)) {        
        al_draw_bitmap_region(objetos, 0, 42, 84, 16, 122, 75, 0);        
        start_game = true;
      }
      if(mouse_y >= 97 && mouse_y <= 112) {
        al_draw_bitmap_region(objetos, 0, 58, 84, 16, 122, 97, 0);        
        options = true;
      }
      if(mouse_y >= 119 && mouse_y <= 134) {
        al_draw_bitmap_region(objetos, 0, 74, 84, 16, 122, 119, 0);
        records = true;
      }
      if(mouse_y >= 141 && mouse_y <= 155) {      
        al_draw_bitmap_region(objetos, 0, 90, 84, 16, 122, 141, 0);
        final = TRUE;
      }
    }
    al_set_target_bitmap(al_get_backbuffer(display));
    al_draw_scaled_bitmap(bitmap, 0, 0, 320, 240, 0, 40, SCREEN_X, SCREEN_Y, 0);
    al_flip_display();

    if (start_game) {
      al_stop_sample_instance(music_intro_instance);
      al_hide_mouse_cursor(display);
      dif_anterior = dif;
      juego();
      dif = dif_anterior;
      al_show_mouse_cursor(display);
      al_stop_sample_instance(last_music_instance);
      al_play_sample_instance(music_intro_instance);
    } else if (records) {
      tabla_records();
    } else if (options) {
      opciones();
    }

  }
}
