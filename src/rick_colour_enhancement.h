#ifndef RICK_COLOUR_ENHANCEMENT_H
#define RICK_COLOUR_ENHANCEMENT_H
#include "colour_enhancement.h"
#include "rick_materials.h"
#include "rick_scenery.h"

// Exact 48K Rick Dangerous II tape, Hyde Park player and scenery enhancement.
// 7329: ordered masked actor draws (IX record). 7D91: completed 76D4 upload.
// Source rows pack ink[2,1,0], mask[0,2,1]; header=vertical offset,height.
// World Y=screen Y+64. Foreground tiles (A800 bit7) suppress actor uploads.
class RickColourEnhancement {
public:
  bool enabled=false;
  static bool Matches(const SpectrumMachine &m) {
    static const unsigned char expected[]={0x45,0x92,0xe6,0x10,0x38,0x50,0x98,0x12,
      0x28,0x00,0xc5,0x06,0x51,0xaf,0x7b,0xff,0x82,0xb6,0x05,0xa9,0x45,0xba,
      0x1e,0x18,0x56,0x01,0xa9,0x84,0xf8,0x89,0x6b,0xe8};
    unsigned char digest[32];SHA256(m.tape.data(),m.tape.size(),digest);
    return memcmp(digest,expected,32)==0;
  }
  void Attach(SpectrumMachine &m) {
    generation=m.SessionGeneration();matched=Matches(m);Reset();
    m.observationPC=0x7329;m.observationPC2=0x7d91;m.observationContext=this;
    m.instructionObserver=[](void *p,SpectrumMachine &machine){static_cast<RickColourEnhancement *>(p)->Observe(machine);};
  }
  bool Available(SpectrumMachine &m) const {
    return enabled && valid && generation==m.SessionGeneration() && Scene(m) && scene==SceneKey(m) &&
      m.FrameCounter()>=frame && m.FrameCounter()-frame<12;
  }
  bool Present(SpectrumMachine &m,ALLEGRO_BITMAP *bitmap) {
    if(!Available(m))return true;
    // Restore original pixels even when their monochrome bits did not change
    // as Rick moved. A sparse colour overlay must never leave coloured trails.
    m.ula.SetBitmap(bitmap);if(!m.ula.Present())return false;
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READWRITE);
    if(!lock)return false;
    for(int y=8;y<192;++y)for(unsigned x=0;x<256;++x) {
      const unsigned at=(y*256+x)*4;
      const unsigned char *colour=pixels[at+3]?pixels.data()+at:
        (scenery[at+3]?scenery.data()+at:nullptr);
      if(colour)memcpy(static_cast<unsigned char *>(lock->data)+(y+24)*lock->pitch+(x+32)*4,colour,4);
    }
    al_unlock_bitmap(bitmap);return true;
  }
  const std::vector<unsigned char> &Pixels() const{return pixels;}
  const std::vector<unsigned char> &SceneryPixels() const{return scenery;}
  unsigned CapturedFrames() const{return captures;}
  unsigned RejectedFrames() const{return rejected;}
  static unsigned Word(SpectrumMachine &m,unsigned a){return m.bus.Read(a)+256*m.bus.Read(a+1);}
  static bool Scene(SpectrumMachine &m) {
    static const unsigned maxScroll[]={0,112,112,0,104,104,8,8};
    const unsigned room=m.bus.Read(0x9b93);
    return m.Configuration().model==MachineModel::Spectrum48 &&
      m.bus.Read(0x7ef1)==1 && room<8 && m.bus.Read(0x88d2)==maxScroll[room] &&
      (m.bus.Read(0x88cf)&7)==0 && m.bus.Read(0x88cf)<=m.bus.Read(0x88d2) &&
      m.bus.Read(0x9b9c)==0 && m.bus.Read(0x8a24)==0 && m.bus.Read(0x9bab)==1;
  }
