#include <cassert>
#include <cstdio>
#include <set>
#include <allegro5/allegro_image.h>
#include "../game_colour_enhancement.h"

// The old cyan patch crossed the hair/face boundary. Check its actual source
// INK pixels on every head placement, including crouch and action poses.
static void checkHairFaceBoundary(SpectrumMachine &m) {
  const int headX[]={0,0,0,0,0,0,3,3,0,0,1,0};
  const int headY[]={0,0,-1,0,0,0,5,5,-1,1,1,1};
  const unsigned ci[]={2,1,0};
  for(unsigned pose=0;pose<12;++pose) {
    unsigned checked[2]={0,0};const unsigned art=0xc26d+pose*128;
    for(unsigned band=0;band<2;++band)for(int x=5+headX[pose];x<=13+headX[pose];++x) {
      const unsigned y=5+headY[pose]+band,source=art+2+(y-m.bus.Read(art))*6;
      if(!(m.bus.Read(source+ci[x/8])&(128>>(x&7))))continue;
      assert(RickMaterials::Colour(art,x,y,true)==(band?0xe9b18d:0xe5c15c));
      ++checked[band];
    }
    assert(checked[0]>0 && checked[1]>0);
  }
}

static void saveAtlas(SpectrumMachine &m) {
  const int oldFlags=al_get_new_bitmap_flags();al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  auto *atlas=al_create_bitmap(576,768);assert(atlas);al_set_new_bitmap_flags(oldFlags);
  auto *old=al_get_target_bitmap();al_set_target_bitmap(atlas);al_clear_to_color(al_map_rgb(34,42,55));
  const unsigned inkIndex[]={2,1,0},maskIndex[]={3,5,4};
  for(unsigned mirror=0;mirror<2;++mirror)for(unsigned pose=0;pose<RickMaterials::DrawingCount;++pose) {
    const unsigned art=RickMaterials::Drawing(pose),offset=m.bus.Read(art),height=m.bus.Read(art+1);
    assert(offset+height<=24);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<24;++x) {
      const unsigned bit=128>>(x&7),source=art+2+y*6;
      const bool ink=m.bus.Read(source+inkIndex[x/8])&bit,transparent=m.bus.Read(source+maskIndex[x/8])&bit;
      if(transparent && !ink)continue;
      const unsigned rgb=RickMaterials::Colour(art,x,offset+y,ink);
      const unsigned px=(pose%4)*144+(mirror?23-x:x)*3,py=(mirror*4+pose/4)*96+(offset+y)*3;
      for(unsigned yy=0;yy<3;++yy)for(unsigned xx=0;xx<3;++xx){
        al_put_pixel(px+xx,py+yy,ink?al_map_rgb(215,215,215):al_map_rgb(0,0,0));
        al_put_pixel(px+72+xx,py+yy,al_map_rgb(rgb>>16,(rgb>>8)&255,rgb&255));
      }
    }
  }
  assert(al_save_bitmap("/tmp/rick-player-comparison.png",atlas));
  al_set_target_bitmap(old);al_destroy_bitmap(atlas);
}

