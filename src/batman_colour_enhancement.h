#ifndef BATMAN_COLOUR_ENHANCEMENT_H
#define BATMAN_COLOUR_ENHANCEMENT_H
#include "colour_enhancement.h"
#include "batman_materials.h"

// Exact local 48K Batman release. Rendering addresses verified by tracing:
// 8E1D dispatches a clipped, shifted masked sprite blit into A000 (6-byte rows).
// Alternate BC=destination, DE=ink, HL=mask; B=height. 9691=source stride.
// 8D9A finishes the region before 82EA copies it to VRAM. 8CD1/3 hold its bounds.
// Observe the real ordered blits, including occluding scenery; never sort again.
class BatmanColourEnhancement {
public:
  bool enabled=false;
  static bool Matches(const SpectrumMachine &m) {
    static const unsigned char expected[]={0xc0,0x61,0x9e,0x7f,0x31,0x04,0x75,0x63,
      0xbf,0xb9,0x1b,0xa5,0x04,0xe0,0x02,0xb1,0xef,0x09,0xc9,0x95,0x5a,0x11,
      0x47,0xb5,0xbb,0x4e,0x10,0xff,0x59,0x51,0xc4,0x27};
    unsigned char digest[32];SHA256(m.tape.data(),m.tape.size(),digest);
    return memcmp(digest,expected,32)==0;
  }
  void Attach(SpectrumMachine &m) {
    generation=m.SessionGeneration();matched=Matches(m);Reset();
    m.observationPC=0x8e1d;m.observationPC2=0x8d9a;m.observationContext=this;
    m.instructionObserver=[](void *p,SpectrumMachine &machine){
      static_cast<BatmanColourEnhancement *>(p)->Observe(machine);
    };
  }
  bool Available(SpectrumMachine &m) const {
    return enabled && valid && generation==m.SessionGeneration() && sceneRoom==Word(m,0x658b) && Scene(m) &&
      m.FrameCounter()>=frame && m.FrameCounter()-frame<12;
  }
  bool Present(SpectrumMachine &m,ALLEGRO_BITMAP *bitmap) {
    if(!Available(m))return true;
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READWRITE);
    if(!lock)return false;
    for(int y=0;y<160;++y)for(unsigned x=0;x<256;++x)if(PaintsPixel(x,y))
      memcpy(static_cast<unsigned char *>(lock->data)+(y+24)*lock->pitch+(x+32)*4,
        pixels.data()+(y*256+x)*4,4);
    al_unlock_bitmap(bitmap);return true;
  }
  const std::array<unsigned char,256*160*4> &Pixels() const{return pixels;}
  // Equipment icons at (232,136) and (200,152) overlap the room's bounding
  // rectangle. Their original colours and all remaining HUD pixels are kept.
  static bool PaintsPixel(unsigned x,unsigned y) {
    return x<256 && y<160 && !(x>=232 && y>=136) &&
      !(x>=200 && x<224 && y>=152);
  }
  static bool SupportedRoom(unsigned room){return FindRoom(room)!=nullptr;}
  unsigned CapturedRegions() const{return captures;}
  unsigned RejectedRegions() const{return rejected;}
