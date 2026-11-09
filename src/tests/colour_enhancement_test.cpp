#include <cassert>
#include <cstdio>
#include <set>
#include <allegro5/allegro_image.h>
#include "../colour_enhancement.h"

int main(int argc, char **) {
  assert(al_init());assert(al_init_image_addon());
  const bool video=argc>1;
  ALLEGRO_DISPLAY *display=nullptr;
  if(video){display=al_create_display(640,480);assert(display);}
  al_set_new_bitmap_flags(video?ALLEGRO_VIDEO_BITMAP:ALLEGRO_MEMORY_BITMAP);
  auto *bitmap=al_create_bitmap(320,240);assert(bitmap);
  auto *probe=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READWRITE);
  assert(probe);printf("Bitmap lock pitch: %d (%s)\n",probe->pitch,video?"video":"memory");fflush(stdout);
  al_unlock_bitmap(bitmap);
  SpectrumMachine m;std::string error;
  assert(m.LoadROM(FindROM("48.rom"),error));assert(m.Open("manic.tap",error));
  m.ula.SetBitmap(bitmap);
  ManicColourEnhancement enhancement;enhancement.enabled=true;enhancement.Attach(m);
  int loaded=-1;unsigned enhancedFrames=0;std::set<unsigned> positions;bool saved=false;
  for(int f=0;f<3500;++f) {
    auto key=[&](int when,unsigned row,unsigned col) {
      if(f==when)m.ula.PressKey(row,col,true);
      if(f==when+10)m.ula.PressKey(row,col,false);
    };
    key(500,2,6);
    if(f==520 || f==540)m.ula.PressKey(3,8,true);
    key(520,1,9);key(540,1,9);
    if(f==550)m.ula.PressKey(3,8,false);
    key(560,2,9);
    if(loaded>=0)key(loaded+200,2,9); // ENTER starts the original game.
    m.Frame();
    if(loaded<0 && m.deck.CurrentBlock()==m.deck.Blocks().size()) {
      loaded=f;printf("Manic loaded at %d\n",f);
    }
    if(!enhancement.Available(m))continue;
    ++enhancedFrames;
    positions.insert((m.bus.Read(32876)&31)*256+m.bus.Read(32872));
    // Verify presentation changes neither RAM, registers, audio nor ULA pixels.
    const auto before=m.CaptureState();
    assert(m.ula.Present());assert(enhancement.Present(m,bitmap));
    // Read back every enhanced row, including on bottom-up GPU locks.
    auto *read=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_READONLY);
    assert(read);
    for(int y=0;y<128;++y)
      assert(memcmp(static_cast<unsigned char *>(read->data)+(y+24)*read->pitch+32*4,
        enhancement.Pixels().data()+y*256*4,256*4)==0);
    al_unlock_bitmap(bitmap);
    const auto after=m.CaptureState();
    assert(before.memory.ram==after.memory.ram);
    assert(memcmp(&before.registers,&after.registers,sizeof(before.registers))==0);
    assert(before.ula.screenRGB==after.ula.screenRGB);
    assert(memcmp(before.ula.frameAudio,after.ula.frameAudio,sizeof(before.ula.frameAudio))==0);
    if(!saved && enhancedFrames>=40) {
      assert(al_save_bitmap("/tmp/manic-enhanced.png",bitmap));
      m.ula.SetBitmap(bitmap);assert(m.ula.Present());
      assert(al_save_bitmap("/tmp/manic-original.png",bitmap));saved=true;
    }
    // Walk right and jump using Manic Miner's own P / SPACE keys.
    m.ula.PressKey(1,9,enhancedFrames<90);
    m.ula.PressKey(3,9,enhancedFrames>20 && enhancedFrames<45);
    if(enhancedFrames==160)break;
  }
  printf("Enhanced frames: %u; positions: %zu; PC=%04x mode=%u cavern=%u\n",
    enhancedFrames,positions.size(),m.cpu.regs.PC,m.bus.Read(33882),m.bus.Read(33799));
  assert(enhancedFrames>=160 && positions.size()>4 && saved);
  // The observer must leave execution identical to execution without a hook.
  const auto checkpoint=m.CaptureState();
  for(unsigned n=0;n<30;++n)m.Frame();
  const auto observed=m.CaptureState();
  assert(m.RestoreState(checkpoint));
  m.instructionObserver=nullptr;
  for(unsigned n=0;n<30;++n)m.Frame();
  const auto plain=m.CaptureState();
  assert(observed.memory.ram==plain.memory.ram);
  assert(memcmp(&observed.registers,&plain.registers,sizeof(plain.registers))==0);
  assert(observed.ula.screenRGB==plain.ula.screenRGB);
  assert(memcmp(observed.ula.frameAudio,plain.ula.frameAudio,sizeof(plain.ula.frameAudio))==0);
  enhancement.Attach(m);
  // Controlled shared-cell regression: Willy, a guardian and scenery occupy
  // the same attribute cells. Their pixel colours must remain independent.
  assert(m.RestoreState(checkpoint));
  m.bus.Write(33799,0);m.bus.Write(33882,0);m.bus.Write(33880,0);
  m.bus.Write(32876,1);m.bus.Write(32872,0);m.bus.Write(32873,0);m.bus.Write(32874,0);
  for(unsigned n=0;n<32;++n){m.bus.Write(33280+n,0);m.bus.Write(33024+n,0);}
  m.bus.Write(33280,128);
  for(unsigned row=0;row<16;++row)m.bus.Write(33024+row*2,128);
  m.bus.Write(32958,3);m.bus.Write(32959,2);m.bus.Write(32960,92);
  m.bus.Write(32962,0);m.bus.Write(32965,255);
  for(unsigned n=0;n<512;++n){m.bus.Write(24064+n,14);m.bus.Write(0x5800+n,7);}
  for(unsigned n=0;n<4096;++n)m.bus.Write(0x4000+n,255);
  const auto fixture=m.CaptureState();
  m.instructionObserver(m.observationContext,m);
  assert(enhancement.Available(m));
  const auto &rgb=enhancement.Pixels();
  auto colour=[&](unsigned x,unsigned y=0) {
    const unsigned at=(y*256+x)*4;
    return unsigned(rgb[at])<<16|unsigned(rgb[at+1])<<8|rgb[at+2];
  };
  assert(colour(8)==0xffd34d); // Willy's cap.
  assert(colour(16)==0xeb7feb); // Guardian highlight inside Willy's attribute cells.
  assert(colour(16,6)==0xd700d7); // Original body hue.
  assert(colour(16,11)==0x40c8ff); // Contrasting lower detail.
  assert(colour(16,15)==0x536a90); // Steel-blue lower detail.
  assert(colour(17)==0xd7d700); // Scenery retains its own INK beside the guardian.
  assert(m.CaptureState().memory.ram==fixture.memory.ram);
  // A recognised crumbling tile receives independent highlight/body/shadow
  // colours even in a cell shared with a guardian. Its bitmap is unchanged.
  for(unsigned type=0;type<8;++type)m.bus.Write(32800+type*9,200+type);
  m.bus.Write(32818,14);
  for(unsigned n=0;n<4096;++n)m.bus.Write(0x7000+n,255);
  const auto tiled=m.CaptureState();
  m.instructionObserver(m.observationContext,m);
  assert(colour(17)==0xffd68c);
  assert(colour(17,4)==0xc6985e);
  assert(colour(17,7)==0x80603f);
  assert(colour(16,6)==0xd700d7); // Guardian remains above the scenery.
  assert(m.CaptureState().memory.ram==tiled.memory.ram);
  // Empty background pixels expose illustrated rock, recessed timber and
  // dark tunnel mouths; tile/sprite layers remain above that backdrop.
  m.bus.Write(32818,202);m.bus.Write(32800,14);
  for(unsigned cell=0;cell<512;++cell)m.bus.Write(0x5800+cell,14);
  for(unsigned n=0;n<4096;++n){m.bus.Write(0x4000+n,0);m.bus.Write(0x7000+n,0);}
  m.instructionObserver(m.observationContext,m);
  assert(colour(17)==0x304255); // Uneven overhead rock.
  assert(colour(78,80)==0x101d2c); // Distant tunnel interior.
  assert(colour(53,80)==0x4b4037); // Recessed timber post.
  assert(colour(130,80)==0x203044); // Rock facet between tunnels.
  const auto backdrop=enhancement.Pixels();
  m.instructionObserver(m.observationContext,m);
  assert(enhancement.Pixels()==backdrop); // Static, deterministic scenery.
  assert(m.RestoreState(checkpoint));
  // Initialise every cavern through the original game routine, then exercise
  // its actual movement/drawing loop with and without the observer.
  unsigned supported=0;
  for(unsigned room=0;room<20;++room) {
    assert(m.RestoreState(checkpoint));
    m.bus.Write(33799,room);m.cpu.regs.PC=34449;
    unsigned available=0;
    for(unsigned n=0;n<80;++n) {
      m.Frame();
      if(enhancement.Available(m)) {
        ++available;assert(m.ula.Present());assert(enhancement.Present(m,bitmap));
        if(n==40) {
          char path[80];snprintf(path,sizeof(path),"/tmp/manic-cavern-%02u.png",room+1);
          assert(al_save_bitmap(path,bitmap));
        }
      }
    }
    if(ManicColourEnhancement::SupportedCavern(room)){assert(available>10);++supported;}
    else assert(available==0);
    const auto start=m.CaptureState();
    for(unsigned n=0;n<20;++n)m.Frame();
    const auto withHook=m.CaptureState();
    assert(m.RestoreState(start));m.instructionObserver=nullptr;
    for(unsigned n=0;n<20;++n)m.Frame();
    const auto withoutHook=m.CaptureState();
    assert(withHook.memory.ram==withoutHook.memory.ram);
    assert(memcmp(&withHook.registers,&withoutHook.registers,sizeof(withHook.registers))==0);
    assert(withHook.ula.screenRGB==withoutHook.ula.screenRGB);
    assert(memcmp(withHook.ula.frameAudio,withoutHook.ula.frameAudio,sizeof(withHook.ula.frameAudio))==0);
    enhancement.Attach(m);
    printf("Cavern %u: %u enhanced frames; execution unchanged\n",room+1,available);
  }
  assert(supported==15);
  m.bus.Write(33799,4);assert(!enhancement.Available(m));
  m.bus.Write(33799,0);
  enhancement.enabled=false;assert(!enhancement.Available(m));
  m.ula.SetBitmap(bitmap);assert(m.ula.Present());
  enhancement.enabled=true;
  m.Reset();assert(!enhancement.Available(m));
  assert(m.Open("cobra.tap",error));
  m.cpu.regs.PC=34771;m.StepInstruction();assert(!enhancement.Available(m));
  al_destroy_bitmap(bitmap);if(display)al_destroy_display(display);al_uninstall_system();
  puts("Colour enhancement: real tape boot, movement/jump, presentation isolation, toggle and reset passed");
}