private:
  // Keep the sparse overlay's backing images off the frontend's stack.
  std::vector<unsigned char> pixels=std::vector<unsigned char>(256*192*4);
  std::vector<unsigned char> pendingPixels=std::vector<unsigned char>(256*192*4);
  std::vector<unsigned char> expectedInk=std::vector<unsigned char>(256*192);
  std::vector<unsigned char> scenery=std::vector<unsigned char>(256*192*4);
  // Covers every observed actor, including the object drawn before Rick.
  // Opaque zero bits matter as much as INK: background PAPER must not leak in.
  std::vector<unsigned char> actorMask=std::vector<unsigned char>(256*192);
  bool actorMaskBad=false;
  unsigned actorScene=~0u;
  bool valid=false,matched=false,pending=false,bad=false;
  unsigned captures=0,rejected=0;
  unsigned scene=~0u,pendingScene=~0u;
  static unsigned SceneKey(SpectrumMachine &m){return m.bus.Read(0x7ef1)*65536+m.bus.Read(0x9b93)*256+m.bus.Read(0x88cf);}
  std::uint64_t generation=0,frame=0,lastObservedFrame=0;
  void ClearActors(){std::fill(actorMask.begin(),actorMask.end(),0);actorMaskBad=false;actorScene=~0u;}
  void Reset(){valid=pending=bad=false;captures=rejected=0;frame=lastObservedFrame=0;scene=pendingScene=~0u;std::fill(pixels.begin(),pixels.end(),0);std::fill(pendingPixels.begin(),pendingPixels.end(),0);std::fill(scenery.begin(),scenery.end(),0);ClearActors();}
  static bool Blocked(SpectrumMachine &m,unsigned x,unsigned worldY) {
    const unsigned tile=m.bus.Read(0x6b00+(worldY/8)*32+x/8);
    return (m.bus.Read(0xa800+tile)&128)!=0;
  }
  void CaptureScenery(SpectrumMachine &m) {
    std::fill(scenery.begin(),scenery.end(),0);
    if(actorMaskBad)return;
    const unsigned room=m.bus.Read(0x9b93),scroll=m.bus.Read(0x88cf);
    for(unsigned ty=1;ty<24;++ty)for(unsigned tx=0;tx<32;++tx) {
      const unsigned tile=m.bus.Read(0x6b00+(ty+8)*32+tx);
      if(!RickScenery::Pixel(room,tile,tx*8,ty*8,true,scroll))continue;
      // Require the original graphic at every uncovered pixel. Actor-covered
      // pixels are excluded from validation AND colouring, including zeros.
      // Reject unexplained changes or incomplete uploads as a whole tile.
      bool same=true;
      for(unsigned row=0;row<8;++row) {
        const unsigned y=ty*8+row,a=0x4000+((y&192)<<5)+((y&7)<<8)+((y&56)<<2)+tx;
        unsigned covered=0;
        for(unsigned col=0;col<8;++col)if(actorMask[y*256+tx*8+col])covered|=128>>col;
        if((m.bus.Read(a)^m.bus.Read(0xa900+tile*8+row))&(~covered&255)){same=false;break;}
      }
      if(!same)continue;
      for(unsigned row=0;row<8;++row)for(unsigned col=0;col<8;++col) {
        if(actorMask[(ty*8+row)*256+tx*8+col])continue;
        const bool ink=m.bus.Read(0xa900+tile*8+row)&(128>>col);
        const unsigned rgb=RickScenery::Pixel(room,tile,tx*8+col,ty*8+row,ink,scroll),at=((ty*8+row)*256+tx*8+col)*4;
        if(!rgb)continue;
        scenery[at]=rgb>>16;scenery[at+1]=rgb>>8;scenery[at+2]=rgb;scenery[at+3]=255;
      }
    }
  }
  void Observe(SpectrumMachine &m) {
    if(generation!=m.SessionGeneration()){Attach(m);return;}
    if(m.FrameCounter()<lastObservedFrame)Reset();
    lastObservedFrame=m.FrameCounter();
    if(!enabled || !matched || !Scene(m)){valid=pending=false;ClearActors();return;}
    if(scene!=SceneKey(m))valid=false;
    if(pending && pendingScene!=SceneKey(m)){pending=false;ClearActors();}
    if(actorScene!=SceneKey(m)){ClearActors();actorScene=SceneKey(m);}
    if(m.cpu.regs.PC==0x7329) {
      const unsigned object=m.cpu.regs.IX;
      const bool player=object==0x9bab;
      if(player){std::fill(pendingPixels.begin(),pendingPixels.end(),0);pending=true;pendingScene=SceneKey(m);bad=false;}
      const unsigned y=Word(m,object+2),x=m.bus.Read(object+1),art=Word(m,object+4);
      if((y>>8)>1 || ((y>>8)==1 && (y&255)>=43) || !art)return;
      const unsigned offset=m.bus.Read(art),height=m.bus.Read(art+1);
      if(!height || height>32 || offset+height>32 || art+2+height*6>65536){bad=actorMaskBad=true;return;}
      const bool reflected=m.bus.Read(object+6)&128;
      if(player && (!RickMaterials::Supported(art) || offset+height>24)){bad=true;return;}
      const unsigned inkIndex[]={2,1,0},maskIndex[]={3,5,4};
      for(unsigned row=0;row<height;++row)for(unsigned sx=0;sx<24;++sx) {
        const unsigned source=art+2+row*6,bit=128>>(sx&7),column=sx/8;
        const bool ink=m.bus.Read(source+inkIndex[column])&bit,transparent=m.bus.Read(source+maskIndex[column])&bit;
        if(transparent && !ink)continue;
        const unsigned dx=x+(reflected?23-sx:sx),worldY=y+offset+row;
        if(dx>=256 || worldY<72 || worldY>=256 || Blocked(m,dx,worldY))continue;
        const unsigned at=((worldY-64)*256+dx)*4;
        actorMask[at/4]=1;
        if(!pending)continue;
        if(!player){pendingPixels[at+3]=0;continue;}
        const unsigned rgb=RickMaterials::Colour(art,sx,offset+row,ink);
        pendingPixels[at]=rgb>>16;pendingPixels[at+1]=rgb>>8;pendingPixels[at+2]=rgb;
        pendingPixels[at+3]=255;expectedInk[at/4]=ink;
      }
      return;
    }
    valid=false;
    if(pending && !bad) {
      unsigned visible=0;
      for(unsigned y=8;y<192 && !bad;++y)for(unsigned x=0;x<256;++x) {
        const unsigned at=y*256+x;if(!pendingPixels[at*4+3])continue;
        const unsigned a=0x4000+((y&192)<<5)+((y&7)<<8)+((y&56)<<2)+x/8;
        if(bool(m.bus.Read(a)&(128>>(x&7)))!=bool(expectedInk[at])){bad=true;break;}
        ++visible;
      }
      if(!bad && visible){pixels=pendingPixels;CaptureScenery(m);scene=pendingScene;valid=true;++captures;frame=m.FrameCounter();}
    }
    if(bad)++rejected;
    pending=false;
    ClearActors();
  }
};
#endif
