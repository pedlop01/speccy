#include "z80.h"

Z80::Z80 () {
    memset(&regs, 0, sizeof(regs));
    DataBus = nullptr;
    IOBus = nullptr;
    tStates = 0;    
}

void Z80::INC_R8(unsigned char &r) {
	r++;
  regs.SF = (r & 0x80) != 0;
  regs.ZF = (r == 0);
  regs.HF = ((r & 0x0F) == 0);
  regs.PF = (r == 0x80);
  regs.NF = 0;
}

void Z80::DEC_R8(unsigned char &r) {
  regs.NF = 1;
	regs.HF = (r & 0x0F ? 0 : 1);
	r--;
  regs.SF = ((r & 0x80) != 0);
  regs.ZF = (r == 0);
  regs.PF = (r == 0x7F);
}

void Z80::ADD_R8(unsigned char v) {
  unsigned short aux = regs.A + v;
	unsigned char idx = ((regs.A & 0x88) >> 3) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 1);
	regs.A = (unsigned char)aux;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = halfcarryTable[idx & 0x07];
  regs.PF = overflowTable[idx >> 4];
  regs.NF = 0;
  regs.CF = (aux & 0x100) != 0;
}

void Z80::ADC_R8(unsigned char v) {
  unsigned short aux = regs.A + v + (regs.CF ? 1 : 0);
	unsigned char idx = ((regs.A & 0x88) >> 3) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 1);
	regs.A = (unsigned char)aux;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = halfcarryTable[idx & 0x07];
  regs.PF = overflowTable[idx >> 4];
  regs.NF = 0;
  regs.CF = (aux & 0x100) != 0;
}

void Z80::AND_R8(unsigned char v) {
	regs.A &= v;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = 1;
  regs.PF = parityTable[regs.A];
  regs.NF = regs.CF = 0;
}

void Z80::CMP_R8(unsigned char v) {
  unsigned short aux = regs.A-v;
	unsigned char idx = ((regs.A & 0x88) >> 3) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 1);
  regs.SF = (aux & 0x80) != 0;
  regs.CF = (aux & 0x100) ? 1 : 0;
	regs.ZF = (aux == 0);
  regs.HF = subhalfcarryTable[idx & 0x07];
  regs.PF = suboverflowTable[idx >> 4];
  regs.NF = 1;
}

void Z80::CMP_R8_NOFLAGS(unsigned char r, unsigned char v) {
  // Same as CMP_R8 without updating the parity and carry flags
  unsigned short aux = r-v;
	unsigned char idx = ((r & 0x88) >> 3) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 1);
  regs.SF = (aux & 0x80) != 0;
  regs.ZF = (aux == 0);
  regs.HF = subhalfcarryTable[idx & 0x07];
  regs.NF = 1;
}

void Z80::OR_R8(unsigned char v) {
	regs.A |= v;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = regs.NF = regs.CF = 0;
  regs.PF = parityTable[regs.A];
}

void Z80::SUB_R8(unsigned char v) {
  unsigned short aux = regs.A - v;
	unsigned char idx = ((regs.A & 0x88) >> 1) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 3);
	regs.A = (unsigned char)aux;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = subhalfcarryTable[idx & 0x07];
  regs.PF = suboverflowTable[idx >> 4];
  regs.NF = 1;
  regs.CF = (aux & 0x100) != 0;
}

void Z80::SBC_R8(unsigned char &r, unsigned char v) {
	unsigned short aux = r - v;
	if (regs.CF)
		aux--;
	
	unsigned char idx = ((r & 0x88) >> 1) | ((v & 0x88) >> 2) |	((aux & 0x88) >> 3);
	r = (unsigned char)aux;
  regs.SF = (r & 0x80) != 0;
  regs.ZF = (r == 0);
  regs.HF = subhalfcarryTable[idx & 0x07];
  regs.PF = suboverflowTable[idx >> 4];
  regs.NF = 1;
  regs.CF = (aux & 0x100) != 0;
}
}

void Z80::XOR_R8(unsigned char v) {
	regs.A ^= v;
  regs.SF = (regs.A & 0x80) != 0;
  regs.ZF = (regs.A == 0);
  regs.HF = regs.NF = regs.CF = 0;
  regs.PF = parityTable[regs.A];
}

void Z80::ADD_R16(unsigned short &r, unsigned short v) {
  unsigned int aux = r+v;
  unsigned char idx = ((r & 0x800) >> 11) | ((v & 0x800) >> 10) | ((aux & 0x800) >> 9);
  r = aux;
  regs.HF = halfcarryTable[idx];
  regs.NF = false;
  regs.CF = (aux & 0x10000) != 0;
}

void Z80::ADC_R16(unsigned short &r, unsigned short v) {
  unsigned int aux = r+v+(regs.CF ? 1 : 0);
  unsigned char idx = ((r & 0x8800) >> 11) | ((v & 0x8800) >> 10) | ((aux & 0x8800) >> 9);
  r = aux;
	regs.SF = ((r & 0x8000) != 0);
	regs.ZF = (r == 0);
  regs.HF = halfcarryTable[idx & 0x7];
	regs.PF = overflowTable[idx >> 4];
  regs.NF = false;
  regs.CF = (aux & 0x10000) != 0;
}

void Z80::SBC_R16(unsigned short &r, unsigned short v) {
  unsigned int aux = r - v - (regs.CF ? 1 : 0);
	unsigned char idx = ((r & 0x8800) >> 9) | ((v & 0x8800) >> 10) |	((aux & 0x8800) >> 11);
	r = (unsigned short)aux;
  regs.SF = (r & 0x8000) != 0;
  regs.ZF = (r == 0);
  regs.HF = subhalfcarryTable[idx & 0x07];
  regs.PF = suboverflowTable[idx >> 4];
  regs.NF = 1;
  regs.CF = (aux & 0x10000) != 0;
}

void Z80::RL8(unsigned char &val) {
  bool cf = (val & 0x80) != 0;
  val <<= 1;
  val |= (regs.CF ? 0x01 : 0x00);
  SHIFTFLAGS(val);
  regs.CF = cf;
}

void Z80::RLC8(unsigned char &val) {
	val = (val << 1) | (val & 0x80 ? 0x01 : 0x00);
	regs.CF = (val & 0x01) != 0;
  SHIFTFLAGS(val);
}

void Z80::RR8(unsigned char &val) {
  bool cf = (val & 0x01) != 0;
  val >>= 1;
  val |= (regs.CF ? 0x80 : 0x00);
  SHIFTFLAGS(val);
  regs.CF = cf;
}

void Z80::RRC8(unsigned char &val) {
  regs.CF = (val & 0x01) != 0;
  val >>= 1;
  val |= regs.CF ? 0x80 : 0x00;
  SHIFTFLAGS(val);
}

void Z80::SLA8(unsigned char &val) {
  regs.CF = (val & 0x80) != 0;
  val <<= 1;
  SHIFTFLAGS(val);
}

void Z80::SRA8(unsigned char &val) {
  regs.CF = (val & 0x01) != 0;
  val = (val & 0x80) | (val >> 1);
  SHIFTFLAGS(val);
}

void Z80::SRL8(unsigned char &val) {
  regs.CF = (val & 0x01) != 0;
  val >>= 1;
  SHIFTFLAGS(val);
}

void Z80::SRI8(unsigned char &val) {
  regs.CF = (val & 0x80) != 0;
  val = (val << 1) | 0x01;
  SHIFTFLAGS(val);
}

// Bit checking, setting and clearing
void Z80::BIT8(unsigned char val,unsigned char bit)
{
  unsigned char mask = (1 << bit);
  regs.ZF = regs.PF = ((val & mask) == 0);
  regs.SF = ((val & mask) == 0x80);
  regs.HF = 1;
  regs.NF = 0;
}

void Z80::IN8(unsigned char &val, unsigned short port) {
	val = IOBus->Read(port);
	regs.SF = (val & 0x80) != 0;
	regs.ZF = (val == 0x00);
	regs.HF = 0;
	regs.PF = parityTable[val];
	regs.NF = 0;
}

void Z80::Read16(unsigned short addr) {
  return(Data->Read(addr) | DataBus->Read(addr+1) << 8);
}

void Z80::Write16(unsigned short addr, unsigned short value) {
  DataBus->Write(addr, value & 0xFF);
  DataBus->Write(addr+1, value >> 8);
}

void Z80::Push16(unsigned short value) {
  regs.SP -= 2;
  Write16(regs.SP, value);
}

void Z80::Pop16() {
  unsigned short retVal = Read16(regs.SP);
  regs.SP += 2;
  return(retVal);
}