static void runUntil(SpectrumMachine &m,unsigned pc) {
  for(unsigned n=0;m.cpu.regs.PC!=pc && n<1000000;++n)m.StepInstruction();
  assert(m.cpu.regs.PC==pc);
}
static void callOriginal(SpectrumMachine &m,unsigned pc,unsigned returnPC=0x7d2d) {
  m.cpu.regs.PC=pc;m.cpu.regs.SP=0xfff0;
  m.bus.Write(0xfff0,returnPC&255);m.bus.Write(0xfff1,returnPC>>8);runUntil(m,returnPC);
}
static void enterRoom(SpectrumMachine &m,unsigned room,unsigned scroll,unsigned x,unsigned y) {
  for(unsigned r=0;r<4;++r)for(unsigned c=0;c<10;++c)m.ula.PressKey(r,c,false);
  for(unsigned a=0x9b95;a<0x9bab;++a)m.bus.Write(a,0);
  m.bus.Write(0x8a24,0);m.bus.Write(0x9bb1,0);
  m.cpu.regs.A=room;m.cpu.regs.C=scroll;m.cpu.regs.E=x;m.cpu.regs.HL=y;
  callOriginal(m,0x8a6d);
}
static void compareExecution(const SpectrumMachine::State &a,const SpectrumMachine::State &b) {
  assert(a.memory.ram==b.memory.ram);
  assert(memcmp(&a.registers,&b.registers,sizeof(a.registers))==0);
  assert(a.ula.screenRGB==b.ula.screenRGB);
  assert(memcmp(a.ula.frameAudio,b.ula.frameAudio,sizeof(a.ula.frameAudio))==0);
}
static void checkOverlay(SpectrumMachine &m,RickColourEnhancement &enhancement,ALLEGRO_BITMAP *bitmap) {
  assert(enhancement.Available(m));const auto before=m.CaptureState();
  assert(enhancement.Present(m,bitmap));const auto after=m.CaptureState();compareExecution(before,after);
  auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(lock);
  const auto &pixels=enhancement.Pixels();
  for(int y=0;y<240;++y)for(unsigned x=0;x<320;++x) {
    const unsigned char *expected=before.ula.screenRGB.data()+(y*320+x)*3;
    if(x>=32 && x<288 && y>=32 && y<216) {
      const unsigned at=((y-24)*256+x-32)*4;if(pixels[at+3])expected=pixels.data()+at;
      else if(enhancement.SceneryPixels()[at+3])expected=enhancement.SceneryPixels().data()+at;
    }
    const auto *actual=static_cast<unsigned char *>(lock->data)+y*lock->pitch+x*4;
    assert(memcmp(actual,expected,3)==0 && actual[3]==255);
  }
  al_unlock_bitmap(bitmap);
}
static unsigned checkScenery(SpectrumMachine &m,RickColourEnhancement &enhancement) {
  unsigned count=0;const auto &scenery=enhancement.SceneryPixels();
  for(unsigned y=8;y<192;++y)for(unsigned x=0;x<256;++x) {
    const unsigned at=(y*256+x)*4;
    const unsigned tile=m.bus.Read(0x6b00+(y/8+8)*32+x/8),room=m.bus.Read(0x9b93);
    // Preserve empty background everywhere; only original art gets coloured.
    if(tile==0)assert(!scenery[at+3]);
    if(!scenery[at+3])continue;
    ++count;
    assert(!enhancement.Pixels()[at+3]);
    const bool ink=m.bus.Read(0xa900+tile*8+(y&7))&(128>>(x&7));
    const unsigned a=0x4000+((y&192)<<5)+((y&7)<<8)+((y&56)<<2)+x/8;
    assert(ink==bool(m.bus.Read(a)&(128>>(x&7))));
    const unsigned rgb=RickScenery::Pixel(room,tile,x,y,ink,m.bus.Read(0x88cf));
    assert(rgb && scenery[at]==(rgb>>16) && scenery[at+1]==((rgb>>8)&255) && scenery[at+2]==(rgb&255));
  }
  return count;
}
static void checkBackgroundActors(SpectrumMachine &m,RickColourEnhancement &enhancement,
                                  const SpectrumMachine::State &checkpoint,ALLEGRO_BITMAP *bitmap) {
  // Actual original blits over non-solid brickwork and original tree graphics.
  // The uncoloured actors
  // straddle Rick in draw order: 9BB4 is drawn BEFORE him in routine 85FA.
  // Their zero pixels must remain original PAPER, even when equal to the wall.
  for(unsigned backdrop:{73u,228u})for(unsigned reflection=0;reflection<2;++reflection)for(unsigned paper=0;paper<2;++paper) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    for(unsigned i=0;i<768;++i)m.bus.Write(0x5800+i,paper?0x47:0x42);
    for(unsigned i=0;i<1024;++i)m.bus.Write(0x6600+i,0);
    for(unsigned ty=20;ty<23;++ty)for(unsigned tx=10;tx<15;++tx) {
      m.bus.Write(0x6b00+ty*32+tx,backdrop);m.bus.Write(0x6600+ty*32+tx,2);
    }
    callOriginal(m,0x76d4,0x7d91);
    const unsigned objects[]={0x9bb4,0x9bab,0x9bbd};
    for(unsigned i=0;i<3;++i) {
      const unsigned object=objects[i];
      m.bus.Write(object,1);m.bus.Write(object+1,80+i*8);
      m.bus.Write(object+2,160);m.bus.Write(object+3,0);
      m.bus.Write(object+4,0x6d);m.bus.Write(object+5,0xc2);m.bus.Write(object+6,reflection?128:0);
      m.cpu.regs.IX=object;if(!i)m.bus.Write(0x77b0,2);
      callOriginal(m,0x7329,0x7d91);
    }
    callOriginal(m,0x76d4,0x7d91);m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m));checkOverlay(m,enhancement,bitmap);
    unsigned protectedZeros=0,visibleWall=0;
    for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
      bool covered=false,zero=false;
      for(unsigned i=0;i<3;++i)if(x>=i*8 && x<i*8+24 && y<21) {
        const unsigned sx=reflection?23-(x-i*8):x-i*8,bit=128>>(sx&7),ci[]={2,1,0},mi[]={3,5,4};
        const unsigned source=0xc26d+2+y*6;
        const bool ink=m.bus.Read(source+ci[sx/8])&bit,mask=m.bus.Read(source+mi[sx/8])&bit;
        if(ink || !mask){covered=true;if(!ink)zero=true;}
      }
      const unsigned at=((96+y)*256+80+x)*4;
      const bool originalInk=m.bus.Read(0xa900+backdrop*8+(y&7))&(128>>(x&7));
      const bool colour=!covered && (backdrop==73 || originalInk);
      assert(bool(enhancement.SceneryPixels()[at+3])==colour);
      protectedZeros+=zero;visibleWall+=!covered;
    }
    assert(protectedZeros>0 && visibleWall>0 && enhancement.RejectedFrames()==0);
  }
  puts("Rick scenery: actors protect INK/opaque zero pixels over brickwork and original foliage, both reflections and PAPER colours");
}
static void checkHydePark(SpectrumMachine &m,RickColourEnhancement &enhancement,
                          const SpectrumMachine::State &checkpoint,ALLEGRO_BITMAP *bitmap) {
  const unsigned maxima[]={0,112,112,0,104,104,8,8};
  // Safe short gameplay spawns discovered through original room-entry probes.
  const unsigned spawnX[]={80,80,208,80,208,80,80,80},spawnY[]={160,160,160,160,160,112,160,160};
  for(unsigned room=0;room<8;++room) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    enterRoom(m,room,0,spawnX[room],spawnY[room]);const auto entry=m.CaptureState();
    unsigned available=0;
    for(unsigned f=0;f<60;++f){m.Frame();available+=enhancement.Available(m);}
    assert(available>30 && enhancement.Available(m));assert(enhancement.RejectedFrames()==0);
    assert(m.bus.Read(0x9b93)==room && m.bus.Read(0x88d2)==maxima[room]);
    const auto observed=m.CaptureState();checkOverlay(m,enhancement,bitmap);
    assert(checkScenery(m,enhancement)>5000);
    char path[120];snprintf(path,sizeof(path),"/tmp/rick-room-%u-enhanced.png",room);assert(al_save_bitmap(path,bitmap));
    m.ula.SetBitmap(bitmap);assert(m.ula.Present());
    snprintf(path,sizeof(path),"/tmp/rick-room-%u-original.png",room);assert(al_save_bitmap(path,bitmap));
    assert(m.RestoreState(entry));m.instructionObserver=nullptr;
    for(unsigned f=0;f<60;++f)m.Frame();compareExecution(observed,m.CaptureState());
    printf("Rick room %u: %u enhanced gameplay frames; execution unchanged\n",room,available);
  }
  unsigned viewports=0;
  for(unsigned room=0;room<8;++room)for(unsigned scroll=0;scroll<=maxima[room];scroll+=8) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    enterRoom(m,room,scroll,80,160);
    assert(m.bus.Read(0x9b93)==room && m.bus.Read(0x88cf)==scroll);
    // Isolate a source-mask draw in this genuinely initialised viewport. This
    // validates rendering, not survival of an arbitrary spawn among hazards.
    m.bus.Write(0x9b9c,0);m.bus.Write(0x9bab,1);
    unsigned bestX=80,bestY=160,best=0;
    for(unsigned y=104;y<=200;y+=24)for(unsigned x=40;x<=200;x+=32) {
      unsigned count=0;
      for(unsigned row=0;row<21;++row)for(unsigned col=0;col<24;++col) {
        const unsigned tile=m.bus.Read(0x6b00+((y+row)/8)*32+(x+col)/8);
        if(!(m.bus.Read(0xa800+tile)&128))++count;
      }
      if(count>best){best=count;bestX=x;bestY=y;}
    }
    assert(best>100);
    m.bus.Write(0x9bac,bestX);m.bus.Write(0x9bad,bestY);m.bus.Write(0x9bae,0);
    const unsigned art=(viewports&1)?0xc8ed:0xc96d;
    m.bus.Write(0x9baf,art&255);m.bus.Write(0x9bb0,art>>8);m.bus.Write(0x9bb1,0);
    for(unsigned i=0;i<1024;++i)m.bus.Write(0x6600+i,0);
    m.bus.Write(0x77b0,2);m.cpu.regs.IX=0x9bab;callOriginal(m,0x7329,0x7d91);
    callOriginal(m,0x76d4,0x7d91);m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m) && enhancement.RejectedFrames()==0);checkOverlay(m,enhancement,bitmap);
    assert(checkScenery(m,enhancement)>512);
    char path[120];snprintf(path,sizeof(path),"/tmp/rick-viewport-%u-%u-enhanced.png",room,scroll);
    assert(al_save_bitmap(path,bitmap));
    ++viewports;
    // Another valid viewport with the same geometry must invalidate immediately.
    m.bus.Write(0x9b93,room==0?3:0);m.bus.Write(0x88d2,0);m.bus.Write(0x88cf,0);
    assert(!enhancement.Available(m));
  }
  assert(viewports==64);printf("Rick: scenery and climbing masks checked in all %u Hyde Park viewports\n",viewports);

  // Place Rick at a genuine ladder and use the original keyboard input. The
  // gold vertical structures further right are not all climbable at their top.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);enterRoom(m,1,0,40,184);
  m.ula.PressKey(1,8,true);std::set<unsigned> climbed;bool savedClimb=false;
  for(unsigned n=0;n<60;++n){m.Frame();const unsigned art=RickColourEnhancement::Word(m,0x9baf);
    if(enhancement.Available(m) && (art==0xc8ed || art==0xc96d)) {
      climbed.insert(art);
      if(!savedClimb) {
        m.ula.SetBitmap(bitmap);assert(m.ula.Present());assert(al_save_bitmap("/tmp/rick-climbing-original.png",bitmap));
        checkOverlay(m,enhancement,bitmap);assert(al_save_bitmap("/tmp/rick-climbing-enhanced.png",bitmap));savedClimb=true;
      }
    }
  }
  m.ula.PressKey(1,8,false);assert(climbed.size()==2 && savedClimb && enhancement.RejectedFrames()==0);
  printf("Rick keyboard climbing: both rear drawings coloured\n");

  // Original exit dispatcher: opening room -> room 2 at its bottom viewport,
  // then back. It computes the destination and scroll offset from exit records.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<8;++n)m.Frame();assert(enhancement.Available(m));
  for(unsigned direction:{2u,1u}) {
    const unsigned targetRoom=direction==2?2:0,targetScroll=direction==2?112:0;
    m.bus.Write(0x9bad,227);m.bus.Write(0x9bae,0);m.bus.Write(0x88d3,direction);
    callOriginal(m,0x8993);
    assert(m.bus.Read(0x9b93)==targetRoom && m.bus.Read(0x88cf)==targetScroll);
    assert(!enhancement.Available(m));unsigned available=0;
    for(unsigned f=0;f<20;++f){m.Frame();available+=enhancement.Available(m);}
    assert(available>0);assert(enhancement.RejectedFrames()==0);
    printf("Rick exit dispatcher: room %u, scroll %u, %u coloured frames\n",targetRoom,targetScroll,available);
  }

  // Exercise the original 64-pixel scroll routines in both directions. Room 1
  // contains a tall continuous map; actor Y and the tile grid move together.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);enterRoom(m,1,0,80,160);
  for(unsigned n=0;n<10;++n)m.Frame();assert(enhancement.Available(m));
  // Preserve the initial scrolled checkpoint before running into that view's
  // hazards, so the reverse-scroll fixture starts alive rather than in death.
  auto scrolledEntry=m.CaptureState();
  for(unsigned direction=0;direction<2;++direction) {
    if(direction){assert(m.RestoreState(scrolledEntry));enhancement.Attach(m);}
    const unsigned before=m.bus.Read(0x88cf);
    m.bus.Write(0x9bad,direction?80:224);m.bus.Write(0x9bae,0);
    callOriginal(m,direction?0x7905:0x78a5);
    assert(m.bus.Read(0x88cf)==(direction?before-8:before+8));
    assert(!enhancement.Available(m));if(!direction)scrolledEntry=m.CaptureState();unsigned available=0;
    for(unsigned n=0;n<12;++n){m.Frame();available+=enhancement.Available(m);}
    assert(available>0 && enhancement.RejectedFrames()==0);
    printf("Rick original scroll %s: %u coloured frames\n",direction?"up":"down",available);
  }

  // Use the original death entry and let the game perform its own respawn.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned r=0;r<4;++r)for(unsigned c=0;c<10;++c)m.ula.PressKey(r,c,false);
  const unsigned lives=m.bus.Read(0x8b3b);callOriginal(m,0x93d0);
  assert(m.bus.Read(0x9b9c)!=0 && !enhancement.Available(m));
  bool sawDeath=false,respawned=false;
  for(unsigned n=0;n<240;++n){m.Frame();
    if(m.bus.Read(0x9b9c)){sawDeath=true;assert(!enhancement.Available(m));}
    else if(sawDeath && enhancement.Available(m))respawned=true;
  }
  assert(sawDeath && respawned && m.bus.Read(0x8b3b)<lives);
  printf("Rick original death/respawn: original death effect, colouring restored\n");

  // Rewind without reattaching: pending pixels and timestamps must be rebuilt.
  assert(m.RestoreState(checkpoint));assert(!enhancement.Available(m));
  for(unsigned n=0;n<12;++n)m.Frame();assert(enhancement.Available(m));checkOverlay(m,enhancement,bitmap);
}

