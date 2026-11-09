#ifndef COLOUR_ENHANCEMENT_H
#define COLOUR_ENHANCEMENT_H
#include <array>
#include <openssl/sha.h>
#include "emulator.h"

// Experimental, presentation-only adapter for bin/manic.tap (Bug-Byte).
// Memory map: https://skoolkit.ca/disassemblies/manic_miner/asm/34574.html
// Keep game-specific knowledge outside the CPU, ULA and machine contracts.
class ManicColourEnhancement {
public:
  bool enabled = false;
  void Attach(SpectrumMachine &machine) {
    machine.observationPC2=0x10000;
    machine.observationPC=34771; // Bitmap AND attributes have both been copied.
    machine.observationContext=this;
    machine.instructionObserver=[](void *context,SpectrumMachine &m) {
      static_cast<ManicColourEnhancement *>(context)->Capture(m);
    };
  }
  bool Available(SpectrumMachine &m) {
    return enabled && valid && generation==m.SessionGeneration() &&
      m.FrameCounter()>=frame && m.FrameCounter()-frame<12 &&
      m.bus.Read(33799)==cavern && SupportedCavern(cavern) &&
      m.bus.Read(33882)==0 && m.bus.Read(33880)==0;
  }
  bool Present(SpectrumMachine &m,ALLEGRO_BITMAP *bitmap) {
    if(!Available(m))return true;
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READWRITE);
    if(!lock)return false;
    // GPU bitmap locks can have a negative pitch. Keep row arithmetic signed
    // so rows above the returned origin are addressed correctly.
    for(int y=0;y<128;++y)
      memcpy(static_cast<unsigned char *>(lock->data)+(y+24)*lock->pitch+32*4,
        pixels.data()+y*256*4,256*4);
    al_unlock_bitmap(bitmap);return true;
  }
  const std::array<unsigned char,256*128*4> &Pixels() const {return pixels;}
  static bool SupportedCavern(unsigned room) {
    // These rooms have additional drawing actors/effects requiring adapters:
    // Eugene, Kong (twice), Skylabs and the Solar Power Generator beam.
    return room<20 && room!=4 && room!=7 && room!=11 && room!=13 && room!=18;
  }