void Z80::EmulateOne() {
    // Fetch next instruction
    unsigned char op = DataBus->Read(regs.PC++);
    // Increment instruction counter register
    regs.R60++;

    switch(op) {
        // nop
        case 0x00:
          tStates += 4;
          break;

        // ld bc, NN
        case 0x01:
          tStates += 10;
          regs.C = DataBus->Read(regs.PC++);
          regs.B = DataBus->Read(regs.PC++);
          break;

        // ld (bc), a
        case 0x02:
          tStates += 7;
          DataBus->Write(regs.BC, regs.A);
          break;

        // inc bc
        case 0x03:
          tStates += 6;
          regs.BC++;
          break;

				// inc b
        case 0x04:
				  tStates += 4;
				  INC_R8(regs.B);
				  break;
				
				// dec b
			  case 0x05:
				  tStates += 4;
				  DEC_R8(regs.B);
				  break;
				
				// ld b,N
			  case 0x06:
				  tStates += 7;
          regs.B = DataBus->Read(regs.PC++);
				  break;
				
				// rlca
			  case 0x07:
				  tStates += 4;
					regs.A = (regs.A << 1) | (regs.A >> 7);
					regs.HF = regs.NF = 0;
					regs.CF = (regs.A & 0x01) != 0;				  
				  break;
				
        // ex af,af'
			case 0x08:
				tStates += 4;
        {
          unsigned short aux = regs.AF;
          regs.AF = regs.altAF;
          regs.altAF = aux;
        }
        break;

				// add hl,bc
			  case 0x09:
				  tStates += 11;
          ADD_R16(regs.HL,regs.BC);
				  break;
				
				// ld a,(bc)
			  case 0x0A:
				  tStates += 7;
				  regs.A = DataBus->Read(regs.BC);
				  break;
				
				// dec bc
			  case 0x0B:
				  tStates += 6;
				  regs.BC--;
				  break;
				
				// inc c
			  case 0x0C:
				  tStates += 4;
          INC_R8(regs.C);
				  break;

				// dec c
			  case 0x0D:
				  tStates += 4;
				  DEC_R8(regs.C);
				  break;
				
				// ld c,N
			  case 0x0E:
				  tStates += 7;
				  regs.C = DataBus->Read(regs.PC++);
				  break;
				
        // rrca
        case 0x0F:
				  tStates += 4;
          regs.CF = (regs.A & 0x01) != 0;
				  regs.A = (regs.A >> 1) | (regs.A << 7);
          regs.HF = 0;
          regs.NF = 0;
				  break;

				// djnz D
			  case 0x10:
          regs.B--;
				  tStates += 8;
				  if (regs.B) {
					  tStates += 5;
            regs.PC += (char)DataBus->Read(regs.PC);
				  }
				  regs.PC++;
				  break;

        // ld de,NN
			  case 0x11:
				  tStates += 10;
          regs.E = DataBus->Read(regs.PC++);
          regs.D = DataBus->Read(regs.PC++);
				  break;
				
				// ld (de),a
			  case 0x12:
				  tStates += 7;
          DataBus->Write(regs.DE,regs.A);
				  break;
				
				// inc de
			  case 0x13:
				  tStates += 6;
				  regs.DE++;
				  break;
				
				// inc d
			  case 0x14:
				  tStates += 4;
				  INC_R8(regs.D);
				  break;
				
				// dec d
			  case 0x15:
				  tStates += 4;
				  DEC_R8(regs.D);
				  break;
				
				// ld d,N
			  case 0x16:
				  tStates += 7;
          regs.D = DataBus->Read(regs.PC++);
				  break;
				
				// rla
			  case 0x17:
				  tStates += 4;
          {
					  unsigned char aux = regs.A;
            regs.A = (regs.A << 1) | (regs.CF ? 0x01 : 0x00);
            regs.CF = (aux & 0x80) != 0;
            regs.NF = 0;
            regs.HF = 0;
          }
				  break;
				
				// jr D
			  case 0x18:
				  tStates += 12;
          regs.PC += 1 + (char)DataBus->Read(regs.PC);
				  break;
				
				// add hl,de
			  case 0x19:
				  tStates += 11;
          ADD_R16(regs.HL,regs.DE);
				  break;
				
				// ld a,(de)
			  case 0x1A:
				  tStates += 7;
          regs.A = DataBus->Read(regs.DE);
				  break;
				
				// dec de
			  case 0x1B:
				  tStates += 6;
				  regs.DE--;
				  break;
				
				// inc e
			  case 0x1C:
				  tStates += 4;
          INC_R8(regs.E);
				  break;
				
				// dec e
			  case 0x1D:
				  tStates += 4;
          DEC_R8(regs.E);
				  break;
				
				// ld e,N
			  case 0x1E:
				  tStates += 7;
          regs.E = DataBus->Read(regs.PC++);
				  break;
				
				// rra
			  case 0x1F:
				  tStates += 4;
          {
					  unsigned char aux = regs.A;
            regs.A = (regs.A >> 1) | (regs.CF ? 0x80 : 0x00);
            regs.CF = (aux & 0x01) != 0;
            regs.NF = 0;
            regs.HF = 0;
          }
				  break;
				
				// jr nz,D
			  case 0x20:
				  tStates += 7;
				  if (!regs.ZF) {
					  tStates += 5;
            regs.PC += (char)DataBus->Read(regs.PC);
				  }
				  regs.PC++;
				  break;
				
				// ld hl,NN
			  case 0x21:
				  tStates += 10;
          regs.L = DataBus->Read(regs.PC++);
          regs.H = DataBus->Read(regs.PC++);
				  break;
				
        // ld (NN),hl
			  case 0x22:
				  tStates += 16;
          Write16(Read16(regs.PC),regs.HL);
          regs.PC+=2;
				  break;

        // inc hl
			  case 0x23:
				  tStates += 6;
				  regs.HL++;
				  break;
				
				// inc h
			  case 0x24:
				  tStates += 4;
				  INC_R8(regs.H);
				  break;
				
				// dec h
			  case 0x25:
				  tStates += 4;
				  DEC_R8(regs.H);
				  break;
				
				// ld h,N
			  case 0x26:
				  tStates += 7;
          regs.H = DataBus->Read(regs.PC++);
				  break;
				
				// daa
			  case 0x27:
				  {
					  unsigned char add = 0;
					  unsigned char carry = regs.CF ? 1 : 0;
					  if (regs.HF || ((regs.A & 0x0F) > 9))
					  	add = 6;
					  if (carry || (regs.A > 0x99))
					  	add |= 0x60;
					  if (regs.A > 0x99)
					  	carry = 1;
					  if (regs.NF) {
					  	SUB_R8(add);
					  } else {
					  	ADD_R8(add);
					  }
  
					  regs.CF = carry;
					  regs.PF = parityTable[regs.A];
  				}
	  			break;
				
				// jr z,D
			  case 0x28:
				  tStates += 7;
				  if (regs.ZF) {
					  tStates += 5;
            regs.PC += (char)DataBus->Read(regs.PC);
				  }
				  regs.PC++;
				  break;
				
				// add hl,hl
			  case 0x29:
				  tStates += 11;
          ADD_R16(regs.HL,regs.HL);
				  break;
				
				// ld hl,(NN)
			  case 0x2A:
				  tStates += 16;
          regs.HL = Read16(Read16(regs.PC));
				  regs.PC+=2;
				  break;
				
				// dec hl
			  case 0x2B:
				  tStates += 6;
				  regs.HL--;
				  break;
				
				// inc l
			  case 0x2C:
				  tStates += 4;
          INC_R8(regs.L);
				  break;
				
				// dec l
			  case 0x2D:
				  tStates += 4;
          DEC_R8(regs.L);
				  break;
				
				// ld l,N
			  case 0x2E:
				  tStates += 7;
				  regs.L = DataBus->Read(regs.PC++);
				  break;
				
				// cpl
			  case 0x2F:
				  tStates += 4;
				  regs.A ^= 0xff;
				  regs.HF = true;
				  regs.NF = true;
				  break;
				
				// jr nc,D
			  case 0x30:
				  tStates += 7;
				  if(!regs.CF) {
					  tStates += 5;
            regs.PC += (char)DataBus->Read(regs.PC);
				  }
				  regs.PC++;
				  break;
				
				// ld sp,NN
			  case 0x31:
				  tStates += 10;
          regs.SP = Read16(regs.PC);
          regs.PC += 2;
				  break;
				
				// ld (NN),a
			  case 0x32:
				  tStates += 13;
          DataBus->Write(Read16(regs.PC),regs.A);
          regs.PC += 2;
				  break;
				
				// inc sp
			  case 0x33:
				  tStates += 6;
				  regs.SP++;
				  break;
				
				// inc (hl)
			  case 0x34:
				  tStates += 11;
          {
            unsigned char val = DataBus->Read(regs.HL);
					  INC_R8(val);
            DataBus->Write(regs.HL, val);
          }
	  			break;
				
				// dec (hl)
			  case 0x35:
				  tStates += 11;
          {
            unsigned char val = DataBus->Read(regs.HL);
					  DEC_R8(val);
            DataBus->Write(regs.HL, val);
          }
				  break;
				
				// ld (hl),N
			  case 0x36:
				  tStates += 10;
          DataBus->Write(regs.HL,DataBus->Read(regs.PC++));
				  break;
				
				// scf
			  case 0x37:
				  tStates += 4;
          regs.HF = regs.NF = false;
          regs.CF = true;
				  break;
				
				// jr c,D
			  case 0x38:
				  tStates += 7;
				  if (regs.CF) {
					  tStates += 5;
            regs.PC += (char)DataBus->Read(regs.PC);
				  }
				  regs.PC++;
				  break;
				
				// add hl,sp
			  case 0x39:
				  tStates += 11;
          ADD_R16(regs.HL,regs.SP);
				  break;
				
				// ld a,(NN)
			  case 0x3A:
				  tStates += 13;
          regs.A = DataBus->Read(Read16(regs.PC));
          regs.PC += 2;
				  break;
				
				// dec sp
			  case 0x3B:
				  tStates += 6;
          regs.SP--;
				  break;
				
				// inc a
			  case 0x3C:
				  tStates += 4;
          INC_R8(regs.A);
				  break;
				
				// dec a
			  case 0x3D:
				  tStates += 4;
          DEC_R8(regs.A);
				  break;
				
				// ld a,N
			  case 0x3E:
				  tStates += 7;
          regs.A = DataBus->Read(regs.PC++);
				  break;
				
				// ccf
			  case 0x3F:
				  tStates += 4;
          regs.HF = regs.CF;
          regs.NF = 0;
          regs.CF = !regs.CF;
				  break;
				
				// ld b,b
			  case 0x40:
				  tStates += 4;
				  break;
				
				// ld b,c
			  case 0x41:
				  tStates += 4;
          regs.B = regs.C;
				  break;
				
				// ld b,d
			  case 0x42:
  				tStates += 4;
          regs.B = regs.D;
		  		break;
				
				// ld b,e
			  case 0x43:
				  tStates += 4;
          regs.B = regs.E;
				  break;
				
				// ld b,h
			  case 0x44:
				  tStates += 4;
          regs.B = regs.H;
				  break;
				
				// ld b,l
			  case 0x45:
				  tStates += 4;
          regs.B = regs.L;
				  break;
				
				// ld b,(hl)
			  case 0x46:
  				tStates += 7;
          regs.B = DataBus->Read(regs.HL);
		  		break;
				
				// ld b,a
			  case 0x47:
				  tStates += 4;
          regs.B = regs.A;
				  break;
				
				// ld c,b
			  case 0x48:
				  tStates += 4;
          regs.C = regs.B;
				  break;
				
				// ld c,c
			  case 0x49:
				  tStates += 4;
				  break;
				
				// ld c,d
			  case 0x4A:
				  tStates += 4;
          regs.C = regs.D;
				  break;
				
				// ld c,e
			  case 0x4B:
				  tStates += 4;
          regs.C = regs.E;
				  break;
				
				// ld c,h
			  case 0x4C:
				  tStates += 4;
          regs.C = regs.H;
				  break;
				
				// ld c,l
			  case 0x4D:
				  tStates += 4;
          regs.C = regs.L;
				  break;
				
				// ld c,(hl)
			  case 0x4E:
				  tStates += 7;
          regs.C = DataBus->Read(regs.HL);
				  break;
				
				// ld c,a
			  case 0x4F:
				  tStates += 4;
          regs.C = regs.A;
				  break;
				
				// ld d,b
			  case 0x50:
				  tStates += 4;
          regs.D = regs.B;
				  break;
				
				// ld d,c
			  case 0x51:
   				tStates += 4;
          regs.D = regs.C;
				  break;
				
				// ld d,d
			  case 0x52:
  				tStates += 4;
	  			break;
				
				// ld d,e
			  case 0x53:
				  tStates += 4;
          regs.D = regs.E;
				  break;
				
				// ld d,h
			  case 0x54:
				  tStates += 4;
          regs.D = regs.H;
				  break;
				
				// ld d,l
			  case 0x55:
				  tStates += 4;
          regs.D = regs.L;
				  break;
				
				// ld d,(hl)
			  case 0x56:
				  tStates += 7;
          regs.D = DataBus->Read(regs.HL);
				  break;
				
				// ld d,a
			  case 0x57:
  				tStates += 4;
          regs.D = regs.A;
				  break;

				// ld e,b
			  case 0x58:
				  tStates += 4;
          regs.E = regs.B;
				  break;
				
				// ld e,c
			  case 0x59:
				  tStates += 4;
          regs.E = regs.C;
				  break;
				
				// ld e,d
			  case 0x5A:
  				tStates += 4;
          regs.E = regs.D;
		  		break;
				
				// ld e,e
			  case 0x5B:
				  tStates += 4;
				  break;
				
				// ld e,h
			  case 0x5C:
				  tStates += 4;
          regs.E = regs.H;
				  break;
				
				// ld e,l
			  case 0x5D:
				  tStates += 4;
          regs.E = regs.L;
				  break;
				
				// ld e,(hl)
			  case 0x5E:
				  tStates += 7;
          regs.E = DataBus->Read(regs.HL);
				  break;
				
				// ld e,a
			  case 0x5F:
				  tStates += 4;
          regs.E = regs.A;
				  break;
				
				// ld h,b
			  case 0x60:
				  tStates += 4;
          regs.H = regs.B;
				  break;
				
				// ld h,c
			  case 0x61:
				  tStates += 4;
          regs.H = regs.C;
				  break;
				
				// ld h,d
			  case 0x62:
				  tStates += 4;
          regs.H = regs.D;
				  break;
				
				// ld h,e
			  case 0x63:
				  tStates += 4;
          regs.H = regs.E;
				  break;
				
				// ld h,h
			  case 0x64:
				  tStates += 4;
				  break;
				
				// ld h,l
			  case 0x65:
				  tStates += 4;
          regs.H = regs.L;
				  break;
				
				// ld h,(hl)
			  case 0x66:
				  tStates += 7;
          regs.H = DataBus->Read(regs.HL);
				  break;
				
				// ld h,a
			  case 0x67:
				  tStates += 4;
          regs.H = regs.A;
				  break;
				
				// ld l,b
			  case 0x68:
				  tStates += 4;
          regs.L = regs.B;
				  break;
				
				// ld l,c
			  case 0x69:
				  tStates += 4;
          regs.L = regs.C;
				  break;
				
				// ld l,d
			  case 0x6A:
				  tStates += 4;
          regs.L = regs.D;
				  break;
				
				// ld l,e
			  case 0x6B:
				  tStates += 4;
          regs.L = regs.E;
				  break;
				
				// ld l,h
			  case 0x6C:
				  tStates += 4;
          regs.L = regs.H;
				  break;
				
				// ld l,l
			  case 0x6D:
				  tStates += 4;
				  break;
				
				// ld l,(hl)
			  case 0x6E:
				  tStates += 7;
          regs.L = DataBus->Read(regs.HL);
				  break;
				
				// ld l,a
			  case 0x6F:
				  tStates += 4;
          regs.L = regs.A;
				  break;
				
				// ld (hl),b
			  case 0x70:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.B);
				  break;
				
				// ld (hl),c
			  case 0x71:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.C);
				  break;
				
				// ld (hl),d
			  case 0x72:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.D);
				  break;
				
				// ld (hl),e
			  case 0x73:
  				tStates += 7;
          DataBus->Write(regs.HL,regs.E);
				  break;
				
				// ld (hl),h
			  case 0x74:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.H);
				  break;
				
				// ld (hl),l
			  case 0x75:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.L);
				  break;
				
				// halt
			  case 0x76:
				  tStates += 4;
          regs.PC--;      // Keep executing same instruction.
				  break;
				
				// ld (hl),a
			  case 0x77:
				  tStates += 7;
          DataBus->Write(regs.HL,regs.A);
				  break;
				
				// ld a,b
			  case 0x78:
				  tStates += 4;
          regs.A = regs.B;
				  break;
				
				// ld a,c
			  case 0x79:
				  tStates += 4;
          regs.A = regs.C;
				  break;
				
				// ld a,d
			  case 0x7A:
				  tStates += 4;
          regs.A = regs.D;
				  break;
				
				// ld a,e
			  case 0x7B:
				  tStates += 4;
          regs.A = regs.E;
				  break;
				
				// ld a,h
			  case 0x7C:
				  tStates += 4;
          regs.A = regs.H;
				  break;
				
				// ld a,l
			  case 0x7D:
				  tStates += 4;
          regs.A = regs.L;
				  break;

				// ld a,(hl)
			  case 0x7E:
				  tStates += 7;
          regs.A = DataBus->Read(regs.HL);
				  break;
				
				// ld a,a
			  case 0x7F:
				  tStates += 4;
				  break;
				
				// add a,b
			  case 0x80:
				  tStates += 4;
          ADD_R8(regs.B);
				  break;
				
				// add a,c
			  case 0x81:
				  tStates += 4;
          ADD_R8(regs.C);
				  break;
				
				// add a,d
			  case 0x82:
				  tStates += 4;
          ADD_R8(regs.D);
				  break;
				
				// add a,e
			  case 0x83:
				  tStates += 4;
          ADD_R8(regs.E);
				  break;
				
				// add a,h
			  case 0x84:
				  tStates += 4;
          ADD_R8(regs.H);
				  break;
				
				// add a,l
			  case 0x85:
				  tStates += 4;
          ADD_R8(regs.L);
				  break;
				
				// add a,(hl)
			  case 0x86:
				  tStates += 7;
          {
            unsigned char b = DataBus->Read(regs.HL);
            ADD_R8(b);
          }
				  break;
				
				// add a,a
			  case 0x87:
				  tStates += 4;
          ADD_R8(regs.A);
				  break;
				
				// adc a,b
			  case 0x88:
				  tStates += 4;
          ADC_R8(regs.B);
				  break;
				
				// adc a,c
			  case 0x89:
				  tStates += 4;
          ADC_R8(regs.C);
				  break;
				
				// adc a,d
			  case 0x8A:
				  tStates += 4;
          ADC_R8(regs.D);
				  break;
				
				// adc a,e
			  case 0x8B:
  				tStates += 4;
          ADC_R8(regs.E);
				  break;
				
				// adc a,h
			  case 0x8C:
  				tStates += 4;
          ADC_R8(regs.H);
				  break;
				
				// adc a,l
			  case 0x8D:
				  tStates += 4;
          ADC_R8(regs.L);
				  break;
				
				// adc a,(hl)
			  case 0x8E:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            ADC_R8(val);
          }
				  break;
				
				// adc a,a
			  case 0x8F:
				  tStates += 4;
          ADC_R8(regs.A);
				  break;
				
				// sub b
			  case 0x90:
				  tStates += 4;
          SUB_R8(regs.B);
				  break;
				
				// sub c
			  case 0x91:
				  tStates += 4;
          SUB_R8(regs.C);
				  break;
				
				// sub d
			  case 0x92:
				  tStates += 4;
          SUB_R8(regs.D);
				  break;
				
				// sub e
			  case 0x93:
				  tStates += 4;
          SUB_R8(regs.E);
				  break;
				
				// sub h
			  case 0x94:
				  tStates += 4;
          SUB_R8(regs.H);
				  break;
				
				// sub l
			  case 0x95:
				  tStates += 4;
          SUB_R8(regs.L);
				  break;
				
				// sub (hl)
			  case 0x96:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            SUB_R8(val);
          }
				  break;
				
				// sub a
			  case 0x97:
				  tStates += 4;
          SUB_R8(regs.A);
				  break;
				
				// sbc a,b
			  case 0x98:
				  tStates += 4;
          SBC_R8(regs.A,regs.B);
				  break;
				
				// sbc a,c
			  case 0x99:
				  tStates += 4;
          SBC_R8(regs.A,regs.C);
				  break;
				
				// sbc a,d
			  case 0x9A:
  				tStates += 4;
          SBC_R8(regs.A,regs.D);
				  break;
				
				// sbc a,e
			  case 0x9B:
				  tStates += 4;
          SBC_R8(regs.A,regs.E);
				  break;
				
				// sbc a,h
			  case 0x9C:
				  tStates += 4;
          SBC_R8(regs.A,regs.H);
				  break;
				
				// sbc a,l
			  case 0x9D:
				  tStates += 4;
          SBC_R8(regs.A,regs.L);
				  break;
				
				// sbc a,(hl)
			  case 0x9E:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            SBC_R8(regs.A,val);
          }
				  break;
				
				// sbc a,a
			  case 0x9F:
				  tStates += 4;
          SBC_R8(regs.A,regs.A);
				  break;
				
				// and b
			  case 0xA0:
				  tStates += 4;
          AND_R8(regs.B);
				  break;
				
				// and c
			  case 0xA1:
				  tStates += 4;
          AND_R8(regs.C);
				  break;
				
				// and d
			  case 0xA2:
				  tStates += 4;
          AND_R8(regs.D);
				  break;
				
				// and e
			  case 0xA3:
				  tStates += 4;
          AND_R8(regs.E);
				  break;
				
				// and h
			  case 0xA4:
				  tStates += 4;
          AND_R8(regs.H);
				  break;
				
				// and l
			  case 0xA5:
				  tStates += 4;
          AND_R8(regs.L);
				  break;
				
				// and (hl)
			  case 0xA6:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            AND_R8(val);
          }
				  break;
				
				// and a
			  case 0xA7:
				  tStates += 4;
          AND_R8(regs.A);
				  break;
				
				// xor b
			  case 0xA8:
				  tStates += 4;
          XOR_R8(regs.B);
				  break;
				
				// xor c
			  case 0xA9:
				  tStates += 4;
          XOR_R8(regs.C);
				  break;
				
				// xor d
			  case 0xAA:
				  tStates += 4;
          XOR_R8(regs.D);
				  break;
				
				// xor e
			  case 0xAB:
				  tStates += 4;
          XOR_R8(regs.E);
				  break;
				
				// xor h
			  case 0xAC:
				  tStates += 4;
          XOR_R8(regs.H);
				  break;
				
				// xor l
			  case 0xAD:
				  tStates += 4;
          XOR_R8(regs.L);
				  break;
				
				// xor (hl)
			  case 0xAE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            XOR_R8(val);
          }
				  break;
				
				// xor a
			  case 0xAF:
				  tStates += 4;
          XOR_R8(regs.A);
				  break;
				
				// or b
			  case 0xB0:
				  tStates += 4;
          OR_R8(regs.B);
				  break;
				
				// or c
			  case 0xB1:
				  tStates += 4;
          OR_R8(regs.C);
				  break;
				
				// or d
			  case 0xB2:
   				tStates += 4;
          OR_R8(regs.D);
		  		break;
				
				// or e
			  case 0xB3:
     			tStates += 4;
          OR_R8(regs.E);
				  break;
				
				// or h
			  case 0xB4:
				  tStates += 4;
          OR_R8(regs.H);
				  break;
				
				// or l
			  case 0xB5:
				  tStates += 4;
          OR_R8(regs.L);
				  break;
				
				// or (hl)
			  case 0xB6:
   				tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            OR_R8(val);
          }
				  break;
				
				// or a
			  case 0xB7:
				  tStates += 4;
          OR_R8(regs.A);
				  break;
				
				// cp b
			  case 0xB8:
  				tStates += 4;
          CMP_R8(regs.B);
				  break;
				
				// cp c
			  case 0xB9:
  				tStates += 4;
          CMP_R8(regs.C);
				  break;
				
				// cp d
			  case 0xBA:
				  tStates += 4;
          CMP_R8(regs.D);
				  break;
				
				// cp e
			  case 0xBB:
				  tStates += 4;
          CMP_R8(regs.E);
				  break;
				
				// cp h
			  case 0xBC:
				  tStates += 4;
          CMP_R8(regs.H);
				  break;
				
				// cp l
			  case 0xBD:
				  tStates += 4;
          CMP_R8(regs.L);
				  break;
				
				// cp (hl)
			  case 0xBE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.HL);
            CMP_R8(val);
          }
				  break;
				
				// cp a
			  case 0xBF:
				  tStates += 4;
          CMP_R8(regs.A);
				  break;
				
				// ret nz
			  case 0xC0:
				  tStates += 5;
				  if (!regs.ZF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// pop bc
			  case 0xC1:
				  tStates += 10;
          regs.BC = Pop16();
				  break;
				
				// jp nz,NN
			  case 0xC2:
				  tStates += 10;
				  if (!regs.ZF)
            regs.PC = Read16(regs.PC);
          else
					  regs.PC += 2;
				  break;
				
				// jp NN
			  case 0xC3:
				  tStates += 10;
          regs.PC = Read16(regs.PC);
				  break;
				
				// call nz,NN
			  case 0xC4:
				  if (!regs.ZF) {
					  tStates += 17;
            Push16(regs.PC+2);
					  regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
					  regs.PC += 2;
				  }
				  break;
				
				// push bc
			  case 0xC5:
				  tStates += 11;
          Push16(regs.BC);
				  break;
				
				// add a,N
			  case 0xC6:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            ADD_R8(val);
          }
				  break;
				
				// rst 0x00
			  case 0xC7:
				  tStates += 11;
          Push16(regs.PC);
          regs.PC = 0x0000;
				  break;
				
				// ret z
			  case 0xC8:
				  tStates += 5;
				  if (regs.ZF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// ret
			  case 0xC9:
				  tStates += 10;
          regs.PC = Pop16();
				  break;
				
				// jp z,NN
			  case 0xCA:
				  tStates += 10;
				  if (regs.ZF)
            regs.PC = Read16(regs.PC);
          else
            regs.PC += 2;
				  break;


			  //  0xCB instructions prefix
			  case 0xCB:
          EmulateOneCB();
				  break;
				
				// call z,NN
			  case 0xCC:
				  if (regs.ZF) {
					  tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
					  regs.PC += 2;
				  }
				  break;
				
				// call NN
			  case 0xCD:
				  tStates += 17;
          Push16(regs.PC+2);
          regs.PC = Read16(regs.PC);
				  break;
				
				// adc a,N
			  case 0xCE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            ADC_R8(val);
          }
				  break;
				
        // rst 0x08
			  case 0xCF:
				  tStates += 11;
          Push16(regs.PC);
          regs.PC = 0x0008;
				  break;
				
				// ret nc
			  case 0xD0:
				  tStates += 5;
				  if (!regs.CF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// pop de
			  case 0xD1:
				  tStates += 10;
          regs.DE = Pop16();
				  break;
				
				// jp nc,NN
			  case 0xD2:
				  tStates += 10;
				  if (!regs.CF)
            regs.PC = Read16(regs.PC);
          else
            regs.PC += 2;
				  break;
				
				// out (N),a
			  case 0xD3:
          tStates += 11;
          // REVISIT pedlop01
          //IOBus->Write(DataBus->Read(regs.PC++),regs.A);
          // IO bus is an 8 bit address bus... and as such is decoded.
          // In reality, register A is also placed on the upper 8 bits
          // of the address bus... Something like this:
          IOBus->Write(DataBus->Read(regs.PC++) | (regs.A << 8),regs.A);
				  break;
				
				// call nc,NN
			  case 0xD4:
				  if (!regs.CF) {
					  tStates += 17;
            Push16(regs.PC + 2);
            regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
            regs.PC += 2;
				  }
				  break;
				
				// push de
			  case 0xD5:
				  tStates += 11;
          Push16(regs.DE);
				  break;
				
				// sub N
			  case 0xD6:
  				tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            SUB_R8(val);
          }
				  break;
				
				// rst 0x10
			  case 0xD7:
				  tStates += 11;
          Push16(regs.PC);
				  regs.PC = 0x0010;
				  break;
				
				// ret c
			  case 0xD8:
				  tStates += 5;
				  if (regs.CF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// exx
			  case 0xD9:
				  tStates += 4;
          {
            unsigned short aux;
            aux = regs.BC; regs.BC = regs.altBC; regs.altBC = aux;
            aux = regs.DE; regs.DE = regs.altDE; regs.altDE = aux;
            aux = regs.HL; regs.HL = regs.altHL; regs.altHL = aux;
          }
				  break;
				
				// jp c,NN
			  case 0xDA:
				  tStates += 10;
				  if (regs.CF)
            regs.PC = Read16(regs.PC);
          else
  					regs.PC += 2;
				  break;
				
				// in a,N
			  case 0xDB:
          tStates += 11;
          // REVISIT pedlop01
          // regs.A = IOBus->Read(DataBus->Read(regs.PC++));
          // IO bus is an 8 bit address bus... and as such is decoded.
          // In reality, register A is also placed on the upper 8 bits
          // of the address bus... Something like this:
          regs.A = IOBus->Read(DataBus->Read(regs.PC++) | (regs.A << 8));
				  break;
				
				// call c,NN
			  case 0xDC:
				  if (regs.CF) {
  					tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
  					tStates += 10;
					  regs.PC += 2;
				  }
				  break;
				
			 // IX register operations
			  case 0xDD:
          regs.XX = regs.IX;
				  EmulateOneXX();
          regs.IX = regs.XX;
				  break;
				
				// sbc a,N
			  case 0xDE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            SBC_R8(regs.A,val);
          }
				  break;
				
				// rst 0x18
			  case 0xDF:
				  tStates += 11;
          Push16(regs.PC);
          regs.PC = 0x0018;
				  break;
				
				// ret po
			  case 0xE0:
				  tStates += 5;
				  if (!regs.PF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// pop hl
			  case 0xE1:
				  tStates += 10;
          regs.HL = Pop16();
				  break;
				
				// jp po,NN
			  case 0xE2:
				  tStates += 10;
				  if (!regs.PF)
            regs.PC = Read16(regs.PC);
          else
  					regs.PC += 2;
				  break;
				
				// ex (sp),hl
			  case 0xE3:
				  tStates += 19;
          {
            unsigned short val = Read16(regs.SP);
            Write16(regs.SP,regs.HL);
            regs.HL = val;
          }
				  break;
				
				// call po,NN
			  case 0xE4:
				  if (!regs.PF) {
					  tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
  					tStates += 10;
            regs.PC += 2;
				  }
				  break;
				
				// push hl
			  case 0xE5:
				  tStates += 11;
          Push16(regs.HL);
				  break;
				
				// and N
			  case 0xE6:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            AND_R8(val);
          }
				  break;
				
				// rst 0x20
			  case 0xE7:
  				tStates += 11;
          Push16(regs.PC);
				  regs.PC = 0x0020;
				  break;
				
				// ret pe
			  case 0xE8:
				  tStates += 5;
				  if (regs.PF) {
  					tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// jp (hl)
			  case 0xE9:
				  tStates += 4;
          regs.PC = regs.HL;
				  break;
				
				// jp pe,NN
			  case 0xEA:
				  tStates += 10;
				  if (regs.PF)
            regs.PC = Read16(regs.PC);
          else
            regs.PC+=2;
				  break;
				
				// ex de,hl
			  case 0xEB:
				  tStates += 4;
          {
            unsigned short aux = regs.DE;
            regs.DE = regs.HL;
            regs.HL = aux;
          }
				  break;
				
				// call pe,NN
			  case 0xEC:
				  if (regs.PF) {
					  tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
            regs.PC+=2;
				  }
				  break;
				
			  // 0xED instructions prefix
			  case 0xED:
          EmulateOneED();
				  break;
				
				// xor N
			  case 0xEE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            XOR_R8(val);
          }
				  break;
				
				// rst 0x28
			  case 0xEF:
				  tStates += 11;
          Push16(regs.PC);
				  regs.PC = 0x0028;
				  break;
				
				// ret p
			  case 0xF0:
				  tStates += 5;
				  if (!regs.SF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// pop af
			  case 0xF1:
				  tStates += 10;
          regs.AF = Pop16();
				  break;
				
				// jp p,NN
			  case 0xF2:
				  tStates += 10;
				  if (!regs.SF)
            regs.PC = Read16(regs.PC);
          else
            regs.PC += 2;
				  break;
				
        // di
        // REVISIT pedlop01
			  case 0xF3:
				  tStates += 4;
          regs.iff1A = 0;
          regs.iff1B = 0;
				  break;
				
				// call p,NN
			  case 0xF4:
				  if (!regs.SF) {
					  tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
            regs.PC+=2;
				  }
				  break;
				
				// push af
			  case 0xF5:
				  tStates += 11;
          Push16(regs.AF);
				  break;
				
				// or N
			  case 0xF6:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            OR_R8(val);
          }
				  break;
				
				// rst 0x30
			  case 0xF7:
				  tStates += 11;
          Push16(regs.PC);
          regs.PC = 0x0030;
				  break;
				
				// ret m
			  case 0xF8:
				  tStates += 5;
				  if (regs.SF) {
					  tStates += 6;
            regs.PC = Pop16();
				  }
				  break;
				
				// ld sp,hl
			  case 0xF9:
				  tStates += 6;
          regs.SP = regs.HL;
				  break;
				
				// jp m,NN
			  case 0xFA:
				  tStates += 10;
				  if (regs.SF)
            regs.PC = Read16(regs.PC);
          else
            regs.PC += 2;
				  break;
				
        // ei
        // REVISIT pedlop01
			  case 0xFB:
				  tStates += 4;
				  regs.iff1A = 1;
				  regs.iff1B = 1;
				  break;
				
				// call m,NN
			  case 0xFC:
				  if (regs.SF) {
					  tStates += 17;
            Push16(regs.PC+2);
            regs.PC = Read16(regs.PC);
				  } else {
					  tStates += 10;
            regs.PC += 2;
				  }
				  break;
				
			  // IY register operation prefix
			  case 0xFD:
          regs.XX = regs.IY;
          EmulateOneXX();
          regs.IY = regs.XX;
				  break;
				
				// cp N
			  case 0xFE:
				  tStates += 7;
          {
            unsigned char val = DataBus->Read(regs.PC++);
            CMP_R8(val);
          }
				  break;
				
				// rst 0x38
			  case 0xFF:
				  tStates += 11;
          Push16(regs.PC);
          regs.PC = 0x0038;
				  break;			

        default:
          break;
    }
}