private:
  std::array<unsigned char,256*160*4> pixels{};
  std::array<unsigned,256*8> regionColours{};
  std::array<unsigned char,256> reconstructed{};
  bool matched=false,valid=false,initialized=false,pending=false,bad=false;
  unsigned left=0,right=0,top=0,bottom=0,captures=0,rejected=0,sceneRoom=0xffff;
  struct Room;
  const Room *roomInfo=nullptr;
  bool flash=false;
  std::uint64_t generation=0,frame=0;
  static unsigned Word(SpectrumMachine &m,unsigned a){return m.bus.Read(a)+256*m.bus.Read(a+1);}
  struct Room {
    unsigned id,xmin,ymin,xmax,ymax,floorInk,floorPaper,wallInk;
  };
  static const Room *FindRoom(unsigned room) {
    static const Room rooms[]={
      {0x8a40,32,8,48,72,0xc2b18d,0x34434a,0x9daebc},
      {0x8940,16,16,64,64,0x99bfc0,0x293e47,0x89aaa7},
      {0x9a40,8,24,72,56,0xc7b88e,0x393a36,0xb8a585},
      {0x8840,24,8,56,72,0xb5b795,0x303d3b,0xa8ad98},
      {0x9940,8,24,72,56,0xb6acc7,0x343247,0xa79bae},
      {0x8a30,16,16,64,64,0xb7b3a5,0x333e4a,0x97aabd}
    };
    for(const auto &r:rooms)if(r.id==room)return &r;
    return nullptr;
  }
  static bool Scene(SpectrumMachine &m) {
    const auto *r=FindRoom(Word(m,0x658b));
    return r && m.Configuration().model==MachineModel::Spectrum48 &&
      m.bus.Read(0x6a41)==r->xmin && m.bus.Read(0x6a42)==r->ymin &&
      m.bus.Read(0x6a43)==r->xmax && m.bus.Read(0x6a44)==r->ymax &&
      m.bus.Read(0x8f20)>=0x10 && m.bus.Read(0x8f20)<=0x17 && m.bus.Read(0x9b06)==0;
  }
  void Reset(){valid=initialized=pending=bad=false;captures=rejected=0;frame=0;sceneRoom=0xffff;roomInfo=nullptr;}
  void Put(unsigned x,unsigned y,unsigned rgb) {
    const unsigned at=(y*256+x)*4;
    pixels[at]=rgb>>16;pixels[at+1]=rgb>>8;pixels[at+2]=rgb;pixels[at+3]=255;
  }
  // Invert the game's floor projection using each checked room's real bounds.
  // The base floor is Z=192; individual objects and raised surfaces keep their
  // own masks and material colours rather than inheriting this floor palette.
  unsigned Scenery(unsigned x,unsigned y,bool ink) const {
    const int px=x,py=y;
    const int worldX4=238+px-2*py,worldY4=494-px-2*py;
    const bool floor=roomInfo && worldX4>=int(roomInfo->xmin*4) &&
      worldX4<=int(roomInfo->xmax*4) && worldY4>=int(roomInfo->ymin*4) &&
      worldY4<=int(roomInfo->ymax*4);
    if(floor)return ink?roomInfo->floorInk:roomInfo->floorPaper;
    return ink?(roomInfo?roomInfo->wallInk:0x8ca4b8):0x0b1320;
  }
  unsigned Original(SpectrumMachine &m,unsigned x,unsigned y,bool ink) const {
    if(x>=256 || y>=192)return ink?0xd7d7d7:0;
    const unsigned attr=m.bus.Read(0x5800+(y/8)*32+x/8);
    if((attr&128) && flash)ink=!ink;
    return dwColorTable[((attr&64)?8:0)+(ink?(attr&7):((attr>>3)&7))]>>8;
  }
  static bool Material(unsigned drawing,bool ink,unsigned &rgb) {
    if(drawing<0x10){rgb=ink?0xc9bca1:0x39424b;return true;}
    switch(drawing) {
      case 0x3d:case 0x3e:case 0x50:case 0x58:case 0x5d:case 0x5f:
        rgb=ink?0xc3b59a:0x49463e;return true; // stone blocks / pedestals
      case 0x3f:case 0x41:case 0x43:case 0x51:case 0x55:case 0x56:case 0x57:case 0x59:
        rgb=ink?0x9db6c6:0x32424d;return true; // metal platforms / springs
      case 0x4d:case 0x53:case 0x54:case 0x5c:
        rgb=ink?0xbea16f:0x4d4034;return true; // crates / raised platforms
      default:return false; // Unchecked actors/items retain Spectrum colours.
    }
  }
  bool Bounds(SpectrumMachine &m) {
    left=m.bus.Read(0x8cd2);right=m.bus.Read(0x8cd1);
    top=m.bus.Read(0x8cd4);bottom=m.bus.Read(0x8cd3);
    return left>=64 && right>left && right<=192 && top>=72 && bottom>top &&
      bottom-top<=42 && (left&3)==0 &&
      (right-left+3)/4<=6 && (bottom-top)*6<=256;
  }
  void Begin(SpectrumMachine &m) {
    pending=true;bad=!Bounds(m);if(bad)return;
    flash=m.ula.CaptureState().blinkState;
    for(unsigned i=0;i<256;++i)reconstructed[i]=m.bus.Read(0xa000+i);
    const unsigned width=(right-left+3)/4;
    for(unsigned y=0;y<bottom-top;++y)for(unsigned byte=0;byte<width;++byte)
      for(unsigned bit=0;bit<8;++bit) {
        const unsigned at=y*6+byte,x=(left-64)*2+byte*8+bit,sy=top-72+y;
        regionColours[at*8+bit]=Scenery(x,sy,reconstructed[at]&(128>>bit));
      }
  }
  void Observe(SpectrumMachine &m) {
    if(generation!=m.SessionGeneration()){Attach(m);return;}
    if(!enabled || !matched || !Scene(m)){Reset();return;}
    if(sceneRoom!=Word(m,0x658b) || (initialized && m.FrameCounter()<frame))Reset();
    sceneRoom=Word(m,0x658b);roomInfo=FindRoom(sceneRoom);
    if(!initialized) {
      for(unsigned y=0;y<160;++y)for(unsigned x=0;x<256;++x) {
        const unsigned address=0x4000+((y&192)<<5)+((y&7)<<8)+((y&56)<<2)+x/8;
        Put(x,y,Scenery(x,y,m.bus.Read(address)&(128>>(x&7))));
      }
      initialized=true;
    }
    if(m.cpu.regs.PC==0x8e1d) {
      if(!pending)Begin(m);
      if(bad)return;
      const unsigned stride=m.bus.Read(0x9691),height=m.cpu.regs.B;
      unsigned width=0;
      if(stride>=3 && stride<=5) {
        const unsigned table=Word(m,0x8e1e + (stride-3)*2);
        for(unsigned n=0;n<stride;++n)if(Word(m,table+n*2)==m.cpu.regs.HL)width=n+1;
      }
      const unsigned dest=m.cpu.regs.altBC,inkBase=m.cpu.regs.altDE,maskBase=m.cpu.regs.altHL;
      if(!width || !height || height>42 || dest<0xa000 || dest>=0xa100 ||
         inkBase+height*stride>65536 || maskBase+height*stride>65536){bad=true;return;}
      const unsigned object=Word(m,0x8daa),drawing=m.bus.Read(0x9692)&127;
      const int cx=128+int(m.bus.Read(object+3))-int(m.bus.Read(object+4));
      const int q=(2*m.bus.Read(object+5)-m.bus.Read(object+3)-m.bus.Read(object+4)+127)&255;
      const bool upper=m.bus.Read(object+7)&2,reflected=m.bus.Read(0x9692)&128;
      const int spriteLeft=2*(cx-70),spriteTop=q-(upper?16:12)-72;
      for(unsigned y=0;y<height;++y)for(unsigned byte=0;byte<width;++byte) {
        const unsigned at=((dest&255)+y*6+byte)&255;
        const unsigned ink=m.bus.Read(inkBase+y*stride+byte),mask=m.bus.Read(maskBase+y*stride+byte);
        reconstructed[at]=(reconstructed[at]&mask)|ink;
        for(unsigned bit=0;bit<8;++bit) {
          if((mask&(128>>bit)) && !(ink&(128>>bit)))continue;
          const int x=(left-64)*2+(at%6)*8+bit,sy=top-72+at/6;
          const bool set=ink&(128>>bit);
          unsigned rgb=Original(m,x,sy,set);
          if(drawing>=0x10 && drawing<=0x17) {
            int localX=x-spriteLeft,localY=sy-spriteTop+(upper?0:16);
            if(reflected)localX=23-localX;
            rgb=BatmanMaterials::Colour(drawing,localX,localY,set);
          } else Material(drawing,set,rgb);
          regionColours[at*8+bit]=rgb;
        }
      }
      return;
    }
    if(!pending)Begin(m);
    if(!bad) {
      const unsigned width=(right-left+3)/4;
      for(unsigned y=0;y<bottom-top && !bad;++y)for(unsigned byte=0;byte<width;++byte)
        if(reconstructed[y*6+byte]!=m.bus.Read(0xa000+y*6+byte)){bad=true;break;}
      if(!bad) {
        for(unsigned y=0;y<bottom-top;++y)for(unsigned byte=0;byte<width;++byte)
          for(unsigned bit=0;bit<8;++bit) {
            const unsigned x=(left-64)*2+byte*8+bit,sy=top-72+y;
            if(x<256 && sy<160)Put(x,sy,regionColours[(y*6+byte)*8+bit]);
          }
        valid=true;frame=m.FrameCounter();++captures;
      }
    }
    if(bad){valid=false;initialized=false;++rejected;}
    pending=false;bad=false;
  }
};
#endif
