#ifndef ARMY_COLOUR_ENHANCEMENT_H
#define ARMY_COLOUR_ENHANCEMENT_H
#include "colour_enhancement.h"
#include "army_enemy_colours.h"

// Local Army Moves tape, SHA-256 checked by Matches(). Addresses verified by
// tracing this release, not borrowed from another game/version:
// BB6B: masked sprite header, FF14=y, FF15=x in four-pixel units, FF16=art.
// CF56: completed scene, before F2A5 restores the next background.
// F2C7 uploads 5DC0 (104 rows x 30 bytes) at screen (8,16).
// 6B90 is the clean background (scrolled after drawing, so snapshot 5DC0
// before the first sprite rather than reading 6B90 at the end); 69F0 the composed 32-column attributes.
// Each source row contains width/2 ink bytes followed by width/2 mask bytes.
class ArmyColourEnhancement {
public:
  bool enabled=false;
  static bool Matches(const SpectrumMachine &m) {
    static const unsigned char expected[]={0x9c,0xce,0x91,0xfa,0x70,0x3d,0x74,0xf3,
      0x1a,0x80,0x92,0x18,0xa9,0x16,0x2b,0x26,0xea,0x0c,0x44,0x5d,0xe3,0x82,
      0xad,0x23,0x08,0xc8,0x31,0xe4,0xec,0x75,0x00,0xec};
    unsigned char digest[32];SHA256(m.tape.data(),m.tape.size(),digest);
    return memcmp(digest,expected,32)==0;
  }
  void Attach(SpectrumMachine &m) {
    valid=false;count=0;overflow=false;generation=m.SessionGeneration();
    matched=Matches(m);m.observationPC=0xbb6b;m.observationPC2=0xcf56;
    m.observationContext=this;
    m.instructionObserver=[](void *p,SpectrumMachine &machine){
      static_cast<ArmyColourEnhancement *>(p)->Observe(machine);
    };
  }
  bool Available(SpectrumMachine &m) const {
    return enabled && valid && generation==m.SessionGeneration() &&
      m.Configuration().model==MachineModel::Spectrum48 && m.bus.Read(0xcbe8)==0 &&
      m.bus.Read(0xcbea)==0 && m.bus.Read(0xcbf4)==0 &&
      m.FrameCounter()>=frame && m.FrameCounter()-frame<12;
  }
  bool Present(SpectrumMachine &m,ALLEGRO_BITMAP *bitmap) {
    if(!Available(m))return true;
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READWRITE);
    if(!lock)return false;
    for(int y=0;y<104;++y)memcpy(static_cast<unsigned char *>(lock->data)+
      (y+40)*lock->pitch+40*4,pixels.data()+y*240*4,240*4);
    al_unlock_bitmap(bitmap);return true;
  }
  const std::array<unsigned char,240*104*4> &Pixels() const {return pixels;}
