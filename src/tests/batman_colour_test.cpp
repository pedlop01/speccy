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
  assert(m.LoadROM(FindROM("48.rom"),error));assert(m.Open("batman.tap",error));
  BatmanColourEnhancement enhancement;enhancement.enabled=true;enhancement.Attach(m);
  m.ula.SetBitmap(bitmap);
  std::set<unsigned> poses,positions;unsigned enhanced=0;bool saved=false;
  SpectrumMachine::State checkpoint=m.CaptureState();
  std::array<unsigned char,320*240*4> original{};
  for(int f=0;f<3400;++f) {
    auto key=[&](int when,int row,int col) {
      if(f==when)m.ula.PressKey(row,col,true);
      if(f==when+10)m.ula.PressKey(row,col,false);
    };
    key(500,2,6);
    if(f==520 || f==540)m.ula.PressKey(3,8,true);
    key(520,1,9);key(540,1,9);
    if(f==550)m.ula.PressKey(3,8,false);
    key(560,2,9);key(1000,2,9);key(1200,2,9);
    key(2000,2,9);key(2400,2,9); // Dismiss equipment introduction.
    if(f>=2500) {
      m.ula.PressKey(1,0,f<2700); // Q: move
      m.ula.PressKey(1,9,f>=2800 && f<3000); // P: turn / move
      m.ula.PressKey(3,9,f>=3100 && f<3140); // SPACE: jump
    }
    m.Frame();
    if(!enhancement.Available(m))continue;
    ++enhanced;poses.insert(m.bus.Read(0x8f20));
    positions.insert(m.bus.Read(0x8f1d)*65536+m.bus.Read(0x8f1e)*256+m.bus.Read(0x8f1f));
    m.ula.SetBitmap(bitmap);assert(m.ula.Present());
    auto *read=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(read);
    for(int y=0;y<240;++y)memcpy(original.data()+y*320*4,
      static_cast<unsigned char *>(read->data)+y*read->pitch,320*4);
    al_unlock_bitmap(bitmap);
    if(f==3100)assert(al_save_bitmap("/tmp/batman-original.png",bitmap));
    auto before=m.CaptureState();assert(enhancement.Present(m,bitmap));auto after=m.CaptureState();
    assert(before.memory.ram==after.memory.ram);
    assert(memcmp(&before.registers,&after.registers,sizeof(before.registers))==0);
    assert(before.ula.screenRGB==after.ula.screenRGB);
    assert(memcmp(before.ula.frameAudio,after.ula.frameAudio,sizeof(before.ula.frameAudio))==0);
    read=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(read);
    for(int y=0;y<240;++y)for(unsigned x=0;x<320;++x) {
      const bool painted=x>=32 && x<288 && y>=24 &&
        BatmanColourEnhancement::PaintsPixel(x-32,y-24);
      const unsigned char *expected=painted?enhancement.Pixels().data()+((y-24)*256+x-32)*4:
        original.data()+(y*320+x)*4;
      assert(memcmp(static_cast<unsigned char *>(read->data)+y*read->pitch+x*4,expected,4)==0);
    }
    al_unlock_bitmap(bitmap);
    if(f==3100){assert(al_save_bitmap("/tmp/batman-enhanced.png",bitmap));checkpoint=m.CaptureState();saved=true;}
  }
  printf("Batman: %u enhanced frames, %zu poses, %zu positions; %u regions, %u rejects\n",
    enhanced,poses.size(),positions.size(),enhancement.CapturedRegions(),enhancement.RejectedRegions());
  assert(enhanced>1000 && poses.size()>1 && positions.size()>2 && saved);
  assert(enhancement.CapturedRegions()>300 && enhancement.RejectedRegions()==0);
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<30;++n)m.Frame();auto observed=m.CaptureState();
  assert(m.RestoreState(checkpoint));m.instructionObserver=nullptr;
  for(unsigned n=0;n<30;++n)m.Frame();auto plain=m.CaptureState();
  assert(observed.memory.ram==plain.memory.ram);
  assert(memcmp(&observed.registers,&plain.registers,sizeof(plain.registers))==0);
  assert(observed.ula.screenRGB==plain.ula.screenRGB);
  assert(memcmp(observed.ula.frameAudio,plain.ula.frameAudio,sizeof(plain.ula.frameAudio))==0);
  // Controlled overlapping masked draws: a later column must occlude Batman,
  // including opaque zero pixels; reverse order must put Batman in front.
  unsigned overlapColours[2][2]={{0,0},{0,0}};
  for(unsigned variation=0;variation<2;++variation)for(unsigned order=0;order<2;++order) {
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    m.bus.Write(0x9b06,0);m.bus.Write(0x8f20,0x13);
    for(unsigned i=0;i<768;++i)m.bus.Write(0x5800+i,variation?0x47:0x42);
    for(unsigned i=0;i<256;++i)m.bus.Write(0xa000+i,0);
    m.bus.Write(0x8cd2,96);m.bus.Write(0x8cd1,108);
    m.bus.Write(0x8cd4,96);m.bus.Write(0x8cd3,128);
    for(unsigned i=0;i<96;++i){m.bus.Write(0xab00+i,0);m.bus.Write(0xac00+i,255);}
    const unsigned sample=8*3+1;
    m.bus.Write(0xab00+sample,0x20);m.bus.Write(0xac00+sample,0xcf);
    auto blit=[&](bool hero) {
      m.bus.Write(0x8daa,0x1a);m.bus.Write(0x8dab,0x8f);
      m.bus.Write(0x9692,hero?0x13:0x03);m.bus.Write(0x9691,3);
      m.cpu.regs.PC=0x8e1d;m.cpu.regs.HL=0x86d5;m.cpu.regs.BC=32<<8;
      m.cpu.regs.altBC=0xa000;m.cpu.regs.altDE=0xab00;m.cpu.regs.altHL=0xac00;
      m.instructionObserver(m.observationContext,m);
      for(unsigned y=0;y<32;++y)for(unsigned x=0;x<3;++x) {
        unsigned a=y*3+x,target=0xa000+y*6+x;
        m.bus.Write(target,(m.bus.Read(target)&m.bus.Read(0xac00+a))|m.bus.Read(0xab00+a));
      }
    };
    blit(order==0);blit(order!=0);
    auto before=m.CaptureState();m.cpu.regs.PC=0x8d9a;m.instructionObserver(m.observationContext,m);
    assert(enhancement.Available(m));assert(m.CaptureState().memory.ram==before.memory.ram);
    const unsigned at=((24+8)*256+64+10)*4;
    const auto &rgb=enhancement.Pixels();
    unsigned colour=unsigned(rgb[at])<<16|unsigned(rgb[at+1])<<8|rgb[at+2];
    assert((colour==0xc9bca1)==(order==0));
    unsigned shadow=unsigned(rgb[at+4])<<16|unsigned(rgb[at+5])<<8|rgb[at+6];
    assert((shadow==0x39424b)==(order==0));
    if(!variation){overlapColours[order][0]=colour;overlapColours[order][1]=shadow;}
    else{assert(overlapColours[order][0]==colour);assert(overlapColours[order][1]==shadow);}
  }
  // Initialise each checked room through the game's own room-entry routine.
  // Fixtures choose a valid idle spawn, rather than carrying a jumping pose
  // from the earlier movement checkpoint into a different room's walls.
  const unsigned rooms[][5]={{0x8a40,32,8,48,72},{0x8940,16,16,64,64},
    {0x9a40,8,24,72,56},{0x8840,24,8,56,72},{0x9940,8,24,72,56},
    {0x8a30,16,16,64,64}};
  for(const auto &room:rooms) {
    assert(BatmanColourEnhancement::SupportedRoom(room[0]));
    assert(m.RestoreState(checkpoint));enhancement.Attach(m);
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<10;++c)m.ula.PressKey(r,c,false);
    m.bus.Write(0x658b,room[0]&255);m.bus.Write(0x658c,room[0]>>8);
    m.bus.Write(0x9b06,0);m.bus.Write(0x8f0a,0);m.bus.Write(0x8f0f,0);
    const unsigned spawnX=(room[1]+room[3])/2,spawnY=(room[2]+room[4])/2;
    for(unsigned base:{0x8f15u,0x8f1du}) {
      m.bus.Write(base,spawnX);m.bus.Write(base+1,spawnY);m.bus.Write(base+2,0xc0);
    }
    m.cpu.regs.PC=0x65b3;m.cpu.regs.SP=0xfff4;
    const auto entry=m.CaptureState();
    unsigned available=0;
    for(unsigned f=0;f<100;++f){m.Frame();available+=enhancement.Available(m)?1:0;}
    assert(available>30 && enhancement.Available(m));
    assert(enhancement.RejectedRegions()==0);
    auto colouredRun=m.CaptureState();
    const auto roomPixels=enhancement.Pixels();
    assert(m.RestoreState(entry));m.instructionObserver=nullptr;
    for(unsigned f=0;f<100;++f)m.Frame();auto originalRun=m.CaptureState();
    assert(colouredRun.memory.ram==originalRun.memory.ram);
    assert(memcmp(&colouredRun.registers,&originalRun.registers,sizeof(colouredRun.registers))==0);
    assert(colouredRun.ula.screenRGB==originalRun.ula.screenRGB);
    assert(memcmp(colouredRun.ula.frameAudio,originalRun.ula.frameAudio,sizeof(originalRun.ula.frameAudio))==0);
    m.ula.SetBitmap(bitmap);assert(m.ula.Present());char path[120];
    snprintf(path,sizeof(path),"/tmp/batman-room-%04x-original.png",room[0]);assert(al_save_bitmap(path,bitmap));
    assert(enhancement.Present(m,bitmap));
    snprintf(path,sizeof(path),"/tmp/batman-room-%04x-enhanced.png",room[0]);assert(al_save_bitmap(path,bitmap));
    auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);assert(lock);
    for(int y=0;y<160;++y)for(unsigned x=0;x<256;++x)if(BatmanColourEnhancement::PaintsPixel(x,y))
      assert(memcmp(static_cast<unsigned char *>(lock->data)+(y+24)*lock->pitch+(x+32)*4,
        roomPixels.data()+(y*256+x)*4,4)==0);
    al_unlock_bitmap(bitmap);
    printf("Batman room %04x: %u enhanced frames, %u regions; execution unchanged\n",
      room[0],available,enhancement.CapturedRegions());
    // Changing to another checked room invalidates the previous room's pixels
    // immediately, even for the pair with identical dimensions.
    unsigned next=room[0]==0x8940?0x8a30:0x8940;
    m.bus.Write(0x658b,next&255);m.bus.Write(0x658c,next>>8);
    assert(!enhancement.Available(m));
    enhancement.Attach(m);assert(!enhancement.Available(m));
  }
  // Exercise the game's actual room-exit dispatcher in both directions:
  // west from the opening hall into 8940, then back east into 8A40.
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned r=0;r<4;++r)for(unsigned c=0;c<10;++c)m.ula.PressKey(r,c,false);
  for(unsigned n=0;n<8;++n)m.Frame();assert(enhancement.Available(m));
  for(unsigned direction:{2u,4u}) {
    const unsigned targetRoom=direction==2?0x8940:0x8a40;
    m.bus.Write(0x9b05,direction);m.cpu.regs.PC=0x6603;m.cpu.regs.SP=0xfff4;
    unsigned seen=0;
    for(unsigned n=0;n<100;++n){m.Frame();seen+=enhancement.Available(m)?1:0;}
    assert((m.bus.Read(0x658b)+256*m.bus.Read(0x658c))==targetRoom);
    assert(seen>20 && enhancement.Available(m) && enhancement.RejectedRegions()==0);
    printf("Batman room-exit dispatcher: entered %04x, %u enhanced frames\n",targetRoom,seen);
  }
  assert(!BatmanColourEnhancement::SupportedRoom(0x8a41));
  assert(m.RestoreState(checkpoint));enhancement.Attach(m);
  for(unsigned n=0;n<8;++n)m.Frame();assert(enhancement.Available(m));
  enhancement.enabled=false;assert(!enhancement.Available(m));enhancement.enabled=true;
  m.bus.Write(0x658b,0x41);assert(!enhancement.Available(m)); // Unchecked room.
  m.bus.Write(0x658b,0x40);m.bus.Write(0x9b06,5);assert(!enhancement.Available(m)); // Death.
  assert(m.RestoreState(checkpoint));
  // Original/coloured atlas from the actual source art, including reflected
  // drawings. The production material maps do not replace any source bits.
  int flags=al_get_new_bitmap_flags();al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  auto *sheet=al_create_bitmap(576,384);assert(sheet);al_set_new_bitmap_flags(flags);
  auto *target=al_get_target_bitmap();al_set_target_bitmap(sheet);
  for(unsigned n=0;n<16;++n)for(unsigned y=0;y<32;++y)for(unsigned x=0;x<24;++x) {
    const unsigned pose=n%8,localX=n<8?x:23-x;
    const bool cachedMirror=m.bus.Read(0xa840+(16+pose)/8)&(128>>(pose&7));
    const unsigned sx=cachedMirror?23-localX:localX;
    const unsigned source=0xc530+pose*192+y*3+sx/8,bit=128>>(sx&7);
    const bool ink=m.bus.Read(source)&bit,transparent=(m.bus.Read(source+96)&bit) && !ink;
    unsigned rgb=BatmanMaterials::Colour(pose+16,localX,y,ink);
    const ALLEGRO_COLOR normal=transparent?al_map_rgb(30,30,42):ink?al_map_rgb(215,215,215):al_map_rgb(0,0,0);
    const ALLEGRO_COLOR coloured=transparent?al_map_rgb(30,30,42):al_map_rgb(rgb>>16,(rgb>>8)&255,rgb&255);
    for(unsigned yy=0;yy<3;++yy)for(unsigned xx=0;xx<3;++xx) {
      unsigned dx=(n%4)*144+x*3+xx,dy=(n/4)*96+y*3+yy;
      al_put_pixel(dx,dy,normal);al_put_pixel(dx+72,dy,coloured);
    }
  }
  assert(al_save_bitmap("/tmp/batman-sprite-comparison.png",sheet));
  al_set_target_bitmap(target);al_destroy_bitmap(sheet);
  GameColourEnhancement selector;selector.enabled=true;selector.Attach(m);
  assert(m.observationPC==0x8e1d && m.observationPC2==0x8d9a);
  assert(m.Open("army.tap",error));selector.Prepare(m);
  assert(m.observationPC==0xbb6b && m.observationPC2==0xcf56);
  assert(!enhancement.Available(m) && !BatmanColourEnhancement::Matches(m));
  al_destroy_bitmap(bitmap);if(display)al_destroy_display(display);
  puts("Batman colour enhancement checks passed");
}