void Z80::EmulateOnceCB() {
  // Fetch next instruction
  unsigned char op = DataBus->Read(regs.PC++);

  switch(op) {
	 	// rlc b
    case 0x00:
      tStates += 8;
      RLC8(regs.B);
      break;
       
    // rlc c
    case 0x01:
      tStates += 8;
      RLC8(regs.C);
      break;
       
    // rlc d
    case 0x02:
      tStates += 8;
      RLC8(regs.D);
      break;
       
    // rlc e
    case 0x03:
      tStates += 8;
      RLC8(regs.E);
      break;
       
    // rlc h
    case 0x04:
      tStates += 8;
      RLC8(regs.H);
      break;
       
    // rlc l
    case 0x05:
      tStates += 8;
      RLC8(regs.L);
      break;
       
    // rlc (hl)
    case 0x06:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        RLC8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // rlc a
    case 0x07:
      tStates += 8;
      RLC8(regs.A);
      break;
       
    // rrc b
    case 0x08:
      tStates += 8;
      RRC8(regs.B);
      break;
       
    // rrc c
    case 0x09:
      tStates += 8;
      RRC8(regs.C);
      break;
       
    // rrc d
    case 0x0A:
      tStates += 8;
      RRC8(regs.D);
      break;
       
    // rrc e
    case 0x0B:
      tStates += 8;
      RRC8(regs.E);
      break;
       
    // rrc h
    case 0x0C:
      tStates += 8;
      RRC8(regs.H);
      break;
       
    // rrc l
    case 0x0D:
      tStates += 8;
      RRC8(regs.L);
      break;
       
    // rrc (hl)
    case 0x0E:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        RRC8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // rrc a
    case 0x0F:
      tStates += 8;
      RRC8(regs.A);
      break;
      
    // rl b
    case 0x10:
      tStates += 8;
      RL8(regs.B);
      break;
      
    // rl c
    case 0x11:
      tStates += 8;
      RL8(regs.C);
      break;
      
    // rl d
    case 0x12:
      tStates += 8;
      RL8(regs.D);
      break;
       
    // rl e
    case 0x13:
      tStates += 8;
      RL8(regs.E);
      break;
       
       // rl h
    case 0x14:
      tStates += 8;
      RL8(regs.H);
      break;
       
    // rl l
    case 0x15:
      tStates += 8;
      RL8(regs.L);
      break;
       
    // rl (hl)
    case 0x16:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        RL8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // rl a
    case 0x17:
      tStates += 8;
      RL8(regs.A);
      break;
       
    // rr b
    case 0x18:
      tStates += 8;
      RR8(regs.B);
      break;
       
    // rr c
    case 0x19:
      tStates += 8;
      RR8(regs.C);
      break;
       
    // rr d
    case 0x1A:
      tStates += 8;
      RR8(regs.D);
      break;
       
    // rr e
    case 0x1B:
      tStates += 8;
      RR8(regs.E);
      break;
       
    // rr h
    case 0x1C:
      tStates += 8;
      RR8(regs.H);
      break;
       
    // rr l
    case 0x1D:
      tStates += 8;
      RR8(regs.L);
      break;
       
    // rr (hl)
    case 0x1E:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        RR8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // rr a
    case 0x1F:
      tStates += 8;
      RR8(regs.A);
      break;
      
    // sla b
    case 0x20:
      tStates += 8;
      SLA8(regs.B);
      break;
       
    // sla c
    case 0x21:
      tStates += 8;
      SLA8(regs.C);
      break;
       
    // sla d
    case 0x22:
      tStates += 8;
      SLA8(regs.D);
      break;
       
    // sla e
    case 0x23:
      tStates += 8;
      SLA8(regs.E);
      break;
       
    // sla h
    case 0x24:
      tStates += 8;
      SLA8(regs.H);
      break;
      
    // sla l
    case 0x25:
      tStates += 8;
      SLA8(regs.L);
      break;
       
    // sla (hl)
    case 0x26:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        SLA8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // sla a
    case 0x27:
      tStates += 8;
      SLA8(regs.A);
      break;
       
    // sra b
    case 0x28:
      tStates += 8;
      SRA8(regs.B);
      break;
       
    // sra c
    case 0x29:
      tStates += 8;
      SRA8(regs.C);
      break;
       
    // sra d
    case 0x2A:
      tStates += 8;
      SRA8(regs.D);
      break;
       
    // sra e
    case 0x2B:
      tStates += 8;
      SRA8(regs.E);
      break;
       
    // sra h
    case 0x2C:
      tStates += 8;
      SRA8(regs.H);
      break;
       
    // sra l
    case 0x2D:
      tStates += 8;
      SRA8(regs.L);
      break;
       
    // sra (hl)
    case 0x2E:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        SRA8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // sra a
    case 0x2F:
      tStates += 8;
      SRA8(regs.A);
      break;
       
      // Next instructions officially don't exist.
      // SLI rotates operand left and always insert a zero bit to the right
       
    // sli b
    case 0x30:
      tStates += 8;
      SLI8(regs.B);
      break;
       
    // sli c
    case 0x31:
      tStates += 8;
      SLI8(regs.C);
      break;
       
    // sli d
    case 0x32:
      tStates += 8;
      SLI8(regs.D);
      break;
       
    // sli e
    case 0x33:
      tStates += 8;
      SLI8(regs.E);
      break;
       
    // sli h
    case 0x34:
      tStates += 8;
      SLI8(regs.H);
      break;
       
    // sli l
    case 0x35:
      tStates += 8;
      SLI8(regs.L);
      break;
       
    // sli (hl)
    case 0x36:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        SLI8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // sli a
    case 0x37:
      tStates += 8;
      SLI8(regs.A);
      break;
       
    // srl b
    case 0x38:
      tStates += 8;
      SRL8(regs.B);
      break;
       
    // srl c
    case 0x39:
      tStates += 8;
      SRL8(regs.C);
      break;
       
    // srl d
    case 0x3A:
      tStates += 8;
      SRL8(regs.D);
      break;
       
    // srl e
    case 0x3B:
      tStates += 8;
      SRL8(regs.E);
      break;
       
    // srl h
    case 0x3C:
      tStates += 8;
      SRL8(regs.H);
      break;
       
    // srl l
    case 0x3D:
      tStates += 8;
      SRL8(regs.L);
      break;
       
    // srl (hl)
    case 0x3E:
      tStates += 15;
      {
        unsigned char val = DataBus->Read(regs.HL);
        SRL8(val);
        DataBus->Write(regs.HL,val);
      }
      break;
       
    // srl a
    case 0x3F:
      tStates += 8;
      SRL8(regs.A);
      break;
       
    // bit 0,b
    case 0x40:
      tStates += 8;
      BIT8(regs.B,0);
      break;
       
    // bit 0,c
    case 0x41:
      tStates += 8;
      BIT8(regs.C,0);
      break;
       
    // bit 0,d
    case 0x42:
      tStates += 8;
      BIT8(regs.D,0);
      break;
       
    // bit 0,e
    case 0x43:
      tStates += 8;
      BIT8(regs.E,0);
      break;
       
    // bit 0,h
    case 0x44:
      tStates += 8;
      BIT8(regs.H,0);
      break;
       
    // bit 0,l
    case 0x45:
      tStates += 8;
      BIT8(regs.L,0);
      break;
       
    // bit 0,(hl)
    case 0x46:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),0);
      break;
       
    // bit 0,a
    case 0x47:
      tStates += 8;
      BIT8(regs.A,0);
      break;
       
    // bit 1,b
    case 0x48:
      tStates += 8;
      BIT8(regs.B,1);
      break;
       
    // bit 1,c
    case 0x49:
      tStates += 8;
      BIT8(regs.C,1);
      break;
       
    // bit 1,d
    case 0x4A:
      tStates += 8;
      BIT8(regs.D,1);
      break;
       
    // bit 1,e
    case 0x4B:
      tStates += 8;
      BIT8(regs.E,1);
      break;
       
    // bit 1,h
    case 0x4C:
      tStates += 8;
      BIT8(regs.H,1);
      break;
       
    // bit 1,l
    case 0x4D:
      tStates += 8;
      BIT8(regs.L,1);
      break;
       
    // bit 1,(hl)
    case 0x4E:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),1);
      break;
       
    // bit 1,a
    case 0x4F:
      tStates += 8;
      BIT8(regs.A,1);
      break;
       
    // bit 2,b
    case 0x50:
      tStates += 8;
      BIT8(regs.B,2);
      break;
       
    // bit 2,c
    case 0x51:
      tStates += 8;
      BIT8(regs.C,2);
      break;
       
    // bit 2,d
    case 0x52:
      tStates += 8;
      BIT8(regs.D,2);
      break;
       
    // bit 2,e
    case 0x53:
      tStates += 8;
      BIT8(regs.E,2);
      break;
       
    // bit 2,h
    case 0x54:
      tStates += 8;
      BIT8(regs.H,2);
      break;
       
    // bit 2,l
    case 0x55:
      tStates += 8;
      BIT8(regs.L,2);
      break;
       
    // bit 2,(hl)
    case 0x56:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),2);
      break;
       
    // bit 2,a
    case 0x57:
      tStates += 8;
      BIT8(regs.A,2);
      break;
       
    // bit 3,b
    case 0x58:
      tStates += 8;
      BIT8(regs.B,3);
      break;
       
    // bit 3,c
    case 0x59:
      tStates += 8;
      BIT8(regs.C,3);
      break;
       
    // bit 3,d
    case 0x5A:
      tStates += 8;
      BIT8(regs.D,3);
      break;
       
    // bit 3,e
    case 0x5B:
      tStates += 8;
      BIT8(regs.E,3);
      break;
       
    // bit 3,h
    case 0x5C:
      tStates += 8;
      BIT8(regs.H,3);
      break;
       
    // bit 3,l
    case 0x5D:
      tStates += 8;
      BIT8(regs.L,3);
      break;
       
    // bit 3,(hl)
    case 0x5E:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),3);
      break;
       
    // bit 3,a
    case 0x5F:
      tStates += 8;
      BIT8(regs.A,3);
      break;
       
    // bit 4,b
    case 0x60:
      tStates += 8;
      BIT8(regs.B,4);
      break;
       
    // bit 4,c
    case 0x61:
      tStates += 8;
      BIT8(regs.C,4);
      break;
       
    // bit 4,d
    case 0x62:
      tStates += 8;
      BIT8(regs.D,4);
      break;
       
    // bit 4,e
    case 0x63:
      tStates += 8;
      BIT8(regs.E,4);
      break;
       
    // bit 4,h
    case 0x64:
      tStates += 8;
      BIT8(regs.H,4);
      break;
       
    // bit 4,l
    case 0x65:
      tStates += 8;
      BIT8(regs.L,4);
      break;
       
    // bit 4,(hl)
    case 0x66:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),4);
      break;
       
    // bit 4,a
    case 0x67:
      tStates += 8;
      BIT8(regs.A,4);
      break;
       
    // bit 5,b
    case 0x68:
      tStates += 8;
      BIT8(regs.B,5);
      break;
       
    // bit 5,c
    case 0x69:
      tStates += 8;
      BIT8(regs.C,5);
      break;
       
    // bit 5,d
    case 0x6A:
      tStates += 8;
      BIT8(regs.D,5);
      break;
       
    // bit 5,e
    case 0x6B:
      tStates += 8;
      BIT8(regs.E,5);
      break;
       
    // bit 5,h
    case 0x6C:
      tStates += 8;
      BIT8(regs.H,5);
      break;
       
    // bit 5,l
    case 0x6D:
      tStates += 8;
      BIT8(regs.L,5);
      break;
       
    // bit 5,(hl)
    case 0x6E:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),5);
      break;
       
    // bit 5,a
    case 0x6F:
      tStates += 8;
      BIT8(regs.A,5);
      break;
       
    // bit 6,b
    case 0x70:
      tStates += 8;
      BIT8(regs.B,6);
      break;
       
    // bit 6,c
    case 0x71:
      tStates += 8;
      BIT8(regs.C,6);
      break;
       
    // bit 6,d
    case 0x72:
      tStates += 8;
      BIT8(regs.D,6);
      break;
       
    // bit 6,e
    case 0x73:
      tStates += 8;
      BIT8(regs.E,6);
      break;
       
    // bit 6,h
    case 0x74:
      tStates += 8;
      BIT8(regs.H,6);
      break;
       
    // bit 6,l
    case 0x75:
      tStates += 8;
      BIT8(regs.L,6);
      break;
       
    // bit 6,(hl)
    case 0x76:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),6);
      break;
       
    // bit 6,a
    case 0x77:
      tStates += 8;
      BIT8(regs.A,6);
      break;
       
    // bit 7,b
    case 0x78:
      tStates += 8;
      BIT8(regs.B,7);
      break;
       
    // bit 7,c
    case 0x79:
      tStates += 8;
      BIT8(regs.C,7);
      break;
       
    // bit 7,d
    case 0x7A:
      tStates += 8;
      BIT8(regs.D,7);
      break;
       
    // bit 7,e
    case 0x7B:
      tStates += 8;
      BIT8(regs.E,7);
      break;
       
    // bit 7,h
    case 0x7C:
      tStates += 8;
      BIT8(regs.H,7);
      break;
       
    // bit 7,l
    case 0x7D:
      tStates += 8;
      BIT8(regs.L,7);
      break;
       
    // bit 7,(hl)
    case 0x7E:
      tStates += 12;
      BIT8(DataBus->Read(regs.HL),7);
      break;
       
    // bit 7,a
    case 0x7F:
      tStates += 8;
      BIT8(regs.A,7);
      break;
       
    // res 0,b
    case 0x80:
      tStates += 8;
      regs.B &= 0xfe;
      break;
       
    // res 0,c
    case 0x81:
      tStates += 8;
      regs.C &= 0xfe;
      break;
       
    // res 0,d
    case 0x82:
      tStates += 8;
      regs.D &= 0xfe;
      break;
       
    // res 0,e
    case 0x83:
      tStates += 8;
      regs.E &= 0xfe;
      break;
       
    // res 0,h
    case 0x84:
      tStates += 8;
      regs.H &= 0xfe;
      break;
       
    // res 0,l
    case 0x85:
      tStates += 8;
      regs.L &= 0xfe;
      break;
       
    // res 0,(hl)
    case 0x86:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xFE);
      break;
       
    // res 0,a
    case 0x87:
      tStates += 8;
      regs.A &= 0xfe;
      break;
       
    // res 1,b
    case 0x88:
      tStates += 8;
      regs.B &= 0xfd;
      break;
       
    // res 1,c
    case 0x89:
      tStates += 8;
      regs.C &= 0xfd;
      break;
       
    // res 1,d
    case 0x8A:
      tStates += 8;
      regs.D &= 0xfd;
      break;
       
    // res 1,e
    case 0x8B:
      tStates += 8;
      regs.E &= 0xfd;
      break;
       
    // res 1,h
    case 0x8C:
      tStates += 8;
      regs.H &= 0xfd;
      break;
       
    // res 1,l
    case 0x8D:
      tStates += 8;
      regs.L &= 0xfd;
      break;
       
    // res 1,(hl)
    case 0x8E:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xFD);
      break;
       
    // res 1,a
    case 0x8F:
      tStates += 8;
      regs.A &= 0xfd;
      break;
       
    // res 2,b
    case 0x90:
      tStates += 8;
      regs.B &= 0xfb;
      break;
       
    // res 2,c
    case 0x91:
      tStates += 8;
      regs.C &= 0xfb;
      break;
       
    // res 2,d
    case 0x92:
      tStates += 8;
      regs.D &= 0xfb;
      break;
       
    // res 2,e
    case 0x93:
      tStates += 8;
      regs.E &= 0xfb;
      break;
       
    // res 2,h
    case 0x94:
      tStates += 8;
      regs.H &= 0xfb;
      break;
       
    // res 2,l
    case 0x95:
      tStates += 8;
      regs.L &= 0xfb;
      break;
       
    // res 2,(hl)
    case 0x96:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xFB);
      break;
       
    // res 2,a
    case 0x97:
      tStates += 8;
      regs.A &= 0xfb;
      break;
       
    // res 3,b
    case 0x98:
      tStates += 8;
      regs.B &= 0xf7;
      break;
       
    // res 3,c
    case 0x99:
      tStates += 8;
      regs.C &= 0xf7;
      break;
       
    // res 3,d
    case 0x9A:
      tStates += 8;
      regs.D &= 0xf7;
      break;
       
    // res 3,e
    case 0x9B:
      tStates += 8;
      regs.E &= 0xf7;
      break;
       
    // res 3,h
    case 0x9C:
      tStates += 8;
      regs.H &= 0xf7;
      break;
       
    // res 3,l
    case 0x9D:
      tStates += 8;
      regs.L &= 0xf7;
      break;
       
    // res 3,(hl)
    case 0x9E:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xF7);
      break;
       
    // res 3,a
    case 0x9F:
      tStates += 8;
      regs.A &= 0xf7;
      break;
       
    // res 4,b
    case 0xA0:
      tStates += 8;
      regs.B &= 0xef;
      break;
       
    // res 4,c
    case 0xA1:
      tStates += 8;
      regs.C &= 0xef;
      break;
       
    // res 4,d
    case 0xA2:
      tStates += 8;
      regs.D &= 0xef;
      break;
       
    // res 4,e
    case 0xA3:
      tStates += 8;
      regs.E &= 0xef;
      break;
       
    // res 4,h
    case 0xA4:
      tStates += 8;
      regs.H &= 0xef;
      break;
       
    // res 4,l
    case 0xA5:
      tStates += 8;
      regs.L &= 0xef;
      break;
       
    // res 4,(hl)
    case 0xA6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xEF);
      break;
       
    // res 4,a
    case 0xA7:
      tStates += 8;
      regs.A &= 0xef;
      break;
       
    // res 5,b
    case 0xA8:
      tStates += 8;
      regs.B &= 0xdf;
      break;
       
    // res 5,c
    case 0xA9:
      tStates += 8;
      regs.C &= 0xdf;
      break;
       
    // res 5,d
    case 0xAA:
      tStates += 8;
      regs.D &= 0xdf;
      break;
       
    // res 5,e
    case 0xAB:
      tStates += 8;
      regs.E &= 0xdf;
      break;
       
    // res 5,h
    case 0xAC:
      tStates += 8;
      regs.H &= 0xdf;
      break;
       
    // res 5,l
    case 0xAD:
      tStates += 8;
      regs.L &= 0xdf;
      break;
       
    // res 5,(hl)
    case 0xAE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xDF);
      break;
       
    // res 5,a
    case 0xAF:
      tStates += 8;
      regs.A &= 0xdf;
      break;
       
    // res 6,b
    case 0xB0:
      tStates += 8;
      regs.B &= 0xbf;
      break;
       
    // res 6,c
    case 0xB1:
      tStates += 8;
      regs.C &= 0xbf;
      break;
       
    // res 6,d
    case 0xB2:
      tStates += 8;
      regs.D &= 0xbf;
      break;
       
    // res 6,e
    case 0xB3:
      tStates += 8;
      regs.E &= 0xbf;
      break;
       
    // res 6,h
    case 0xB4:
      tStates += 8;
      regs.H &= 0xbf;
      break;
       
    // res 6,l
    case 0xB5:
      tStates += 8;
      regs.L &= 0xbf;
      break;
       
    // res 6,(hl)
    case 0xB6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0xBF);
      break;
       
    // res 6,a
    case 0xB7:
      tStates += 8;
      regs.A &= 0xbf;
      break;
       
    // res 7,b
    case 0xB8:
      tStates += 8;
      regs.B &= 0x7f;
      break;
       
    // res 7,c
    case 0xB9:
      tStates += 8;
      regs.C &= 0x7f;
      break;
       
    // res 7,d
    case 0xBA:
      tStates += 8;
      regs.D &= 0x7f;
      break;
       
    // res 7,e
    case 0xBB:
      tStates += 8;
      regs.E &= 0x7f;
      break;
       
    // res 7,h
    case 0xBC:
      tStates += 8;
      regs.H &= 0x7f;
      break;
       
    // res 7,l
    case 0xBD:
      tStates += 8;
      regs.L &= 0x7f;
      break;
       
    // res 7,(hl)
    case 0xBE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) & 0x7F);
      break;
       
    // res 7,a
    case 0xBF:
      tStates += 8;
      regs.A &= 0x7f;
      break;
       
    // set 0,b
    case 0xC0:
      tStates += 8;
      regs.B |= 0x1;
      break;
       
    // set 0,c
    case 0xC1:
      tStates += 8;
      regs.C |= 0x1;
      break;
       
    // set 0,d
    case 0xC2:
      tStates += 8;
      regs.D |= 0x1;
      break;
       
    // set 0,e
    case 0xC3:
      tStates += 8;
      regs.E |= 0x1;
      break;
       
    // set 0,h
    case 0xC4:
      tStates += 8;
      regs.H |= 0x1;
      break;
       
    // set 0,l
    case 0xC5:
      tStates += 8;
      regs.L |= 0x1;
      break;
       
    // set 0,(hl)
    case 0xC6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x01);
      break;
       
    // set 0,a
    case 0xC7:
      tStates += 8;
      regs.A |= 0x1;
      break;
       
    // set 1,b
    case 0xC8:
      tStates += 8;
      regs.B |= 0x2;
      break;
       
    // set 1,c
    case 0xC9:
      tStates += 8;
      regs.C |= 0x2;
      break;
       
    // set 1,d
    case 0xCA:
      tStates += 8;
      regs.D |= 0x2;
      break;
       
    // set 1,e
    case 0xCB:
      tStates += 8;
      regs.E |= 0x2;
      break;
       
    // set 1,h
    case 0xCC:
      tStates += 8;
      regs.H |= 0x2;
      break;
       
    // set 1,l
    case 0xCD:
      tStates += 8;
      regs.L |= 0x2;
      break;
       
    // set 1,(hl)
    case 0xCE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x02);
      break;
       
    // set 1,a
    case 0xCF:
      tStates += 8;
      regs.A |= 0x2;
      break;
       
    // set 2,b
    case 0xD0:
      tStates += 8;
      regs.B |= 0x4;
      break;
       
    // set 2,c
    case 0xD1:
      tStates += 8;
      regs.C |= 0x4;
      break;
       
    // set 2,d
    case 0xD2:
      tStates += 8;
      regs.D |= 0x4;
      break;
       
    // set 2,e
    case 0xD3:
      tStates += 8;
      regs.E |= 0x4;
      break;
       
    // set 2,h
    case 0xD4:
      tStates += 8;
      regs.H |= 0x4;
      break;
       
    // set 2,l
    case 0xD5:
      tStates += 8;
      regs.L |= 0x4;
      break;
       
    // set 2,(hl)
    case 0xD6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x04);
      break;
       
    // set 2,a
    case 0xD7:
      tStates += 8;
      regs.A |= 0x4;
      break;
       
    // set 3,b
    case 0xD8:
      tStates += 8;
      regs.B |= 0x8;
      break;
       
    // set 3,c
    case 0xD9:
      tStates += 8;
      regs.C |= 0x8;
      break;
       
    // set 3,d
    case 0xDA:
      tStates += 8;
      regs.D |= 0x8;
      break;
       
    // set 3,e
    case 0xDB:
      tStates += 8;
      regs.E |= 0x8;
      break;
       
    // set 3,h
    case 0xDC:
      tStates += 8;
      regs.H |= 0x8;
      break;
       
    // set 3,l
    case 0xDD:
      tStates += 8;
      regs.L |= 0x8;
      break;
       
    // set 3,(hl)
    case 0xDE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x08);
      break;
       
    // set 3,a
    case 0xDF:
      tStates += 8;
      regs.A |= 0x8;
      break;
       
    // set 4,b
    case 0xE0:
      tStates += 8;
      regs.B |= 0x10;
      break;
       
    // set 4,c
    case 0xE1:
      tStates += 8;
      regs.C |= 0x10;
      break;
       
    // set 4,d
    case 0xE2:
      tStates += 8;
      regs.D |= 0x10;
      break;
       
    // set 4,e
    case 0xE3:
      tStates += 8;
      regs.E |= 0x10;
      break;
       
    // set 4,h
    case 0xE4:
      tStates += 8;
      regs.H |= 0x10;
      break;
       
    // set 4,l
    case 0xE5:
      tStates += 8;
      regs.L |= 0x10;
      break;
       
    // set 4,(hl)
    case 0xE6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x10);
      break;
      
    // set 4,a
    case 0xE7:
      tStates += 8;
      regs.A |= 0x10;
      break;
       
    // set 5,b
    case 0xE8:
      tStates += 8;
      regs.B |= 0x20;
      break;
       
    // set 5,c
    case 0xE9:
      tStates += 8;
      regs.C |= 0x20;
      break;
       
    // set 5,d
    case 0xEA:
      tStates += 8;
      regs.D |= 0x20;
      break;
       
    // set 5,e
    case 0xEB:
      tStates += 8;
      regs.E |= 0x20;
      break;
       
    // set 5,h
    case 0xEC:
      tStates += 8;
      regs.H |= 0x20;
      break;
       
    // set 5,l
    case 0xED:
      tStates += 8;
      regs.L |= 0x20;
      break;
       
    // set 5,(hl)
    case 0xEE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x20);
      break;
       
    // set 5,a
    case 0xEF:
      tStates += 8;
      regs.A |= 0x20;
      break;
       
    // set 6,b
    case 0xF0:
      tStates += 8;
      regs.B |= 0x40;
      break;
       
    // set 6,c
    case 0xF1:
      tStates += 8;
      regs.C |= 0x40;
      break;
       
    // set 6,d
    case 0xF2:
      tStates += 8;
      regs.D |= 0x40;
      break;
       
    // set 6,e
    case 0xF3:
      tStates += 8;
      regs.E |= 0x40;
      break;
       
    // set 6,h
    case 0xF4:
      tStates += 8;
      regs.H |= 0x40;
      break;
       
    // set 6,l
    case 0xF5:
      tStates += 8;
      regs.L |= 0x40;
      break;
       
    // set 6,(hl)
    case 0xF6:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x40);
      break;
       
    // set 6,a
    case 0xF7:
      tStates += 8;
      regs.A |= 0x40;
      break;
       
    // set 7,b
    case 0xF8:
      tStates += 8;
      regs.B |= 0x80;
      break;
       
    // set 7,c
    case 0xF9:
      tStates += 8;
      regs.C |= 0x80;
      break;
       
    // set 7,d
    case 0xFA:
      tStates += 8;
      regs.D |= 0x80;
      break;
       
    // set 7,e
    case 0xFB:
      tStates += 8;
      regs.E |= 0x80;
      break;
       
    // set 7,h
    case 0xFC:
      tStates += 8;
      regs.H |= 0x80;
      break;
       
    // set 7,l
    case 0xFD:
      tStates += 8;
      regs.L |= 0x80;
      break;
       
    // set 7,(hl)
    case 0xFE:
      tStates += 15;
      DataBus->Write(regs.HL,DataBus->Read(regs.HL) | 0x80);
      break;
       
    // set 7,a
    case 0xFF:
      tStates += 8;
      regs.A |= 0x80;
      break;			 

    default:
      break;
  }
}

