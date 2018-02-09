// Constant values (FIXME use const var instead of define)
#define SCREEN_X_ORIG 1024
#define SCREEN_Y_ORIG  768 
#define SCREEN_X      1024
#define SCREEN_Y       768

#define LEFT   0x0001
#define RIGHT  0x0002
#define UP     0x0004
#define DOWN   0x0008
#define FIRE   0x0010
#define NO_KEY 0x0020
#define PAUSE  0x0040
#define ESC    0x0080


#define LEFT_SCROLL  0x0040
#define RIGHT_SCROLL 0x0080
#define UP_SCROLL    0x0100
#define DOWN_SCROLL  0x0200

// Estado actual del personaje. Estos son macro estados, internamente
// existen estados intermedios según la animación en la que se encuentre
#define WARRIOR_PARADO                        0
#define WARRIOR_CAMINANDO                     1
#define WARRIOR_SALTANDO                      2
#define WARRIOR_CAYENDO                       3
#define WARRIOR_COGER_ESPADA                  4
#define WARRIOR_GOLPEANDO                     5
#define WARRIOR_EN_GUARDIA                    6
#define WARRIOR_AGACHADO                      7
#define WARRIOR_EN_ESCALERAS_LEFT_PARADO      8
#define WARRIOR_EN_ESCALERAS_LEFT_CAMINANDO   9
#define WARRIOR_EN_ESCALERAS_RIGHT_PARADO    10
#define WARRIOR_EN_ESCALERAS_RIGHT_CAMINANDO 11

#define WARRIOR_RANA_PARADO    12
#define WARRIOR_RANA_CAMINANDO 13
#define WARRIOR_RANA_SALTANDO  14
#define WARRIOR_RANA_CAYENDO   15

// Sentido en hacia el que está orientado el personaje
#define WARRIOR_SENTIDO_IZQUIERDA 0
#define WARRIOR_SENTIDO_DERECHA   1

// Tipos de tiles
#define TILE_NO_COL              0
#define TILE_COLISIONABLE        1
#define TILE_PENDIENTE_IZQUIERDA 2
#define TILE_PENDIENTE_DERECHA   3

// Tipo de pintado de tiles
#define TILE_PINTADO_DETRAS      0
#define TILE_PINTADO_DELANTE     1
#define TILE_PINTADO_ESCALERAS   2

#define FRAMES_AGUA 48
#define DIST_DESP_AGUA 4

// Estado del juego
#define TO_NEXT_LEVEL            1
#define ESC_FROM_LEVEL           2