private:
  unsigned cavern=0;
  struct Guardian {unsigned x,y,sprite,attribute;};
  bool valid=false,matched=false;
  std::uint64_t generation=~std::uint64_t(0),frame=0;
  std::array<unsigned char,256*128*4> pixels{};
  // A shared four-tone treatment keeps each guardian's original body hue.
  // Coordinates are local to the sprite, so colours follow movement/animation.
  static unsigned GuardianColour(unsigned attribute,unsigned row) {
    const unsigned body=dwColorTable[((attribute&64)?8:0)+(attribute&7)];
    if(!(attribute&7))return body; // Preserve intentionally black graphics.
    if(row>=14)return 0x536a9000; // Steel-blue feet / lower detail.
    if(row>=10) {
      const unsigned ink=attribute&7;
      return (ink==1 || ink==4 || ink==5)?0xffb34700:0x40c8ff00;
    }
    if(row>=4)return body;
    return ((body>>24)+255)/2<<24|((((body>>16)&255)+255)/2)<<16|
        ((((body>>8)&255)+255)/2)<<8;
  }
  static unsigned BackgroundColour(unsigned room,unsigned x,unsigned y) {
    // Decorative distant scenery only; never writes to the game bitmap/RAM.
    static const unsigned themes[]={0x0b1526,0x201326,0x10251d,0x271910,0x101e2c};
    const unsigned base=themes[room%5],light=y/16;
    unsigned colour=(((base>>16)+light)<<24)|(((((base>>8)&255)+light)<<16))|
      (((base&255)+light)<<8);
    if(room!=0)return colour; // First illustrated backdrop: Central Cavern.
    // Faceted rock wall, with dim seams rather than foreground-like outlines.
    const int px=int(x),py=int(y);
    const int facet=(px/32+py/24)%3;
    colour=facet==0?0x1b283800:facet==1?0x20304400:0x24344900;
    const int seam=24+((px*3/8+11)%29);
    if(py==seam || py==seam+47)colour=0x14213100;
    // Two distant tunnel mouths, dark inside an irregular stone arch.
    for(int centre : {78,182}) {
      const int dx=px-centre;
      const int roof=60-(25-std::min(25,dx*dx/24));
      if(dx>=-27 && dx<=27 && py>=roof && py<119) {
        colour=0x30405100;
        if(dx>=-22 && dx<=22 && py>=roof+5) {
          colour=py<76?0x0b152300:0x101d2c00;
          // Rails disappear into the tunnel, deliberately low contrast.
          const int spread=3+std::max(0,py-70)/3;
          if(py>74 && (dx==spread || dx==-spread))colour=0x28374700;
        }
      }
      // Recessed timber supports: two posts, lintel and small diagonal braces.
      const int height=38;
      if((dx>=-25 && dx<=-22 && py>=height && py<119) ||
         (dx>=22 && dx<=25 && py>=height && py<119) ||
         (dx>=-28 && dx<=28 && py>=height-3 && py<=height))
        colour=(dx==-25 || dx==22 || py==height-3)?0x4b403700:0x352d2900;
      if(py>height && py<height+12 &&
         (dx==-22+(py-height) || dx==22-(py-height)))colour=0x41373000;
    }
    // Rough overhead rock and foreground-free distant rock at the foot.
    const int tooth=6+((px/17*7+3)%15);
    if(py<tooth)colour=(px%17<8)?0x30425500:0x24344800;
    const int foot=118-((px/23*5)%11);
    if(py>foot)colour=(px%23<11)?0x26394b00:0x1c2d3e00;
    return colour;
  }
  static unsigned TileColour(unsigned type,bool ink,unsigned x,unsigned y,unsigned room) {
    if(type==0) {
      if(!ink)return BackgroundColour(room,x,y);
      return 0x46556d00; // Existing background pattern, where present.
    }
    // Highlight, body, shadow, mortar/paper. Original bitmap shapes are kept.
    static const unsigned colours[7][4]={
      {0xa9df7500,0x9c804b00,0x62503400,0x252b2400}, // floor: moss and earth
      {0xffd68c00,0xc6985e00,0x80603f00,0x30251d00}, // crumbling sandstone
      {0xbed0df00,0x8198ae00,0x4a607800,0x202c3b00}, // stone walls
      {0xffd65e00,0x60c5c600,0x367f9900,0x192c3600}, // conveyor: brass/steel
      {0xffd28400,0xff714b00,0xb8324800,0x301b2700}, // hazard, warm
      {0xffb4e400,0xe567aa00,0x8d367b00,0x291d3200}, // hazard, magenta
      {0xb1dba000,0x77ad8000,0x466f6400,0x202c2c00}  // extra floor tile
    };
    const auto &palette=colours[type-1];
    if(!ink)return palette[3];
    const unsigned row=y&7;
    if(row<2)return palette[0];
    if(row>=6)return palette[2];
    return palette[1+(type==3 && (x&7)==7)];
  }
  void Capture(SpectrumMachine &m) {
    if(generation!=m.SessionGeneration()) {
      generation=m.SessionGeneration();valid=false;
      static const unsigned char expected[32]={
        0x02,0x29,0xca,0x09,0xbb,0x87,0xc5,0x80,0x24,0xd6,0x8b,0x05,0xa6,0xa5,0xec,0xd8,
        0xac,0x93,0xfb,0x49,0x32,0xd9,0x37,0x88,0x37,0x22,0x46,0x7f,0x86,0x27,0x7f,0x4e};
      unsigned char digest[32];SHA256(m.tape.data(),m.tape.size(),digest);
      matched=memcmp(digest,expected,32)==0;
    }
    valid=false;
    if(!enabled || !matched || m.Configuration().model!=MachineModel::Spectrum48 ||
       !SupportedCavern(m.bus.Read(33799)) || m.bus.Read(33882)!=0 || m.bus.Read(33880)!=0)return;
    cavern=m.bus.Read(33799);
    // Reject altered drawing code, menus, demo mode, unsupported caverns and flashes.
    if(m.bus.Read(34771)!=0xdd || m.bus.Read(34772)!=0x21 ||
       m.bus.Read(37503)!=0x3a || m.bus.Read(37504)!=0x68)return;
    const unsigned wx=(m.bus.Read(32876)&31)*8,wy=m.bus.Read(32872)/2;
    if(wx>240 || wy>112)return;
    const unsigned sprite=33280+(m.bus.Read(32874)&1)*128+(m.bus.Read(32873)&3)*32;
    std::array<Guardian,8> guardians{};unsigned count=0;
    std::array<bool,512> actorCells{};
    auto mark=[&](unsigned x,unsigned y) {
      for(unsigned row=y/8;row<=(y+15)/8 && row<16;++row)
        for(unsigned col=x/8;col<=(x+15)/8 && col<32;++col)actorCells[row*32+col]=true;
    };
    mark(wx,wy);
    auto add=[&](unsigned x,unsigned y,unsigned graphic,unsigned attr) {
      if(x>240 || y>112 || count==guardians.size())return;
      guardians[count++]={x,y,graphic,attr};mark(x,y);
    };
    for(unsigned slot=0;slot<4;++slot) {
      const unsigned base=32958+slot*7,attr=m.bus.Read(base);
      if(attr==255)break;
      if(!attr)continue;
      const unsigned location=m.bus.Read(base+1)+256*m.bus.Read(base+2);
      if(location<23552 || location>=24064)continue;
      unsigned animation=m.bus.Read(base+4)&7;
      if(cavern>=7 && cavern!=9 && cavern!=15)animation|=4;
      add(((location-23552)%32)*8,((location-23552)/32)*8,
          33024+animation*32,attr&127);
    }
    if(cavern>=8)for(unsigned slot=0;slot<4;++slot) {
      const unsigned base=32989+slot*7,attr=m.bus.Read(base);
      if(attr==255)break;
      if(!attr)continue;
      add((m.bus.Read(base+3)&31)*8,m.bus.Read(base+2)&127,
          33024+(m.bus.Read(base+1)&3)*32,attr&127);
    }
    auto covers=[&](unsigned x,unsigned y,unsigned sx,unsigned sy,unsigned graphic) {
      return x>=sx && x<sx+16 && y>=sy && y<sy+16 &&
        (m.bus.Read(graphic+(y-sy)*2+(x-sx)/8)&(0x80>>((x-sx)&7)));
    };
    // Classify by the game's own tile attributes, independently of sprites.
    std::array<unsigned char,512> tileTypes{};
    tileTypes.fill(255);
    for(unsigned cell=0;cell<512;++cell) {
      const unsigned attr=m.bus.Read(24064+cell);
      for(unsigned type=0;type<8;++type)if(attr==m.bus.Read(32800+type*9)) {
        tileTypes[cell]=type;break;
      }
    }
    const bool flash=m.ula.CaptureState().blinkState;
    for(unsigned y=0;y<128;++y)for(unsigned x=0;x<256;++x) {
      const unsigned cell=(y/8)*32+x/8;
      const unsigned address=0x4000+((y&0xc0)<<5)+((y&7)<<8)+((y&0x38)<<2)+x/8;
      const bool bit=(m.bus.Read(address)&(0x80>>(x&7)))!=0;
      unsigned attr=m.bus.Read(0x5800+cell);
      // Sprite colours belong to sprite pixels, not to surrounding scenery.
      if(actorCells[cell])
        attr=m.bus.Read(24064+cell);
      bool ink=bit;if((attr&128)&&flash)ink=!ink;
      unsigned colour=dwColorTable[((attr&64)?8:0)+(ink?(attr&7):((attr>>3)&7))];
      const unsigned backgroundAttr=m.bus.Read(24064+cell);
      const bool backgroundBit=(m.bus.Read(address+0x3000)&(0x80>>(x&7)))!=0;
      // Preserve live items/portals and attribute animation. The moving actors
      // are overlaid below, so their shared cells still get coloured scenery.
      if(tileTypes[cell]<8 && !(backgroundAttr&128) &&
         (actorCells[cell] || attr==backgroundAttr ||
          (tileTypes[cell]==0 && !bit && !backgroundBit &&
           (attr&0xf8)==(backgroundAttr&0xf8))) &&
         (bit==backgroundBit || tileTypes[cell]==4))
        colour=TileColour(tileTypes[cell],bit,x,y,cavern);
      for(unsigned n=0;n<count;++n) {
        const auto &g=guardians[n];
        if(covers(x,y,g.x,g.y,g.sprite))
          colour=GuardianColour(g.attribute,y-g.y);
      }
      if(covers(x,y,wx,wy,sprite)) {
        const unsigned row=y-wy;
        colour=row<4?0xffd34d00:row<7?0xffc09000:row<12?0x40c8ff00:0x4878ef00;
      }
      const unsigned at=(y*256+x)*4;
      pixels[at]=colour>>24;pixels[at+1]=colour>>16;pixels[at+2]=colour>>8;pixels[at+3]=255;
    }
    frame=m.FrameCounter();valid=true;
  }
};
#endif