void Z80::EmulateOneED() {
  // Fetch next instruction
  unsigned char op = DataBus->Read(regs.PC++);

  // Increment instruction counter register
  regs.R60++;

  switch(op) {
		// in b,(c)
		case 0x40:
      tStates += 12;
      IN8(regs.B,regs.BC);
      break;
    
    // out (c),b
    case 0x41:
      tStates += 12;
      IOBus->Write(regs.BC,regs.B);
      break;
    
    // sbc hl,bc
    case 0x42:
      tStates += 15;
      SBC_R16(regs.HL,regs.BC);
      break;
    
    // ld (NN),bc
    case 0x43:
      tStates += 20;
      Write16(Read16(regs.PC),regs.BC);
      regs.PC += 2;
      break;
    
    // neg
    case 0x44:
    case 0x4c:	// undocumented
    case 0x54:	// undocumented
    case 0x5c:	// undocumented
    case 0x64:	// undocumented
    case 0x6c:	// undocumented
    case 0x74:	// undocumented
    case 0x7c:	// undocumented
      tStates += 8;
      {
        unsigned char val = regs.A;
        regs.A = 0;
        SUB_R8(val);
      }
      break;
    
    // retn
    case 0x45:
    case 0x55:	// undocumented
    case 0x5d:	// undocumented
    case 0x65:	// undocumented
    case 0x6d:	// undocumented
    case 0x75:	// undocumented
    case 0x7d:	// undocumented
      tStates += 14;
      regs.PC = Pop16();
      regs.iff1A = regs.iff1B;
      break;
    
    // im 0
    case 0x46:
    case 0x4e:	// undocumented
    case 0x66:	// undocumented
    case 0x6e:	// undocumented
      tStates += 8;
      regs.IM = 0;
      break;
    
    // ld i,a
    case 0x47:
      tStates += 9;
      regs.I = regs.A;
      break;
    
    // in c,(c)
    case 0x48:
      tStates += 12;
      IN8(regs.C,regs.BC);
      break;
    
    // out (c),c
    case 0x49:
      tStates += 12;
      IOBus->Write(regs.BC,regs.C);
      break;
    
    // adc hl,bc
    case 0x4A:
      tStates += 15;
      ADC_R16(regs.HL,regs.BC);
      break;
    
    // ld bc,(NN)
    case 0x4B:
      tStates += 20;
      regs.BC = Read16(Read16(regs.PC));
      regs.PC += 2;
      break;
    
    // reti
    case 0x4D:
      tStates += 14;
      regs.PC = Pop16();
      break;
    
    // ld r,a
    case 0x4F:
      tStates += 9;
      regs.R = regs.A;
      break;
    
    // in d,(c)
    case 0x50:
      tStates += 12;
      IN8(regs.D,regs.BC);
      break;
    
    // out (c),d
    case 0x51:
      tStates += 12;
      IOBus->Write(regs.BC,regs.D);
      break;
    
    // sbc hl,de
    case 0x52:
      tStates += 15;
      SBC_R16(regs.HL,regs.DE);
      break;
    
    // ld (NN),de
    case 0x53:
      tStates += 20;
      Write16(Read16(regs.PC),regs.DE);
      regs.PC += 2;
      break;

    // im 1
    case 0x56:
    case 0x76:	// undocumented
      tStates += 8;
      regs.IM = 1;
      break;
    
    // ld a,i
    case 0x57:
      tStates += 9;
      regs.A = regs.I;
      regs.SF = (regs.A & 0x80) != 0;
      regs.ZF = (regs.A == 0);
      regs.HF = regs.NF = 0;
      regs.PF = (regs.iff1B != 0);
      break;
    
    // in e,(c)
    case 0x58:
      tStates += 12;
      IN8(regs.E,regs.BC);
      break;
    
    // out (c),e
    case 0x59:
      tStates += 12;
      IOBus->Write(regs.BC,regs.E);
      break;
    
    // adc hl,de
    case 0x5A:
      tStates += 15;
      ADC_R16(regs.HL,regs.DE);
      break;
    
    // ld de,(NN)
    case 0x5B:
      tStates += 20;
      regs.DE = Read16(Read16(regs.PC));
      regs.PC += 2;
      break;
    
    // im 2
    case 0x5E:
    case 0x7e:	// undocumented
      tStates += 8;
      regs.IM = 2;
      break;
    
    // ld a,r
    case 0x5F:
      tStates += 9;
      regs.A = regs.R;
      regs.SF = (regs.A & 0x80) != 0;
      regs.ZF = (regs.A == 0);
      regs.HF = regs.NF = 0;
      regs.PF = (regs.iff1B != 0);
      break;
    
    // in h,(c)
    case 0x60:
      tStates += 12;
      IN8(regs.H,regs.BC);
      break;
    
    // out (c),h
    case 0x61:
      tStates += 12;
      IOBus->Write(regs.BC,regs.H);
      break;
    
    // sbc hl,hl
    case 0x62:
      tStates += 15;
      SBC_R16(regs.HL,regs.HL);
      break;
    
    // ld (nn),hl
    case 0x63:	// undocumented
      tStates += 20;
      Write16(Read16(regs.PC),regs.HL);
      regs.PC += 2;
      break;
    
    // rrd
    case 0x67:
      tStates += 18;
      {
        unsigned char val = DataBus->Read(regs.HL);
        DataBus->Write(regs.HL,(val >> 4) | (regs.A << 4));
        regs.A &= 0xF0;
        regs.A |= (val & 0x0F);
        regs.SF = ((regs.A & 0x80) != 0);
        regs.ZF = (regs.A == 0);
        regs.HF = regs.NF = 0;
        regs.PF = parityTable[regs.A];
      }
      break;
    
    // in l,(c)
    case 0x68:
      tStates += 12;
      IN8(regs.L,regs.BC);
      break;
    
    // out (c),l
    case 0x69:
      tStates += 12;
      IOBus->Write(regs.BC,regs.L);
      break;
    
    // adc hl,hl
    case 0x6A:
      tStates += 15;
      ADC_R16(regs.HL,regs.HL);
      break;
    
    // ld hl,(NN)
    case 0x6B:	// undocumented
      tStates += 20;
      regs.HL = Read16(Read16(regs.PC));
      regs.PC += 2;
      break;
    
    // rld
    case 0x6F:
      tStates += 18;
      {
        unsigned char aux = DataBus->Read(regs.HL);
        DataBus->Write(regs.HL,(aux << 4) | (regs.A & 0x0F));
        regs.A &= 0xF0;
        regs.A |= (aux >> 4);
        regs.SF = ((regs.A & 0x80) != 0);
        regs.ZF = (regs.A == 0);
        regs.HF = regs.NF = 0;
        regs.PF = parityTable[regs.A];
      }
      break;
    
    // This instruction is not documented. It reads the port
    // but does not use the result for anything but to set
    // flags.
    
    // in f,(c)
    case 0x70:
      tStates += 12;
      {
        //IOBus->Read(regs.BC);
        unsigned char val;
        IN8(val,regs.BC);
      }
      break;
    
    // Another undocumented opcode: writes a zero to the
    // (BC) IO port.

    // out (c),0
    case 0x71:
      tStates += 12;
      IOBus->Write(regs.BC,0);
      break;
    
    // sbc hl,sp
    case 0x72:
      tStates += 15;
      SBC_R16(regs.HL,regs.SP);
      break;
    
    // ld (NN),sp
    case 0x73:
      tStates += 20;
      Write16(Read16(regs.PC),regs.SP);
      regs.PC += 2;
      break;
    
    // undocumented nop's
    case 0x77:
    case 0x7f:
      tStates += 8;
      break;
    
    // in a,(c)
    case 0x78:
      tStates += 12;
      IN8(regs.A,regs.BC);
      break;
    
    // out (c),a
    case 0x79:
      tStates += 12;
      IOBus->Write(regs.BC,regs.A);
      break;
    
    // adc hl,sp
    case 0x7A:
      tStates += 15;
      ADC_R16(regs.HL,regs.SP);
      break;
    
    // ld sp,(NN)
    case 0x7B:
      tStates += 20;
      regs.SP = Read16(Read16(regs.PC));
      regs.PC += 2;
      break;
    
    // ldi
    case 0xA0:
      tStates += 16;
      {
        unsigned char val = DataBus->Read(regs.HL++);
        DataBus->Write(regs.DE++,val);
        regs.BC--;
        regs.PF = (regs.BC != 0);
        regs.NF = regs.HF = 0;
      }
      break;
    
    // cpi
    case 0xA1:
      tStates += 16;
      {
        unsigned char val = DataBus->Read(regs.HL++);
        unsigned short res = regs.A - val;
        unsigned char idx = ((regs.A & 0x08) >> 3) | ((val & 0x08) >> 2) | ((res & 0x08) >> 3);
        regs.BC--;
        regs.SF = ((res & 0x80) != 0);
        regs.ZF = (val == 0);
        regs.HF = subhalfcarryTable[idx & 0x07];
        regs.NF = 1;
        regs.PF = (regs.BC != 0);
      }
      break;
    
    // ini
    case 0xA2:
      tStates += 16;
      Write16(regs.HL++,IOBus->Read(regs.BC));
      regs.B--;
      regs.ZF = (regs.B == 0);
      regs.NF = true;
      break;
    
    // outi
    case 0xA3:
      tStates += 16;
      regs.B--;
      IOBus->Write(regs.BC,DataBus->Read(regs.HL++));
      regs.ZF = (regs.B == 0);
      regs.NF = true;
      break;
    
    // ldd
    case 0xA8:
      tStates += 16;
      DataBus->Write(regs.DE--,DataBus->Read(regs.HL--));
      regs.BC--;
      regs.PF = (regs.BC != 0);
      regs.HF = regs.NF = 0;
      break;
    
    // cpd
    case 0xA9:
      tStates += 16;
      {
        unsigned char val = DataBus->Read(regs.HL);
        unsigned short res = regs.A - val;
        unsigned char idx = ((regs.A & 0x88) >> 1) | ((val & 0x88) >> 2) | ((res & 0x88) >> 1);
        regs.SF = ((val & 0x80) != 0);
        regs.ZF = (val == 0);
        regs.HF = subhalfcarryTable[idx & 0x07];
        regs.NF = 1;
        regs.HL--;
        regs.BC--;
        regs.PF = (regs.BC != 0);
      }
      break;
    
    // ind
    case 0xAA:
      tStates += 16;
      DataBus->Write(regs.HL--,IOBus->Read(regs.BC));
      regs.B--;
      regs.ZF = (regs.B != 0);
      regs.NF = 1;
      break;

    // outd
    case 0xAB:
      tStates += 16;
      regs.B--;
      IOBus->Write(regs.BC,DataBus->Read(regs.HL--));
      regs.ZF = (regs.B != 0);
      regs.NF = 1;
      break;
    
    // ldir
    case 0xB0:
      tStates += 16;
      DataBus->Write(regs.DE,DataBus->Read(regs.HL));
      regs.DE++;
      regs.HL++;
      regs.BC--;
      regs.PF = (regs.BC != 0);
      regs.HF = regs.NF = 0;
      if (regs.BC) {
        tStates += 5;
        regs.PC -= 2;   // Repeat instruction.
      }
      break;
    
    // cpir
    case 0xB1:
      tStates += 16;
      {
        unsigned char val = DataBus->Read(regs.HL++);
        unsigned short res = regs.A - val;
        unsigned char idx = ((regs.A & 0x88) >> 3) | ((val & 0x88) >> 2) | ((res & 0x88) >> 1);
        regs.BC--;
        regs.SF = ((val & 0x80) != 0);
        regs.ZF = (res == 0);
        regs.HF = subhalfcarryTable[idx & 0x07];
        regs.NF = 1;
        regs.PF = (regs.BC != 0);
        if (regs.PF && !regs.ZF) {
          tStates += 5;
          regs.PC -= 2;   // repeat instruction.
        }
      }
      break;
    
    // inir
    case 0xB2:
      tStates += 16;
      DataBus->Write(regs.HL++,IOBus->Read(regs.BC));
      regs.B--;
      regs.ZF = (regs.B == 0);
      regs.NF = 1;
      if (!regs.ZF) {
        tStates += 5;
        regs.PC -= 2;   // repeat instruction.
      }
      break;
    
    // otir
    case 0xB3:
      tStates += 16;
      regs.B--;
      IOBus->Write(regs.BC,DataBus->Read(regs.HL++));
      regs.ZF = (regs.B == 0);
      regs.NF = 1;
      if (!regs.ZF) {
        tStates += 5;
        regs.PC -= 2;   // repeat instruction.
      }
      break;

    // lddr
    case 0xB8:
      tStates += 16;
      DataBus->Write(regs.DE--,DataBus->Read(regs.HL--));
      regs.BC--;
      regs.PF = (regs.BC != 0);
      regs.HF = regs.NF = 0;
      if (regs.BC) {
        tStates += 5;
        regs.PC -= 2;   // repeat instruction.
      }
      break;
    
    // cpdr
    case 0xB9:
      tStates += 16;
      {
        unsigned char val = DataBus->Read(regs.HL--);
        CMP_R8_NOFLAGS(regs.A,val);
        regs.BC--;
        regs.PF = (regs.BC != 0);
        if (regs.PF && !regs.ZF) {
          tStates += 5;
          regs.PC -= 2;   // repeat instruction.
        }
      }
      break;
    
    // indr
    case 0xBA:
      tStates += 16;
      DataBus->Write(regs.HL--,IOBus->Read(regs.BC));
      regs.B--;
      regs.ZF = (regs.B == 0);
      regs.NF = 1;
      if (!regs.ZF) {
        tStates += 5;
        regs.PC -= 2;   // repeat instruction.
      }
      break;
    
    // otdr
    case 0xBB:
      tStates += 16;
      regs.B--;
      IOBus->Write(regs.BC,DataBus->Read(regs.HL--));
      regs.ZF = (regs.B == 0);
      regs.NF = 1;
      if (!regs.ZF) {
        tStates += 5;
        regs.PC -= 2;   // repeat instruction.
      }
      break;
    
    case 0xFB:
      tStates += 8;
      break;
    
    case 0xFC:
      tStates += 8;
      break;
    
    case 0xFD:
      tStates += 8;
      break;
  
    default:
      tStates += 8;
      break;
  }
}