int main(int argc,char **) {
  assert(al_init());assert(al_init_image_addon());ALLEGRO_DISPLAY *display=nullptr;
  if(argc>1){display=al_create_display(640,480);assert(display);}
  al_set_new_bitmap_flags(display?ALLEGRO_VIDEO_BITMAP:ALLEGRO_MEMORY_BITMAP);
  auto *bitmap=al_create_bitmap(320,240);assert(bitmap);
  SpectrumMachine m;std::string error;assert(m.LoadROM(FindROM("48.rom"),error));assert(m.Open("rick2.tap",error));
  RickColourEnhancement enhancement;enhancement.enabled=true;enhancement.Attach(m);
  assert(RickColourEnhancement::Matches(m));m.ula.SetBitmap(bitmap);
  unsigned enhanced=0;std::set<unsigned> drawings,positions,directions;
  auto checkpoint=m.CaptureState();bool saved=false;
  std::array<unsigned char,320*240*4> original{};
  for(int f=0;f<3700;++f) {
    auto key=[&](int when,int row,int col){if(f==when)m.ula.PressKey(row,col,true);if(f==when+10)m.ula.PressKey(row,col,false);};
    key(500,2,6);if(f==520 || f==540)m.ula.PressKey(3,8,true);key(520,1,9);key(540,1,9);
    if(f==550)m.ula.PressKey(3,8,false);key(560,2,9);
    key(1200,3,9); // Dismiss loading picture.
    key(1600,0,0);key(1800,0,9);key(2200,0,0); // Keyboard, start, Hyde Park.
    m.ula.PressKey(3,2,(f>=2450 && f<2510) || (f>=2600 && f<2680)); // X right.
    m.ula.PressKey(1,8,f>=2600 && f<2620); // O jump.
    m.ula.PressKey(2,7,f>=2750 && f<2790); // K crouch.
    m.ula.PressKey(3,1,f>=2850 && f<2880); // Z left, reflected art.
    m.Frame();
    if(!enhancement.Available(m))continue;
    ++enhanced;drawings.insert(RickColourEnhancement::Word(m,0x9baf));positions.insert(m.bus.Read(0x9bac));directions.insert(m.bus.Read(0x9bb1));
    m.ula.SetBitmap(bitmap);assert(m.ula.Present());
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(lock);
    for(int y=0;y<240;++y)memcpy(original.data()+y*320*4,static_cast<unsigned char *>(lock->data)+y*lock->pitch,320*4);
    al_unlock_bitmap(bitmap);
    if(f==2670)assert(al_save_bitmap("/tmp/rick-original.png",bitmap));
    if(f==2670 || f==2671) {
      // Poison the previous host image to ensure Present restores every original
      // pixel itself, even when unchanged monochrome pixels would leave trails.
      auto *target=al_get_target_bitmap();al_set_target_bitmap(bitmap);
      al_clear_to_color(al_map_rgb(255,0,255));al_set_target_bitmap(target);
    }
    auto before=m.CaptureState();assert(enhancement.Present(m,bitmap));auto after=m.CaptureState();
    assert(before.memory.ram==after.memory.ram);
    assert(memcmp(&before.registers,&after.registers,sizeof(before.registers))==0);
    assert(before.ula.screenRGB==after.ula.screenRGB);
    assert(memcmp(before.ula.frameAudio,after.ula.frameAudio,sizeof(before.ula.frameAudio))==0);
    lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(lock);
    for(int y=0;y<240;++y)for(unsigned x=0;x<320;++x) {
      const auto &pixels=enhancement.Pixels();const unsigned char *expected=original.data()+(y*320+x)*4;
      if(x>=32 && x<288 && y>=32 && y<216) {
        const unsigned at=((y-24)*256+x-32)*4;if(pixels[at+3])expected=pixels.data()+at;
        else if(enhancement.SceneryPixels()[at+3])expected=enhancement.SceneryPixels().data()+at;
      }
      assert(memcmp(static_cast<unsigned char *>(lock->data)+y*lock->pitch+x*4,expected,4)==0);
    }
    al_unlock_bitmap(bitmap);
    if(f==2670){assert(al_save_bitmap("/tmp/rick-enhanced.png",bitmap));checkpoint=m.CaptureState();saved=true;}
  }
  printf("Rick: %u enhanced frames, %zu drawings, %zu x positions, %zu directions; %u captures, %u rejects\n",
    enhanced,drawings.size(),positions.size(),directions.size(),enhancement.CapturedFrames(),enhancement.RejectedFrames());
  assert(enhanced>800 && drawings.size()>=7 && positions.size()>10 && directions.size()==2 && saved);
  assert(enhancement.RejectedFrames()==0);checkHairFaceBoundary(m);saveAtlas(m);
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<30;++n)m.Frame();auto observed=m.CaptureState();
  assert(m.RestoreState(checkpoint));m.instructionObserver=nullptr;
  for(unsigned n=0;n<30;++n)m.Frame();auto plain=m.CaptureState();
  assert(observed.memory.ram==plain.memory.ram);
  assert(memcmp(&observed.registers,&plain.registers,sizeof(plain.registers))==0);
  assert(observed.ula.screenRGB==plain.ula.screenRGB);
  assert(memcmp(observed.ula.frameAudio,plain.ula.frameAudio,sizeof(plain.ula.frameAudio))==0);
  checkHydePark(m,enhancement,checkpoint,bitmap);
  checkBackgroundActors(m,enhancement,checkpoint,bitmap);
  // One altered bit rejects the whole scenery tile, while unrelated tiles and
  // Rick remain enhanced. The adapter only reads the original game's memory.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  enterRoom(m,0,0,80,160);for(unsigned n=0;n<8;++n)m.Frame();
  assert(enhancement.Available(m));
  assert(enhancement.SceneryPixels()[(8*256)*4+3]);
  m.cpu.regs.IX=0x9bab;callOriginal(m,0x7329,0x7d91);
  callOriginal(m,0x76d4,0x7d91);
  m.bus.Write(0x4020,m.bus.Read(0x4020)^128); // top-left playable 8x8 tile
  m.instructionObserver(m.observationContext,m);assert(enhancement.Available(m));
  for(unsigned y=8;y<16;++y)for(unsigned x=0;x<8;++x)
    assert(!enhancement.SceneryPixels()[(y*256+x)*4+3]);
  assert(enhancement.SceneryPixels()[(8*256+8)*4+3]);
  checkOverlay(m,enhancement,bitmap);
  // Controlled source-mask draw uses actual game code, including the shifted
  // blitter and upload. Vary PAPER, reflection and pixel alignment, then compare
  // palette independence and foreground/later-actor occlusion below.
  for(unsigned pose=0;pose<RickMaterials::DrawingCount;++pose)for(unsigned reflected=0;reflected<2;++reflected)for(unsigned shift=0;shift<8;++shift)for(unsigned paper=0;paper<2;++paper) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    for(unsigned i=0;i<768;++i)m.bus.Write(0x5800+i,paper?0x47:0x42);
    // Use a clear patch of the real opening room, and a supported walking pose.
    m.bus.Write(0x9bac,80+shift);m.bus.Write(0x9bad,160);m.bus.Write(0x9bae,0);
    const unsigned art=RickMaterials::Drawing(pose);
    m.bus.Write(0x9baf,art&255);m.bus.Write(0x9bb0,art>>8);m.bus.Write(0x9bb1,reflected?128:0);
    // Isolate the original drawing/upload calls. Clear the original dirty flags
    // as its normal update does; these are fixture writes, not adapter behaviour.
    for(unsigned i=0;i<1024;++i)m.bus.Write(0x6600+i,0);
    m.bus.Write(0x77b0,2);m.cpu.regs.IX=0x9bab;m.cpu.regs.PC=0x7329;
    m.cpu.regs.SP=0xfef0;m.bus.Write(0xfef0,0x91);m.bus.Write(0xfef1,0x7d);
    for(unsigned n=0;m.cpu.regs.PC!=0x7d91 && n<200000;++n)m.StepInstruction();
    assert(m.cpu.regs.PC==0x7d91);
    // Draw returns to the capture boundary in this fixture; perform its normal
    // upload first so the capture's bit check remains real, not fabricated.
    m.cpu.regs.PC=0x76d4;m.cpu.regs.SP=0xfef0;
    for(unsigned n=0;m.cpu.regs.PC!=0x7d91 && n<200000;++n)m.StepInstruction();
    assert(m.cpu.regs.PC==0x7d91);
    m.instructionObserver(m.observationContext,m);assert(enhancement.Available(m));
    unsigned coloured=0;const auto &rgb=enhancement.Pixels();
    for(unsigned y=0;y<24;++y)for(unsigned x=0;x<24;++x) {
      const unsigned source=art+2+y*6,bit=128>>(x&7),ci[]={2,1,0},mi[]={3,5,4};
      if(y>=m.bus.Read(art+1))continue;
      bool ink=m.bus.Read(source+ci[x/8])&bit,transparent=m.bus.Read(source+mi[x/8])&bit;
      if(transparent && !ink)continue;
      const unsigned at=((96+m.bus.Read(art)+y)*256+80+shift+(reflected?23-x:x))*4;
      unsigned expected=RickMaterials::Colour(art,x,m.bus.Read(art)+y,ink);
      assert(rgb[at+3]==255 && rgb[at]==(expected>>16) && rgb[at+1]==((expected>>8)&255) && rgb[at+2]==(expected&255));++coloured;
    }
    assert(coloured>150);
  }
  // Real foreground tiles and later masked actors must erase only the
  // player pixels they actually cover, including their opaque zero pixels.
  for(unsigned mode=0;mode<2;++mode) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    m.bus.Write(0x9bac,80);m.bus.Write(0x9bad,160);m.bus.Write(0x9bae,0);
    m.bus.Write(0x9baf,0x6d);m.bus.Write(0x9bb0,0xc2);m.bus.Write(0x9bb1,0);
    for(unsigned i=0;i<1024;++i)m.bus.Write(0x6600+i,0);
    if(mode==0) {
      const unsigned cell=0x6b00+20*32+10,tile=m.bus.Read(cell);
      // A private foreground tile with the same original background graphic.
      for(unsigned i=0;i<8;++i)m.bus.Write(0xa900+255*8+i,m.bus.Read(0xa900+tile*8+i));
      m.bus.Write(0xa8ff,m.bus.Read(0xa800+tile)|128);m.bus.Write(cell,255);
    }
    auto call=[&](unsigned pc,unsigned object) {
      m.cpu.regs.IX=object;m.cpu.regs.PC=pc;m.cpu.regs.SP=0xfef0;
      m.bus.Write(0xfef0,0x91);m.bus.Write(0xfef1,0x7d);
      for(unsigned n=0;m.cpu.regs.PC!=0x7d91 && n<200000;++n)m.StepInstruction();
      assert(m.cpu.regs.PC==0x7d91);
    };
    m.bus.Write(0x77b0,2);call(0x7329,0x9bab);
    if(mode==1) {
      for(unsigned i=0;i<7;++i)m.bus.Write(0x9bb4+i,m.bus.Read(0x9bab+i));
      m.bus.Write(0x9bb5,88);call(0x7329,0x9bb4);
    }
    call(0x76d4,0x9bab);m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m));unsigned hidden=0,visible=0;
    auto opaque=[&](unsigned x,unsigned y) {
      const unsigned ci[]={2,1,0},mi[]={3,5,4},bit=128>>(x&7),source=0xc26d+2+y*6;
      return (m.bus.Read(source+ci[x/8])&bit) || !(m.bus.Read(source+mi[x/8])&bit);
    };
    for(unsigned y=0;y<21;++y)for(unsigned x=0;x<24;++x) {
      const bool covered=mode==0?(x<8 && y<8):(x>=8 && opaque(x-8,y));
      const bool expected=opaque(x,y) && !covered;
      const auto &rgb=enhancement.Pixels();const unsigned at=((96+y)*256+80+x)*4;
      assert(bool(rgb[at+3])==expected);
      if(expected)++visible;else if(opaque(x,y))++hidden;
    }
    assert(hidden>0 && visible>0 && enhancement.RejectedFrames()==0);
  }
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);for(unsigned n=0;n<8;++n)m.Frame();assert(enhancement.Available(m));
  enhancement.enabled=false;assert(!enhancement.Available(m));enhancement.enabled=true;
  m.bus.Write(0x9b93,3);assert(!enhancement.Available(m));m.bus.Write(0x9b93,0);
  m.bus.Write(0x7ef1,2);assert(!enhancement.Available(m));m.bus.Write(0x7ef1,1);
  m.bus.Write(0x9b9c,255);assert(!enhancement.Available(m));
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);assert(!enhancement.Available(m));
  GameColourEnhancement selector;selector.enabled=true;selector.Attach(m);assert(m.observationPC==0x7329 && m.observationPC2==0x7d91);
  assert(m.Open("batman.tap",error));selector.Prepare(m);assert(m.observationPC==0x8e1d);
  assert(!enhancement.Available(m));assert(!RickColourEnhancement::Matches(m));
  m.Reset();assert(!enhancement.Available(m));al_destroy_bitmap(bitmap);if(display)al_destroy_display(display);
  puts("Rick colour enhancement checks passed");
}