private:
  struct Sprite {int x,y;unsigned art,width,height;};
  std::array<Sprite,128> sprites{};
  std::array<unsigned char,240*104*4> pixels{};
  std::array<unsigned char,3120> reconstructed{},background{};
  unsigned count=0;bool valid=false,matched=false,overflow=false,flash=false;
  std::uint64_t generation=0,frame=0;
  static unsigned Word(SpectrumMachine &m,unsigned a){return m.bus.Read(a)+256*m.bus.Read(a+1);}
  void Put(unsigned x,unsigned y,unsigned rgb) {
    const unsigned at=(y*240+x)*4;
    pixels[at]=rgb>>16;pixels[at+1]=rgb>>8;pixels[at+2]=rgb;pixels[at+3]=255;
  }
  unsigned Original(SpectrumMachine &m,unsigned x,unsigned y,bool ink) {
    const unsigned attr=m.bus.Read(0x69f0+(y/8)*32+x/8);
    if((attr&128) && flash)ink=!ink;
    return dwColorTable[((attr&64)?8:0)+(ink?(attr&7):((attr>>3)&7))]>>8;
  }
  // Authored regions for the two original 40x24 jeep wheel drawings. Keep
  // every ink/mask bit and opaque black detail; these do not redraw the jeep.
  static unsigned Jeep(unsigned x,unsigned y,bool ink) {
    if(y>=16 && ((int(x)-14)*(int(x)-14)+(int(y)-19)*(int(y)-19)<38 ||
                 (int(x)-31)*(int(x)-31)+(int(y)-19)*(int(y)-19)<38)) {
      if(!ink)return 0x10151c;
      if((x==14 || x==31) && y>=18 && y<=20)return 0xc4cbd0;
      return y>=21?0x505966:0x88939d;
    }
    if(x<7)return ink?0xa1a89b:0x242c29; // radiator and front bumper
    if(y<=9 && x>=17 && x<=24)return ink?0xa3d4da:0x243e46; // windscreen
    if(y<=8 && x<18)return ink?0xb4be80:0x39432e; // canvas / raised bonnet
    return ink?(y<14?0xa6b873:0x80954f):0x293622;
  }
  // Static distant scenery: deliberately softer than the copper bridge and
  // vehicle palettes. Screen coordinates keep it stable across game updates.
  static unsigned Backdrop(unsigned x,unsigned y) {
    static const int far[][2]={{0,57},{24,39},{38,47},{66,23},{86,43},
      {108,35},{137,60},{166,33},{185,49},{214,27},{239,50}};
    static const int near[][2]={{0,79},{28,64},{51,75},{83,51},{104,69},
      {130,60},{157,78},{188,54},{211,68},{239,58}};
    auto ridge=[&](const int (*points)[2],unsigned count) {
      for(unsigned n=1;n<count;++n)if(int(x)<=points[n][0]) {
        const int width=points[n][0]-points[n-1][0];
        return points[n-1][1]+(int(x)-points[n-1][0])*
          (points[n][1]-points[n-1][1])/width;
      }
      return points[count-1][1];
    };
    const int distant=ridge(far,sizeof(far)/sizeof(far[0]));
    const int closer=ridge(near,sizeof(near)/sizeof(near[0]));
    if(int(y)>=closer) {
      // Broad shaded faces and faint atmospheric haze below the bridge.
      const unsigned haze=(y-unsigned(closer))/12;
      const unsigned base=(x<83 || (x>=157 && x<188))?0x23384b:0x293e51;
      return base+(haze<<16)+(haze<<8)+haze;
    }
    if(int(y)>=distant) {
      if(int(y)<distant+3)return 0x3a5065;
      return (x<66 || (x>=137 && x<166) || x>=214)?0x30465c:0x354b60;
    }
    const unsigned glow=std::min(y,60u)/6;
    return ((0x13+glow)<<16)|((0x20+glow)<<8)|(0x31+glow);
  }
  void Observe(SpectrumMachine &m) {
    if(generation!=m.SessionGeneration()){Attach(m);return;}
    if(!enabled || !matched || m.Configuration().model!=MachineModel::Spectrum48){valid=false;count=0;return;}
    if(m.bus.Read(0xcbe8)!=0 || m.bus.Read(0xcbea)!=0 || m.bus.Read(0xcbf4)!=0){valid=false;count=0;return;}
    if(m.cpu.regs.PC==0xbb6b) {
      if(count==0)for(unsigned i=0;i<background.size();++i)background[i]=m.bus.Read(0x5dc0+i);
      const unsigned art=Word(m,0xff16),height=m.bus.Read(art),bytes=m.bus.Read(art+1);
      if(!height || height>104 || !bytes || bytes>60 || (bytes&1) || art+2+height*bytes>65536){overflow=true;return;}
      if(count==sprites.size()){overflow=true;return;}
      sprites[count++]={int(static_cast<std::int8_t>(m.bus.Read(0xff15)))*4,
        int(static_cast<std::int8_t>(m.bus.Read(0xff14))),art,bytes*4,height};
      return;
    }
    bool jeep=false;
    for(unsigned n=0;n<count;++n)jeep|=sprites[n].art==0x7f0c || sprites[n].art==0x7ffe;
    valid=false;
    // Reject modified code and incomplete/death/transition captures.
    if(!overflow && jeep && m.bus.Read(0xcf56)==0xcd && m.bus.Read(0xbb6b)==0x2a) {
      reconstructed=background;
      flash=m.ula.CaptureState().blinkState;
      // Clean scenery first; then replay every observed mask in original order.
      for(unsigned y=0;y<104;++y)for(unsigned x=0;x<240;++x) {
        const bool ink=background[y*30+x/8]&(128>>(x&7));
        // The clean opening-stage layer is the bridge and empty sky. Its
        // palette must not inherit attributes written by overlaid columns.
        Put(x,y,ink?0xb58457:Backdrop(x,y));
      }
      for(unsigned n=0;n<count;++n) {
        const auto &s=sprites[n];const unsigned bytes=s.width/8;
        for(unsigned y=0;y<s.height;++y)for(unsigned x=0;x<s.width;++x) {
          const int dx=s.x+int(x),dy=s.y+int(y);
          if(dx<0 || dx>=240 || dy<0 || dy>=104)continue;
          const unsigned a=s.art+2+y*bytes*2+x/8,bit=128>>(x&7);
          const bool ink=m.bus.Read(a)&bit,transparent=m.bus.Read(a+bytes)&bit;
          if(transparent && !ink)continue;
          const unsigned index=dy*30+dx/8,screenBit=128>>(dx&7);
          if(ink)reconstructed[index]|=screenBit;else reconstructed[index]&=~screenBit;
          unsigned rgb=Original(m,dx,dy,ink);
          if(s.art==0x7f0c || s.art==0x7ffe)rgb=Jeep(x,y,ink);
          else if(s.art==0x9172)rgb=ink?0xd7e4dd:0x45566a; // mountain drawing
          else if(s.art==0x963c || s.art==0x9458 || s.art==0x955a)
            rgb=ink?0xb58457:Backdrop(dx,dy); // openings reveal the same distant scenery
          else if(s.art==0x971e || s.art==0x8e88)
            rgb=ink?0xc6b89b:0x30383c; // broken / intact stone column
          else ArmyEnemyColours::Colour(s.art,x,y,ink,rgb);
          // Unchecked actors and explosions retain original INK/PAPER.
          Put(dx,dy,rgb);
        }
      }
      valid=true;
      for(unsigned i=0;i<reconstructed.size();++i)
        if(reconstructed[i]!=m.bus.Read(0x5dc0+i)){valid=false;break;}
      frame=m.FrameCounter();
    }
    count=0;overflow=false;
  }
};
#endif