// Emulates a single DD or FD prefixed opcode.
// This function uses a "phantom" register XX, previously
// set up by the "EmulateOne()" function, copying the
// contents of IX or IY.
void Z80::EmulateOneXX() {
  // Fetch next instruction
  unsigned char op = DataBus->Read(regs.PC++);

  // Increment instruction counter register
  regs.R60++;

  switch(op) {
    // add xx,bc
    case 0x09:
      tStates += 15;
      ADD_R16(regs.XX,regs.BC);
      break;
      
    // add xx,de
    case 0x19:
      tStates += 15;
      ADD_R16(regs.XX,regs.DE);
      break;
      
    // ld xx,NN
    case 0x21:
      tStates += 14;
      regs.XX = Read16(regs.PC);
      regs.PC += 2;
      break;
      
    // ld (NN),xx
    case 0x22:
      tStates += 20;
      Write16(Read16(regs.PC),regs.XX);
      regs.PC += 2;
      break;
      
    // inc xx
    case 0x23:
      tStates += 10;
      regs.XX++;
      break;
      
    // inc hx
    case 0x24:
      tStates += 8;
      regs.HX++;
      break;
      
    // dec hx
    case 0x25:
      tStates += 8;
      regs.HX--;
      break;
      
    // ld hx,N
    case 0x26:
      tStates += 11;
      regs.HX = DataBus->Read(regs.PC++);
      break;
      
    // add xx,xx
    case 0x29:
      tStates += 15;
      ADD_R16(regs.XX,regs.XX);
      break;
      
    // ld xx,(NN)
    case 0x2A:
      tStates += 20;
      regs.XX = Read16(Read16(regs.PC));
      regs.PC += 2;
      break;
      
    // dec xx
    case 0x2B:
      tStates += 10;
      regs.XX--;
      break;
      
    // inc lx
    case 0x2C:
      tStates += 8;
      regs.LX++;
      break;
      
    // dec lx
    case 0x2D:
      tStates += 8;
      regs.LX--;
      break;
      
    // ld lx,N
    case 0x2E:
      tStates += 11;
      regs.LX = DataBus->Read(regs.PC++);
      break;
      
    // inc (xx+d)
    case 0x34:
      tStates += 23;
      {
        unsigned short addr = regs.XX + (char)DataBus->Read(regs.PC++);
        unsigned char val = DataBus->Read(addr);
        INC_R8(val);
        DataBus->Write(addr,val);
      }
      break;
      
    // dec (xx+d)
    case 0x35:
      tStates += 23;
      {
        unsigned short addr = regs.XX + (char)DataBus->Read(regs.PC++);
        unsigned char val = DataBus->Read(addr);
        DEC_R8(val);
        DataBus->Write(addr,val);
      }
      break;

    // ld (xx+d),N
    case 0x36:
      tStates += 19;
      DataBus->Write(regs.XX+(char)DataBus->Read(regs.PC),DataBus->Read(regs.PC+1));
      regs.PC += 2;
      break;
      
    // add xx,sp
    case 0x39:
      tStates += 15;
      ADD_R16(regs.XX,regs.SP);
      break;
      
    // ld b,hx
    case 0x44:
      tStates += 8;
      regs.B = regs.HX;
      break;
      
    // ld b,lx
    case 0x45:
      tStates += 8;
      regs.B = regs.LX;
      break;
      
    // ld b,(xx+d)
    case 0x46:
      tStates += 15;
      regs.B = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld c,hx
    case 0x4C:
      tStates += 8;
      regs.C = regs.HX;
      break;
      
    // ld c,lx
    case 0x4D:
      tStates += 8;
      regs.C = regs.LX;
      break;
      
    // ld c,(xx+d)
    case 0x4E:
      tStates += 15;
      regs.C = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld d,hx
    case 0x54:
      tStates += 8;
      regs.D = regs.HX;
      break;
      
    // ld d,lx
    case 0x55:
      tStates += 8;
      regs.D = regs.LX;
      break;
      
    // ld d,(xx+d)
    case 0x56:
      tStates += 19;
      regs.D = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld e,hx
    case 0x5C:
      tStates += 8;
      regs.E = regs.HX;
      break;
      
    // ld e,lx
    case 0x5D:
      tStates += 8;
      regs.E = regs.LX;
      break;
      
    // ld e,(xx+d)
    case 0x5E:
      tStates += 19;
      regs.E = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld hx,b
    case 0x60:
      tStates += 8;
      regs.HX = regs.B;
      break;
      
    // ld hx,c
    case 0x61:
      tStates += 8;
      regs.HX = regs.C;
      break;
      
    // ld hx,d
    case 0x62:
      tStates += 8;
      regs.HX = regs.D;
      break;
      
    // ld hx,e
    case 0x63:
      tStates += 8;
      regs.HX = regs.E;
      break;
      
    // ld hx,hx
    case 0x64:
      tStates += 8;
      break;
      
    // ld hx,lx
    case 0x65:
      tStates += 8;
      regs.HX = regs.LX;
      break;
      
    // ld h,(xx+d)
    case 0x66:
      tStates += 19;
      regs.H = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld hx,a
    case 0x67:
      tStates += 8;
      regs.HX = regs.A;
      break;
      
    // ld lx,b
    case 0x68:
      tStates += 8;
      regs.LX = regs.B;
      break;
      
    // ld lx,c
    case 0x69:
      tStates += 8;
      regs.LX = regs.C;
      break;
      
    // ld lx,d
    case 0x6A:
      tStates += 8;
      regs.LX = regs.D;
      break;
      
    // ld lx,e
    case 0x6B:
      tStates += 8;
      regs.LX = regs.E;
      break;
      
    // ld lx,hx
    case 0x6C:
      tStates += 8;
      regs.LX = regs.HX;
      break;
      
    // ld lx,lx
    case 0x6D:
      tStates += 8;
      break;
      
    // ld l,(xx+d)
    case 0x6E:
      tStates += 19;
      regs.L = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
      break;
      
    // ld lx,a
    case 0x6F:
      tStates += 8;
      regs.LX = regs.A;
      break;
      
    // ld (xx+d),b
    case 0x70:
      tStates += 19;
      DataBus->Write(regs.XX+(char)DataBus->Read(regs.PC++),regs.B);
      break;
      
    // ld (xx+d),c
    case 0x71:
      tStates += 19;
      DataBus->Write(regs.XX+(char)DataBus->Read(regs.PC++),regs.C);
      break;
      
    // ld (xx+d),d
    case 0x72:
      tStates += 19;
      DataBus->Write(regs.XX + (char)DataBus->Read(regs.PC++),regs.D);
      break;
      
    // ld (xx+d),e
    case 0x73:
      tStates += 19;
      DataBus->Write(regs.XX + (char)DataBus->Read(regs.PC++),regs.E);
      break;
      
    // ld (xx+d),h
    case 0x74:
      tStates += 19;
      DataBus->Write(regs.XX + (char)DataBus->Read(regs.PC++),regs.H);
      break;
      
    // ld (xx+d),l
    case 0x75:
      tStates += 19;
      DataBus->Write(regs.XX + (char)DataBus->Read(regs.PC++),regs.L);
      break;
      
    // ld (xx+d),a
    case 0x77:
      tStates += 19;
      DataBus->Write(regs.XX + (char)DataBus->Read(regs.PC++),regs.A);
      break;
      
    // ld a,hx
    case 0x7C:
      tStates += 8;
      regs.A = regs.HX;
      break;
      
    // ld a,lx
    case 0x7D:
      tStates += 8;
      regs.A = regs.LX;
      break;
      
    // ld a,(xx+d)
    case 0x7E:
      tStates += 19;
      regs.A = DataBus->Read(regs.XX + (char)DataBus->Read(regs.PC++));
      break;
      
    // add a,hx
    case 0x84:
      tStates += 8;
      ADD_R8(regs.HX);
      break;
      
    // add a,lx
    case 0x85:
      tStates += 8;
      ADD_R8(regs.LX);
      break;
      
    // add a,(xx+d)
    case 0x86:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        ADD_R8(val);
      }
      break;
      
    // adc a,hx
    case 0x8C:
      tStates += 8;
      ADC_R8(regs.HX);
      break;
      
    // adc a,lx
    case 0x8D:
      tStates += 8;
      ADC_R8(regs.LX);
      break;
      
    // adc a,(xx+d)
    case 0x8E:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        ADC_R8(val);
      }
      break;
      
    // sub hx
    case 0x94:
      tStates += 8;
      SUB_R8(regs.HX);
      break;
      
    // sub lx
    case 0x95:
      tStates += 8;
      SUB_R8(regs.LX);
      break;
      
    // sub (xx+d)
    case 0x96:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        SUB_R8(val);
      }
      break;
      
    // sbc a,hx
    case 0x9C:
      tStates += 8;
      SBC_R8(regs.A,regs.HX);
      break;
      
    // sbc a,l
    case 0x9D:
      tStates += 8;
      SBC_R8(regs.A,regs.L);
      break;
      
    // sbc a,(xx+d)
    case 0x9E:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        SBC_R8(regs.A,val);
      }
      break;
      
    // and hx
    case 0xA4:
      tStates += 8;
      AND_R8(regs.HX);
      break;
      
    // and lx
    case 0xA5:
      tStates += 8;
      AND_R8(regs.LX);
      break;
      
    // and (xx+d)
    case 0xA6:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        AND_R8(val);
      }
      break;
      
    // xor hx
    case 0xAC:
      tStates += 8;
      XOR_R8(regs.HX);
      break;
      
    // xor lx
    case 0xAD:
      tStates += 8;
      XOR_R8(regs.LX);
      break;
      
    // xor (xx+d)
    case 0xAE:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        XOR_R8(val);
      }
      break;
      
    // or hx
    case 0xB4:
      tStates += 8;
      OR_R8(regs.HX);
      break;
      
    // or lx
    case 0xB5:
      tStates += 8;
      OR_R8(regs.LX);
      break;
      
    // or (xx+d)
    case 0xB6:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        OR_R8(val);
      }
      break;
      
    // cp hx
    case 0xBC:
      tStates += 8;
      CMP_R8(regs.HX);
      break;
      
    // cp lx
    case 0xBD:
      tStates += 8;
      CMP_R8(regs.LX);
      break;
      
    // cp (xx+d)
    case 0xBE:
      tStates += 19;
      {
        unsigned char val = DataBus->Read(regs.XX+(char)DataBus->Read(regs.PC++));
        CMP_R8(val);
      }
      break;
      
    // Index register with 0xCB handling
    case 0xCB:
      EmulateOneXXCB();
      break;
    
    // pop xx
    case 0xE1:
      tStates += 14;
      regs.XX = Pop16();
      break;
      
    // ex (sp),xx
    case 0xE3:
      tStates += 23;
      {
        unsigned short val = Read16(regs.SP);
        Write16(regs.SP,regs.XX);
        regs.XX = val;
      }
      break;
      
    // push XX
    case 0xE5:
      tStates += 15;
      Push16(regs.XX);
      break;
      
    // jp (xx)
    case 0xE9:
      tStates += 8;
      regs.PC = regs.XX;
      break;
      
    // ld sp,xx
    case 0xF9:
      tStates += 10;
      regs.SP = regs.XX;
      break;
      
    case 0xDD:
    case 0xFD:
      tStates += 4;
      regs.PC--;  // Back to the second DD/FD
      break;
      
    case 0xEB:
    case 0xED:
      tStates += 8;
      break;
      
    // Unimplemented opcodes do the same as the unprefixed versions. Roll
    // back PC register and re-evaluate.
    default:
      tStates += 4;
      regs.PC--;  // Back to the nonprefixed opcode
      break;
  }
}

