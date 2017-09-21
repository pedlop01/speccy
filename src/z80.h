#ifndef Z80_H
#define Z80_H

#include <cstring>
#include "bus.h"

struct Z80Registers {
    // AF register pair
    union {
        unsigned short AF;
        struct {
            union {
                unsigned char F; // LSB
                struct {
                    unsigned char CF : 1;
                    unsigned char NF : 1;
                    unsigned char PF : 1;
                    unsigned char XF : 1;
                    unsigned char HF : 1;
                    unsigned char YF : 1;
                    unsigned char ZF : 1;
                    unsigned char SF : 1;
                };
            };
            unsigned char A; // MSB
        };
    };

    // BC register pair
    union {
        unsigned short BC;
        struct {
            unsigned char C; // LSB
            unsigned char B; // MSB
        };
    };

    // DE register pair
    union {
        unsigned short DE;
        struct {
            unsigned char E; // LSB
            unsigned char D; // MSB
        };
    };

    // HL register pair
    union {
        unsigned short HL;
        struct {
            unsigned char L; // LSB
            unsigned char H; // MSB
        };
    };

    // IR register pair
    union {
      unsigned short IR;
      struct {
        union {
          unsigned char R; // LSB
          struct {
            unsigned char R60 : 7;
            unsigned char R7  : 1;
          };
        };
        unsigned char I; // MSB
      };
    };

    unsigned short IX;
    unsigned short IY;
    unsigned short SP;
    unsigned short PC;

    unsigned short altAF;
    unsigned short altBC;
    unsigned short altDE;
    unsigned short altHL;

    // XX register pair
    union {
        unsigned short XX;
        struct {
            unsigned char LX; // LSB
            unsigned char HX; // MSB
        };
    };

    unsigned char iff1A;
    unsigned char iff1B;
    unsigned char IM;

};

class Z80 {
  public:
    Z80();

    void EmulateOne();

  public:
    unsigned int tStates;
    Z80Registers regs;
    BusComponent<0x0000, 0x10000>* DataBus;
    BusComponent<0x0000, 0x10000>* IOBus;
};

#endif
