#include <cassert>
#include <cstdio>
#include <set>
#include <allegro5/allegro_image.h>
#include "../game_colour_enhancement.h"

int main(int argc,char **) {
  assert(al_init());assert(al_init_image_addon());
  ALLEGRO_DISPLAY *display=nullptr;
  if(argc>1){display=al_create_display(640,480);assert(display);}
  al_set_new_bitmap_flags(display?ALLEGRO_VIDEO_BITMAP:ALLEGRO_MEMORY_BITMAP);
  auto *bitmap=al_create_bitmap(320,240);assert(bitmap);
  SpectrumMachine m;std::string error;
  assert(m.LoadROM(FindROM("48.rom"),error));assert(m.Open("army.tap",error));
  assert(ArmyColourEnhancement::Matches(m));
  m.ula.SetBitmap(bitmap);ArmyColourEnhancement enhancement;
  enhancement.enabled=true;enhancement.Attach(m);
  struct EnemyTrace {
    void *context;void (*observer)(void *,SpectrumMachine &);
    std::set<unsigned> drawings;
    bool visible=false,ready=false;
  } trace;
  trace.context=m.observationContext;trace.observer=m.instructionObserver;
  m.observationContext=&trace;
  m.instructionObserver=[](void *p,SpectrumMachine &machine) {
    auto &t=*static_cast<EnemyTrace *>(p);
    if(machine.cpu.regs.PC==0xbb6b) {
      unsigned art=machine.bus.Read(0xff16)+256*machine.bus.Read(0xff17),colour=0;
      if(ArmyEnemyColours::Colour(art,0,0,true,colour)) {
        t.drawings.insert(art);
        int x=static_cast<std::int8_t>(machine.bus.Read(0xff15));
        if(x>0 && x<48)t.visible=true;
      }
    }
    t.observer(t.context,machine);
    if(machine.cpu.regs.PC==0xcf56){t.ready=t.visible;t.visible=false;}
  };
  bool enemyPreviewSaved=false;
  unsigned enhanced=0,gaps=0;bool checkpointSaved=false;
  SpectrumMachine::State checkpoint=m.CaptureState();
  std::set<unsigned> poses,xPositions;
  for(int f=0;f<2900;++f) {
    auto key=[&](int when,int row,int col){
      if(f==when)m.ula.PressKey(row,col,true);
      if(f==when+10)m.ula.PressKey(row,col,false);
    };
    key(500,2,6);
    if(f==520 || f==540)m.ula.PressKey(3,8,true);
    key(520,1,9);key(540,1,9);
    if(f==550)m.ula.PressKey(3,8,false);
    key(560,2,9);key(2000,0,2); // LOAD "", then menu 3 (Play).
    // Defaults decoded from FBFA's key table: Q jump, P right, A / CAPS fire.
    if(f>=2000) {
      m.ula.PressKey(1,0,f>=2050 && f<2085);
      m.ula.PressKey(1,9,f>=2100 && f<2150);
      m.ula.PressKey(2,0,f>=2300 && f<2360);
      m.ula.PressKey(3,0,f>=2380 && f<2440);
    }
    m.Frame();
    if(!enhancement.Available(m)){if(enhanced && m.bus.Read(0xcbf4)==0 && m.bus.Read(0xcbea)==0)++gaps;continue;}
    ++enhanced;xPositions.insert(m.bus.Read(0xcc00));poses.insert(m.bus.Read(0xcbf0)*256+m.bus.Read(0xcbfe));
    auto before=m.CaptureState();
    m.ula.SetBitmap(bitmap);assert(m.ula.Present());
    if(enhanced==80)assert(al_save_bitmap("/tmp/army-original.png",bitmap));
    assert(enhancement.Present(m,bitmap));
    if(trace.ready && !enemyPreviewSaved) {
      assert(al_save_bitmap("/tmp/army-enemies-gameplay.png",bitmap));enemyPreviewSaved=true;
    }
    auto after=m.CaptureState();
    assert(before.memory.ram==after.memory.ram);
    assert(memcmp(&before.registers,&after.registers,sizeof(before.registers))==0);
    assert(before.ula.screenRGB==after.ula.screenRGB);
    assert(memcmp(before.ula.frameAudio,after.ula.frameAudio,sizeof(before.ula.frameAudio))==0);
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(lock);
    for(int y=0;y<104;++y)assert(memcmp(static_cast<unsigned char *>(lock->data)+
      (y+40)*lock->pitch+40*4,enhancement.Pixels().data()+y*240*4,240*4)==0);
    al_unlock_bitmap(bitmap);
    if(enhanced==80){assert(al_save_bitmap("/tmp/army-enhanced.png",bitmap));checkpoint=m.CaptureState();checkpointSaved=true;}
  }
  printf("Army: %u enhanced frames, %zu player poses, %u fallback frames outside death\n",enhanced,poses.size(),gaps);
  printf("Real enemy drawings observed: %zu\n",trace.drawings.size());
  assert(!trace.drawings.empty() && enemyPreviewSaved);
  assert(enhanced>200 && poses.size()>=2 && xPositions.size()>2 && checkpointSaved);
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<40;++n)m.Frame();
  auto observed=m.CaptureState();assert(m.RestoreState(checkpoint));m.instructionObserver=nullptr;
  for(unsigned n=0;n<40;++n)m.Frame();
  auto plain=m.CaptureState();
  assert(observed.memory.ram==plain.memory.ram);
  assert(memcmp(&observed.registers,&plain.registers,sizeof(plain.registers))==0);
  assert(observed.ula.screenRGB==plain.ula.screenRGB);
  assert(memcmp(observed.ula.frameAudio,plain.ula.frameAudio,sizeof(plain.ula.frameAudio))==0);
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<20 && !enhancement.Available(m);++n)m.Frame();
  assert(enhancement.Available(m));
  // Synthetic shared-cell fixture: every opaque jeep pixel must keep the same
  // material over two PAPER colours, with the original source silhouette.
  std::array<unsigned char,240*104*4> first{};
  for(unsigned paper=0;paper<2;++paper) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    m.bus.Write(0xcbe8,0);m.bus.Write(0xcbea,0);m.bus.Write(0xcbf4,0);
    for(unsigned i=0;i<3120;++i)m.bus.Write(0x5dc0+i,0);
    for(unsigned i=0;i<416;++i)m.bus.Write(0x69f0+i,7|(paper?16:0));
    m.bus.Write(0xff14,32);m.bus.Write(0xff15,20);
    m.bus.Write(0xff16,0xfe);m.bus.Write(0xff17,0x7f);
    m.cpu.regs.PC=0xbb6b;m.instructionObserver(m.observationContext,m);
    for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
      unsigned a=0x8000+y*10+x/8,bit=128>>(x&7);
      bool ink=m.bus.Read(a)&bit,transparent=m.bus.Read(a+5)&bit;
      unsigned target=0x5dc0+(32+y)*30+(80+x)/8;
      if(ink)m.bus.Write(target,m.bus.Read(target)|(128>>((80+x)&7)));
      else if(!transparent)m.bus.Write(target,m.bus.Read(target)&~(128>>((80+x)&7)));
    }
    auto fixture=m.CaptureState();
    m.cpu.regs.PC=0xcf56;m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m));assert(m.CaptureState().memory.ram==fixture.memory.ram);
    if(!paper) {
      first=enhancement.Pixels();
      const int sheetFlags=al_get_new_bitmap_flags();
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      auto *sheet=al_create_bitmap(320,96);assert(sheet);
      al_set_new_bitmap_flags(sheetFlags);
      auto *previousTarget=al_get_target_bitmap();al_set_target_bitmap(sheet);
      for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
        unsigned a=0x8000+y*10+x/8,bit=128>>(x&7);
        bool ink=m.bus.Read(a)&bit,transparent=(m.bus.Read(a+5)&bit) && !ink;
        unsigned at=((32+y)*240+80+x)*4;
        ALLEGRO_COLOR originalColour=transparent?al_map_rgb(32,32,48):
          ink?al_map_rgb(215,215,215):al_map_rgb(0,0,0);
        ALLEGRO_COLOR enhancedColour=transparent?al_map_rgb(32,32,48):
          al_map_rgb(first[at],first[at+1],first[at+2]);
        for(unsigned dy=0;dy<4;++dy)for(unsigned dx=0;dx<4;++dx) {
          al_put_pixel(x*4+dx,y*4+dy,originalColour);
          al_put_pixel(160+x*4+dx,y*4+dy,enhancedColour);
        }
      }
      assert(al_save_bitmap("/tmp/army-jeep-comparison.png",sheet));
      al_set_target_bitmap(previousTarget);al_destroy_bitmap(sheet);
    }
    else for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
      unsigned a=0x8000+y*10+x/8,bit=128>>(x&7);
      if((m.bus.Read(a)&bit) || !(m.bus.Read(a+5)&bit)) {
        unsigned at=((32+y)*240+80+x)*4;
        assert(memcmp(first.data()+at,enhancement.Pixels().data()+at,4)==0);
      }
    }
  }
  // Bridge ends and stone columns share attribute cells with the clean
  // bridge. Changing those attributes must not split either material palette.
  std::array<unsigned char,240*104*4> scenery{};
  for(unsigned variation=0;variation<2;++variation) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    m.bus.Write(0xcbe8,0);m.bus.Write(0xcbea,0);m.bus.Write(0xcbf4,0);
    for(unsigned i=0;i<3120;++i)m.bus.Write(0x5dc0+i,0x55);
    for(unsigned i=0;i<416;++i)m.bus.Write(0x69f0+i,variation?0x47:0x42);
    auto draw=[&](unsigned art,unsigned px,unsigned py) {
      m.bus.Write(0xff14,py);m.bus.Write(0xff15,px/4);
      m.bus.Write(0xff16,art&255);m.bus.Write(0xff17,art>>8);
      m.cpu.regs.PC=0xbb6b;m.instructionObserver(m.observationContext,m);
      unsigned height=m.bus.Read(art),stride=m.bus.Read(art+1),bytes=stride/2;
      for(unsigned y=0;y<height;++y)for(unsigned x=0;x<bytes*8;++x) {
        if(px+x>=240 || py+y>=104)continue;
        unsigned a=art+2+y*stride+x/8,bit=128>>(x&7);
        bool ink=m.bus.Read(a)&bit,transparent=m.bus.Read(a+bytes)&bit;
        unsigned target=0x5dc0+(py+y)*30+(px+x)/8,screenBit=128>>((px+x)&7);
        if(ink)m.bus.Write(target,m.bus.Read(target)|screenBit);
        else if(!transparent)m.bus.Write(target,m.bus.Read(target)&~screenBit);
      }
    };
    draw(0x7ffe,80,32);draw(0x963c,0,64);draw(0x971e,60,64);
    draw(0x9458,100,64);draw(0x955a,140,80);draw(0x8e88,200,64);
    const unsigned enemies[]={0x7960,0x7a52,0x7b44,0x7c36,0x7d28,0x7e1a,0x80f0,0x81e2};
    for(unsigned n=0;n<8;++n)draw(enemies[n],(n%4)*48,(n/4)*24);
    m.cpu.regs.PC=0xcf56;m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m));
    // Every pose, including opaque source zeroes, is material-stable over
    // different PAPER colours. Compare the composed pixels, not just helpers.
    if(!variation)scenery=enhancement.Pixels();else assert(scenery==enhancement.Pixels());
    auto rgb=[&](unsigned x,unsigned y) {
      unsigned at=(y*240+x)*4;const auto &pixels=enhancement.Pixels();
      return unsigned(pixels[at])<<16|unsigned(pixels[at+1])<<8|pixels[at+2];
    };
    for(unsigned n=0;n<8;++n) {
      unsigned art=enemies[n],px=(n%4)*48,py=(n/4)*24;
      for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
        unsigned a=art+2+y*10+x/8,bit=128>>(x&7);
        bool ink=m.bus.Read(a)&bit,transparent=m.bus.Read(a+5)&bit;
        if(transparent && !ink)continue;
        unsigned expected=0;assert(ArmyEnemyColours::Colour(art,x,y,ink,expected));
        assert(rgb(px+x,py+y)==expected);
      }
    }
    if(!variation) {
      const int sheetFlags=al_get_new_bitmap_flags();
      al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
      auto *sheet=al_create_bitmap(480,288);assert(sheet);
      al_set_new_bitmap_flags(sheetFlags);
      auto *previousTarget=al_get_target_bitmap();al_set_target_bitmap(sheet);
      for(unsigned n=0;n<8;++n)for(unsigned y=0;y<24;++y)for(unsigned x=0;x<40;++x) {
        unsigned a=enemies[n]+2+y*10+x/8,bit=128>>(x&7);
        bool ink=m.bus.Read(a)&bit,transparent=(m.bus.Read(a+5)&bit) && !ink;
        unsigned at=(((n/4)*24+y)*240+(n%4)*48+x)*4;
        const auto &pixels=enhancement.Pixels();
        ALLEGRO_COLOR originalColour=transparent?al_map_rgb(32,32,48):
          ink?al_map_rgb(215,215,215):al_map_rgb(0,0,0);
        ALLEGRO_COLOR enhancedColour=transparent?al_map_rgb(32,32,48):
          al_map_rgb(pixels[at],pixels[at+1],pixels[at+2]);
        for(unsigned dy=0;dy<3;++dy)for(unsigned dx=0;dx<3;++dx) {
          unsigned sx=(n%2)*240+x*3+dx,sy=(n/2)*72+y*3+dy;
          al_put_pixel(sx,sy,originalColour);al_put_pixel(sx+120,sy,enhancedColour);
        }
      }
      assert(al_save_bitmap("/tmp/army-enemies-comparison.png",sheet));
      al_set_target_bitmap(previousTarget);al_destroy_bitmap(sheet);
    }
    // Known clean bridge bits remain copper even inside white column cells.
    assert(rgb(239,103)==0xb58457);assert(rgb(238,103)!=0xb58457 && rgb(238,103)!=0x101d2c);
    // Distant scenery varies spatially but remains stable across attribute
    // changes (the full-frame comparison above). No bridge ink is replaced.
    std::set<unsigned> backdropColours;
    for(unsigned y=48;y<104;y+=8)for(unsigned x=232;x<240;x+=2)
      backdropColours.insert(rgb(x,y));
    assert(backdropColours.size()>=3);
    unsigned checkedOpenings=0;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<56;++x) {
      unsigned a=0x963e + y*14+x/8,bit=128>>(x&7);
      if(!(m.bus.Read(a)&bit) && !(m.bus.Read(a+7)&bit)) {
        assert(rgb(x,64+y)!=0x101d2c && rgb(x,64+y)!=0xb58457);
        ++checkedOpenings;
      }
    }
    assert(checkedOpenings>10);
    for(unsigned art:{0x963cu,0x971eu,0x8e88u}) {
      unsigned px=art==0x963c?0:art==0x971e?60:200;
      unsigned stride=m.bus.Read(art+1),bytes=stride/2,checked=0;
      for(unsigned y=0;y<m.bus.Read(art);++y)for(unsigned x=0;x<bytes*8;++x)
        if(m.bus.Read(art+2+y*stride+x/8)&(128>>(x&7))) {
          assert(rgb(px+x,64+y)==(art==0x963c?0xb58457u:0xc6b89bu));++checked;
        }
      assert(checked>10);
    }
  }
  // A missing sprite / direct bitmap change must reject the reconstruction.
  m.bus.Write(0x5dc0,m.bus.Read(0x5dc0)^128);
  m.cpu.regs.PC=0xcf56;m.instructionObserver(m.observationContext,m);
  assert(!enhancement.Available(m));
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<20 && !enhancement.Available(m);++n)m.Frame();
  assert(enhancement.Available(m));
  enhancement.enabled=false;assert(!enhancement.Available(m));enhancement.enabled=true;
  m.bus.Write(0xcbe8,1);assert(!enhancement.Available(m)); // Later stage fallback.
  m.bus.Write(0xcbe8,0);m.bus.Write(0xcbf4,1);assert(!enhancement.Available(m)); // Death.
  // Frontend selector must erase enhanced pixels when falling back, including
  // unchanged original ULA pixels that its incremental upload would skip.
  assert(m.RestoreState(checkpoint));
  GameColourEnhancement frontend;frontend.enabled=true;frontend.Attach(m);
  for(unsigned n=0;n<20;++n)m.Frame();
  m.ula.SetBitmap(bitmap);assert(m.ula.Present());
  std::array<unsigned char,320*240*4> original{};
  auto readBitmap=[&]() {
    std::array<unsigned char,320*240*4> result{};
    auto *r=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(r);
    for(int y=0;y<240;++y)memcpy(result.data()+y*320*4,
      static_cast<unsigned char *>(r->data)+y*r->pitch,320*4);
    al_unlock_bitmap(bitmap);return result;
  };
  original=readBitmap();assert(frontend.Present(m,bitmap));assert(readBitmap()!=original);
  frontend.enabled=false;assert(frontend.Present(m,bitmap));assert(readBitmap()==original);
  assert(m.Open("manic.tap",error));assert(!enhancement.Available(m));assert(!ArmyColourEnhancement::Matches(m));
  GameColourEnhancement selector;selector.enabled=true;selector.Attach(m);
  assert(m.observationPC==34771 && m.observationPC2==0x10000);
  assert(m.Open("army.tap",error));selector.Prepare(m);
  assert(m.observationPC==0xbb6b && m.observationPC2==0xcf56);
  m.Reset();selector.Prepare(m);assert(selector.Present(m,bitmap));
  al_destroy_bitmap(bitmap);if(display)al_destroy_display(display);
  puts("Army colour enhancement checks passed");
}