void Z80::EmulateOneXXCB() {
  // Fetch offset
	char offset = DataBus->Read(regs.PC++);
  unsigned short xxd = regs.XX + offset;

  // Fetch next instruction
  unsigned char op = DataBus->Read(regs.PC++);

  switch(op) {
		// rlc (xx+d),b
		case 0x00:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      RLC8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// rlc (xx+d),c
		case 0x01:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      RLC8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// rlc (xx+d),d
		case 0x02:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      RLC8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// rlc (xx+d),e
		case 0x03:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      RLC8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// rlc (xx+d),h
		case 0x04:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      RLC8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// rlc (xx+d),l
		case 0x05:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      RLC8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// rlc (xx+d)
		case 0x06:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        RLC8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// rlc (xx+d),a
		case 0x07:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      RLC8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// rrc (xx+d),b
		case 0x08:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      RRC8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// rrc (xx+d),c
		case 0x09:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      RRC8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// rrc (xx+d),d
		case 0x0A:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      RRC8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// rrc (xx+d),e
		case 0x0B:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      RRC8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// rrc (xx+d),h
		case 0x0C:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      RRC8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// rrc (xx+d),l
		case 0x0D:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      RRC8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// rrc (xx+d)
		case 0x0E:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        RRC8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// rrc a
		case 0x0F:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      RRC8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// rl (xx+d),b
		case 0x10:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      RL8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// rl (xx+d),c
		case 0x11:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      RL8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// rl (xx+d),d
		case 0x12:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      RL8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// rl (xx+d),e
		case 0x13:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      RL8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// rl (xx+d),h
		case 0x14:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      RL8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// rl (xx+d),l
		case 0x15:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      RL8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// rl (xx+d)
		case 0x16:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        RL8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// rl (xx+d),a
		case 0x17:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      RL8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// rr (xx+d),b
		case 0x18:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      RR8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// rr (xx+d),c
		case 0x19:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      RR8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// rr (xx+d),d
		case 0x1A:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      RR8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// rr (xx+d),e
		case 0x1B:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      RR8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// rr (xx+d),h
		case 0x1C:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      RR8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// rr (xx+d),l
		case 0x1D:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      RR8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// rr (xx+d)
		case 0x1E:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        RR8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// rr (xx+d),a
		case 0x1F:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      RR8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// sla (xx+d),b
		case 0x20:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      SLA8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// sla (xx+d),c
		case 0x21:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      SLA8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// sla (xx+d),d
		case 0x22:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      SLA8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// sla (xx+d),e
		case 0x23:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      SLA8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// sla (xx+d),h
		case 0x24:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      SLA8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// sla (xx+d),l
		case 0x25:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      SLA8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// sla (xx+d)
		case 0x26:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        SLA8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// sla (xx+d),a
		case 0x27:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      SLA8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// sra (xx+d),b
		case 0x28:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      SRA8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// sra (xx+d),c
		case 0x29:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      SRA8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// sra (xx+d),d
		case 0x2A:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      SRA8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// sra (xx+d),e
		case 0x2B:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      SRA8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// sra (xx+d),h
		case 0x2C:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      SRA8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// sra (xx+d),l
		case 0x2D:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      SRA8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// sra (xx+d)
		case 0x2E:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        SRA8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// sra (xx+d),a
		case 0x2F:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      SRA8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;

		// More undocumented SLI instruction variants.
				
		// sli (xx+d),b
		case 0x30:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      SLI8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// sli (xx+d),c
		case 0x31:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      SLI8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// sli (xx+d),d
		case 0x32:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      SLI8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// sli (xx+d),e
		case 0x33:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      SLI8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// sli (xx+d),h
		case 0x34:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      SLI8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// sli (xx+d),l
		case 0x35:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      SLI8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// sli (xx+d)
		case 0x36:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        SLI8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// sli (xx+d),a
		case 0x37:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      SLI8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// srl (xx+d),b
		case 0x38:
			tStates += 23;
      regs.B = DataBus->Read(xxd);
      SRL8(regs.B);
      DataBus->Write(xxd,regs.B);
			break;
				
		// srl (xx+d),c
		case 0x39:
			tStates += 23;
      regs.C = DataBus->Read(xxd);
      SRL8(regs.C);
      DataBus->Write(xxd,regs.C);
			break;
				
		// srl (xx+d),d
		case 0x3A:
			tStates += 23;
      regs.D = DataBus->Read(xxd);
      SRL8(regs.D);
      DataBus->Write(xxd,regs.D);
			break;
				
		// srl (xx+d),e
		case 0x3B:
			tStates += 23;
      regs.E = DataBus->Read(xxd);
      SRL8(regs.E);
      DataBus->Write(xxd,regs.E);
			break;
				
		// srl (xx+d),h
		case 0x3C:
			tStates += 23;
      regs.H = DataBus->Read(xxd);
      SRL8(regs.H);
      DataBus->Write(xxd,regs.H);
			break;
				
		// srl (xx+d),l
		case 0x3D:
			tStates += 23;
      regs.L = DataBus->Read(xxd);
      SRL8(regs.L);
      DataBus->Write(xxd,regs.L);
			break;
				
		// srl (xx+d)
		case 0x3E:
			tStates += 23;
      {
        unsigned char val = DataBus->Read(xxd);
        SRL8(val);
        DataBus->Write(xxd,val);
      }
			break;
				
		// srl (xx+d),a
		case 0x3F:
			tStates += 23;
      regs.A = DataBus->Read(xxd);
      SRL8(regs.A);
      DataBus->Write(xxd,regs.A);
			break;
				
		// bit 0,(ix+d)
		case 0x40:	// undocumented
		case 0x41:	// undocumented
		case 0x42:	// undocumented
		case 0x43:	// undocumented
		case 0x44:	// undocumented
		case 0x45:	// undocumented
		case 0x46:
		case 0x47:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),0);
			break;
				
		// bit 1,(xx+d)
		case 0x48:	// undocumented
		case 0x49:	// undocumented
		case 0x4A:	// undocumented
		case 0x4B:	// undocumented
		case 0x4C:	// undocumented
		case 0x4D:	// undocumented
		case 0x4E:
		case 0x4F:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),1);
			break;
				
		// bit 2,(xx+d)
		case 0x50:	// undocumented
		case 0x51:	// undocumented
		case 0x52:	// undocumented
		case 0x53:	// undocumented
		case 0x54:	// undocumented
		case 0x55:	// undocumented
		case 0x56:
		case 0x57:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),2);
			break;
				
		// bit 3,(xx+d)
		case 0x58:	// undocumented
		case 0x59:	// undocumented
		case 0x5A:	// undocumented
		case 0x5B:	// undocumented
		case 0x5C:	// undocumented
		case 0x5D:	// undocumented
		case 0x5E:
		case 0x5F:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),3);
			break;
				
		// bit 4,(xx+d)
		case 0x60:	// undocumented
		case 0x61:	// undocumented
		case 0x62:	// undocumented
		case 0x63:	// undocumented
		case 0x64:	// undocumented
		case 0x65:	// undocumented
		case 0x66:
		case 0x67:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),4);
			break;
				
		// bit 5,(xx+d)
		case 0x68:	// undocumented
		case 0x69:	// undocumented
		case 0x6A:	// undocumented
		case 0x6B:	// undocumented
		case 0x6C:	// undocumented
		case 0x6D:	// undocumented
		case 0x6E:
		case 0x6F:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),5);
			break;
				
		// bit 6,(xx+d)
		case 0x70:	// undocumented
		case 0x71:	// undocumented
		case 0x72:	// undocumented
		case 0x73:	// undocumented
		case 0x74:	// undocumented
		case 0x75:	// undocumented
		case 0x76:
		case 0x77:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),6);
			break;
				
		// bit 7,(xx+d)
		case 0x78:	// undocumented
		case 0x79:	// undocumented
		case 0x7A:	// undocumented
		case 0x7B:	// undocumented
		case 0x7C:	// undocumented
		case 0x7D:	// undocumented
		case 0x7E:
		case 0x7F:	// undocumented
			tStates += 20;
      BIT8(DataBus->Read(xxd),7);
			break;
				
		// res 0,(xx+d),b
		case 0x80:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 0,(xx+d),c
		case 0x81:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 0,(xx+d),d
		case 0x82:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 0,(xx+d),e
		case 0x83:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 0,(xx+d),h
		case 0x84:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 0,(xx+d),l
		case 0x85:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 0,(xx+d)
		case 0x86:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xFE);
			break;
				
		// res 0,(xx+d),a
		case 0x87:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xFE;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 1,(xx+d),b
		case 0x88:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 1,(xx+d),c
		case 0x89:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 1,(xx+d),d
		case 0x8A:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 1,(xx+d),e
		case 0x8B:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 1,(xx+d),h
		case 0x8C:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 1,(xx+d),l
		case 0x8D:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 1,(xx+d)
		case 0x8E:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xFD);
			break;
				
		// res 1,(xx+d),a
		case 0x8F:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xFD;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 2,(xx+d),b
		case 0x90:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 2,(xx+d),c
		case 0x91:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 2,(xx+d),d
		case 0x92:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 2,(xx+d),e
		case 0x93:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 2,(xx+d),h
		case 0x94:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 2,(xx+d),l
		case 0x95:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 2,(xx+d)
		case 0x96:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xFB);
			break;
				
		// res 2,(xx+d),a
		case 0x97:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xFB;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 3,(xx+d),b
		case 0x98:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 3,(xx+d),c
		case 0x99:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 3,(xx+d),d
		case 0x9A:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 3,(xx+d),e
		case 0x9B:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 3,(xx+d),h
		case 0x9C:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 3,(xx+d),l
		case 0x9D:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 3,(xx+d)
		case 0x9E:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xF7);
			break;
				
		// res 3,(xx+d),a
		case 0x9F:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xF7;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 4,(xx+d),b
		case 0xA0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 4,(xx+d),c
		case 0xA1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 4,(xx+d),d
		case 0xA2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 4,(xx+d),e
		case 0xA3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 4,(xx+d),h
		case 0xA4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 4,(xx+d),l
		case 0xA5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 4,(xx+d)
		case 0xA6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xEF);
			break;
				
		// res 4,(xx+d),a
		case 0xA7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xEF;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 5,(xx+d),b
		case 0xA8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 5,(xx+d),c
		case 0xA9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 5,(xx+d),d
		case 0xAA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 5,(xx+d),e
		case 0xAB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 5,(xx+d),h
		case 0xAC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 5,(xx+d),l
		case 0xAD:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 5,(xx+d)
		case 0xAE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xDF);
			break;
				
		// res 5,(xx+d),a
		case 0xAF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xDF;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 6,(xx+d),b
		case 0xB0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 6,(xx+d),c
		case 0xB1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 6,(xx+d),d
		case 0xB2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 6,(xx+d),e
		case 0xB3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 6,(xx+d),h
		case 0xB4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 6,(xx+d),l
		case 0xB5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 6,(xx+d)
		case 0xB6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0xBF);
			break;
				
		// res 6,(xx+d),a
		case 0xB7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0xBF;
      DataBus->Write(xxd,regs.A);
			break;
				
		// res 7,(xx+d),b
		case 0xB8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.B);
			break;
				
		// res 7,(xx+d),c
		case 0xB9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.C);
			break;
				
		// res 7,(xx+d),d
		case 0xBA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.D);
			break;
				
		// res 7,(xx+d),e
		case 0xBB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.E);
			break;
				
		// res 7,(xx+d),h
		case 0xBC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.H);
			break;
				
		// res 7,(xx+d),l
		case 0xBD:
			tStates += 23;
      regs.L = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.L);
			break;
				
		// res 7,(xx+d)
		case 0xBE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) & 0x7F);
			break;
				
		// res 7,(xx+d),a
		case 0xBF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) & 0x7F;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 0,(xx+d),b
		case 0xC0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 0,(xx+d),c
		case 0xC1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 0,(xx+d),d
		case 0xC2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 0,(xx+d),e
		case 0xC3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 0,(xx+d),h
		case 0xC4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 0,(xx+d),l
		case 0xC5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 0,(xx+d)
		case 0xC6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x01);
			break;
				
		// set 0,(xx+d),a
		case 0xC7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x01;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 1,(xx+d),b
		case 0xC8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 1,(xx+d),c
		case 0xC9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 1,(xx+d),d
		case 0xCA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 1,(xx+d),e
		case 0xCB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 1,(xx+d),h
		case 0xCC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 1,(xx+d),l
		case 0xCD:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 1,(xx+d)
		case 0xCE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x02);
			break;
				
		// set 1,(xx+d),a
		case 0xCF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x02;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 2,(xx+d),b
		case 0xD0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 2,(xx+d),c
		case 0xD1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 2,(xx+d),d
		case 0xD2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 2,(xx+d),e
		case 0xD3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 2,(xx+d),h
		case 0xD4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 2,(xx+d),l
		case 0xD5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 2,(xx+d)
		case 0xD6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x04);
			break;
				
		// set 2,(xx+d),a
		case 0xD7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x04;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 3,(xx+d),b
		case 0xD8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 3,(xx+d),c
		case 0xD9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 3,(xx+d),d
		case 0xDA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 3,(xx+d),e
		case 0xDB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 3,(xx+d),h
		case 0xDC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 3,(xx+d),l
		case 0xDD:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 3,(xx+d)
		case 0xDE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x08);
			break;
				
		// set 3,(xx+d),a
		case 0xDF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x08;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 4,(xx+d),b
		case 0xE0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 4,(xx+d),c
		case 0xE1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 4,(xx+d),d
		case 0xE2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 4,(xx+d),e
		case 0xE3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 4,(xx+d),h
		case 0xE4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 4,(xx+d),l
		case 0xE5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 4,(xx+d)
		case 0xE6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x10);
			break;
				
		// set 4,(xx+d),a
		case 0xE7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x10;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 5,(xx+d),b
		case 0xE8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 5,(xx+d),c
		case 0xE9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 5,(xx+d),d
		case 0xEA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 5,(xx+d),e
		case 0xEB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 5,(xx+d),h
		case 0xEC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 5,(xx+d),l
		case 0xED:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 5,(xx+d)
		case 0xEE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x20);
			break;
				
		// set 5(xx+d),,a
		case 0xEF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x20;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 6,(xx+d),b
		case 0xF0:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 6,(xx+d),c
		case 0xF1:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 6,(xx+d),d
		case 0xF2:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 6,(xx+d),e
		case 0xF3:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 6,(xx+d),h
		case 0xF4:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 6,(xx+d),l
		case 0xF5:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 6,(xx+d)
		case 0xF6:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x40);
			break;
				
		// set 6,(xx+d),a
		case 0xF7:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x40;
      DataBus->Write(xxd,regs.A);
			break;
				
		// set 7,(xx+d),b
		case 0xF8:
			tStates += 23;
      regs.B = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.B);
			break;
				
		// set 7,(xx+d),c
		case 0xF9:
			tStates += 23;
      regs.C = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.C);
			break;
				
		// set 7,(xx+d),d
		case 0xFA:
			tStates += 23;
      regs.D = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.D);
			break;
				
		// set 7,(xx+d),e
		case 0xFB:
			tStates += 23;
      regs.E = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.E);
			break;
				
		// set 7,(xx+d),h
		case 0xFC:
			tStates += 23;
      regs.H = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.H);
			break;
				
		// set 7,(xx+d),l
		case 0xFD:
			tStates += 23;
      regs.L = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.L);
			break;
				
		// set 7,(xx+d)
		case 0xFE:
			tStates += 23;
      DataBus->Write(xxd,DataBus->Read(xxd) | 0x80);
			break;
				
		// set 7,(xx+d),a
		case 0xFF:
			tStates += 23;
      regs.A = DataBus->Read(xxd) | 0x80;
      DataBus->Write(xxd,regs.A);
			break;
  }  
}