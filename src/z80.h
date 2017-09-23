#ifndef Z80_H
#define Z80_H

#include <stdio.h>
#include <cstring>
#include "bus.hpp"

#define SHIFTFLAGS(v) regs.SF = ((v & 0x80) != 0); \
                      regs.ZF = (v == 0); \
                      regs.HF = regs.NF = 0; \
                      regs.PF = parityTable[(unsigned char)(v)];

static bool halfcarryTable[] = { 
    0, 1, 1, 1, 0, 0, 0, 1
};
static bool subhalfcarryTable[] = {
    0, 0, 1, 0, 1, 0, 1, 1
};
static bool overflowTable[] = {
    0, 0, 0, 1, 1, 0, 0, 0
};
static bool suboverflowTable[] = {
    0, 1, 0, 0, 0, 0, 1, 0
};
static bool parityTable[] = {
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    false, true, true, false, true, false, false, true, true, false, false, true, false, true, true, false,
    true, false, false, true, false, true, true, false, false, true, true, false, true, false, false, true,
};                      


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

  protected:
    void INC_R8(unsigned char &r);
    void DEC_R8(unsigned char &r);
    void ADD_R8(unsigned char v);
    void ADC_R8(unsigned char v);
    void AND_R8(unsigned char v);
    void CMP_R8(unsigned char v);
    void CMP_R8_NOFLAGS(unsigned char r, unsigned char v);
    void OR_R8(unsigned char v);
    void SUB_R8(unsigned char v);
    void SBC_R8(unsigned char &r, unsigned char v);
    void XOR_R8(unsigned char v);

    void ADD_R16(unsigned short &r, unsigned short v);
    void ADC_R16(unsigned short &r, unsigned short v);
    void SBC_R16(unsigned short &r, unsigned short v);

    void RL8(unsigned char &val);
    void RLC8(unsigned char &val);
    void RR8(unsigned char &val);
    void RRC8(unsigned char &val);
    void SLA8(unsigned char &val);
    void SRA8(unsigned char &val);
    void SRL8(unsigned char &val);
    void SLI8(unsigned char &val);

    void BIT8(unsigned char val, unsigned char bit);

    void IN8(unsigned char &val, unsigned short port);

    unsigned short Read16(unsigned short addr);
    void Write16(unsigned short addr, unsigned short value);

    void Push16(unsigned short value);
    unsigned short Pop16();

    void EmulateOneCB();
    void EmulateOneED();
    void EmulateOneXX();
    void EmulateOneXXCB();


};

#endif
