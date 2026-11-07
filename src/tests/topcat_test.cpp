// Optional integration test: supply the user's installed TAP; it is not bundled.
#include <cassert>
#include <cstdio>
#include "../catalog_load.h"
int main(int argc,char **argv) {
  if(argc!=2){fprintf(stderr,"Usage: topcat-tests /path/to/Top_Cat_in_Beverly_Hills_Cats.tap\n");return 2;}
  assert(al_init());auto bytes=GameCatalog::Read(argv[1],TapeFiles::MaxBytes);
  if(GameCatalog::MD5(bytes)!="2c6da90b3f5de817ad112fcbcff4d259") {
    fprintf(stderr,"This regression uses ZXArt's 48K TAP crack, MD5 2c6da90b3f5de817ad112fcbcff4d259\n");return 2;
  }
  SpectrumMachine m;std::string error;
  if(!GameCatalog::Load(m,argv[1],MachineModel::Spectrum48,error)){fprintf(stderr,"%s\n",error.c_str());return 1;}
  m.io.kempstonEnabled=true;int loaded=-1;unsigned checked=0,minimum=1200;
  for(int frame=0;frame<4000;++frame) {
    m.Frame();
    if(loaded<0 && m.deck.CurrentBlock()==m.deck.Blocks().size())loaded=frame;
    if(loaded<0)continue;int n=frame-loaded;
    // Select Kempston, continue, fire to start, then walk across the courtyard.
    if(n==550)m.ula.PressKey(0,2,true);if(n==560)m.ula.PressKey(0,2,false);
    if(n==600)m.ula.PressKey(0,0,true);if(n==610)m.ula.PressKey(0,0,false);
    if(n==700)m.io.kempstonState=16;if(n==710)m.io.kempstonState=0;
    if(n==800)m.io.kempstonState=1;
    if(n>=900 && n<916) {
      auto rgb=m.ula.CaptureRGB();unsigned black=0;
      // This region contains the walking player. Erased frames have only
      // 116/117 background pixels here; complete poses have at least 240.
      for(unsigned y=120;y<150;++y)for(unsigned x=156;x<196;++x) {
        auto p=&rgb[(y*320+x)*3];if(!p[0] && !p[1] && !p[2])++black;
      }
      minimum=std::min(minimum,black);++checked;
      if(black<=200){fprintf(stderr,"Top Cat: missing/partial sprite at gameplay frame %d (%u pixels)\n",n,black);return 1;}
    }
    if(n==916)break;
  }
  assert(checked==16);
  printf("Top Cat 48K: all 16 consecutive walking poses visible (minimum %u pixels); no alternating erased frames\n",minimum);
}
