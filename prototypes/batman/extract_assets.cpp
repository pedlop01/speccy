// One-time local asset extraction. The Python game never executes Z80 code.
#include <fstream>
#include <map>
#include <stdexcept>
#include <allegro5/allegro_image.h>
#include "../../src/emulator.h"
#include "batman_materials.h"
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
using nlohmann::json;
namespace {
const unsigned W=256,H=160;
struct Layer {
  std::array<unsigned char,W*H> ink{},opaque{};
  unsigned x=0,y=0,z=0,drawing=0,options=0;
};
struct Capture {
  Layer base,reference,referenceHeroMask;std::map<unsigned,Layer> objects;
  bool pending=false;unsigned regions=0;
  static unsigned word(SpectrumMachine &m,unsigned a){return m.bus.Read(a)+256*m.bus.Read(a+1);}
  void observe(SpectrumMachine &m) {
    if(word(m,0x658b)!=0x8a40 || m.bus.Read(0x6a41)!=32 || m.bus.Read(0x6a44)!=72)return;
    const unsigned left=m.bus.Read(0x8cd2),right=m.bus.Read(0x8cd1),top=m.bus.Read(0x8cd4),bottom=m.bus.Read(0x8cd3);
    if(left<64 || right>192 || right<=left || top<72 || bottom<=top || bottom-top>42 || (right-left+3)/4>6)return;
    if(!pending) {
      for(unsigned y=0;y<bottom-top;++y)for(unsigned b=0;b<(right-left+3)/4;++b)for(unsigned bit=0;bit<8;++bit){
        unsigned x=(left-64)*2+b*8+bit,sy=top-72+y;
        if(x<W && sy<H){base.opaque[sy*W+x]=1;base.ink[sy*W+x]=(m.bus.Read(0xa000+y*6+b)&(128>>bit))!=0;}
      }
      pending=true;
    }
    if(m.cpu.regs.PC==0x8d9a){
      for(unsigned y=0;y<bottom-top;++y)for(unsigned b=0;b<(right-left+3)/4;++b)for(unsigned bit=0;bit<8;++bit){
        unsigned x=(left-64)*2+b*8+bit,sy=top-72+y;
        if(x<W && sy<H){reference.opaque[sy*W+x]=1;reference.ink[sy*W+x]=(m.bus.Read(0xa000+y*6+b)&(128>>bit))!=0;}
      }
      pending=false;++regions;return;
    }
    unsigned p=word(m,0x8daa),drawing=m.bus.Read(0x9692)&127;
    const bool hero=drawing>=0x10 && drawing<=0x17;
    auto &layer=hero?referenceHeroMask:objects[p];layer.x=m.bus.Read(p+3);layer.y=m.bus.Read(p+4);layer.z=m.bus.Read(p+5);
    layer.drawing=drawing;layer.options=m.bus.Read(p+7);
    unsigned stride=m.bus.Read(0x9691),width=0,height=m.cpu.regs.B,dest=m.cpu.regs.altBC;
    if(stride<3 || stride>5)throw std::runtime_error("Unexpected blit stride");
    unsigned table=word(m,0x8e1e + 2*(stride-3));
    for(unsigned n=0;n<stride;++n)if(word(m,table+n*2)==m.cpu.regs.HL)width=n+1;
    if(!width || height>42 || dest<0xa000 || dest>=0xa100)throw std::runtime_error("Unexpected blit bounds");
    for(unsigned y=0;y<height;++y)for(unsigned b=0;b<width;++b)for(unsigned bit=0;bit<8;++bit){
      unsigned at=((dest&255)+y*6+b)&255,x=(left-64)*2+(at%6)*8+bit,sy=top-72+at/6;
      unsigned ink=m.bus.Read(m.cpu.regs.altDE+y*stride+b),mask=m.bus.Read(m.cpu.regs.altHL+y*stride+b);
      if(x<W && sy<H && (!(mask&(128>>bit)) || (ink&(128>>bit)))) {
        layer.opaque[sy*W+x]=1;layer.ink[sy*W+x]=(ink&(128>>bit))!=0;
      }
    }
  }
};
unsigned scenery(unsigned x,unsigned y,bool ink){
  int a=238+int(x)-2*int(y),b=494-int(x)-2*int(y);
  if(a>=128 && a<=192 && b>=32 && b<=288)return ink?0xc2b18d:0x34434a;
  return ink?0x9daebc:0x0b1320;
}
void pixel(unsigned x,unsigned y,unsigned rgb,unsigned alpha=255){al_put_pixel(x,y,al_map_rgba(rgb>>16,(rgb>>8)&255,rgb&255,alpha));}
void saveLayer(const Layer &layer,const std::string &path,bool original,bool base){
  auto *image=al_create_bitmap(W,H);if(!image)throw std::runtime_error("Cannot create extraction bitmap");
  al_set_target_bitmap(image);al_clear_to_color(al_map_rgba(0,0,0,0));
  for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x){unsigned at=y*W+x;
    if(base || layer.opaque[at]){bool ink=layer.ink[at];unsigned rgb;
      if(original)rgb=ink?0xd7d7d7:0;
      else if(base)rgb=scenery(x,y,ink);
      else if(layer.drawing==0x5f)rgb=ink?0xc3b59a:0x49463e;
      else rgb=ink?0xc9bca1:0x39424b;
      pixel(x,y,rgb);
    }
  }
  if(!al_save_bitmap(path.c_str(),image))throw std::runtime_error("Cannot save "+path);al_destroy_bitmap(image);
}
}
int main(int argc,char **argv)try{
  if(argc!=3){fprintf(stderr,"usage: extract_assets TAPE OUTPUT_DIRECTORY (run from bin/)\n");return 2;}
  if(!al_init() || !al_init_image_addon())throw std::runtime_error("Allegro initialisation failed");
  al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
  SpectrumMachine m;std::string error;
  if(!m.LoadROM(FindROM("48.rom"),error)||!m.Open(argv[1],error))throw std::runtime_error(error);
  unsigned char digest[SHA256_DIGEST_LENGTH];SHA256(m.tape.data(),m.tape.size(),digest);
  const unsigned char expected[]={0xc0,0x61,0x9e,0x7f,0x31,0x04,0x75,0x63,
    0xbf,0xb9,0x1b,0xa5,0x04,0xe0,0x02,0xb1,0xef,0x09,0xc9,0x95,0x5a,0x11,
    0x47,0xb5,0xbb,0x4e,0x10,0xff,0x59,0x51,0xc4,0x27};
  if(memcmp(digest,expected,sizeof(expected)))throw std::runtime_error("This extractor requires the checked bin/batman.tap release");
  for(int f=0;f<2500;++f){auto key=[&](int when,int row,int col){if(f==when)m.ula.PressKey(row,col,true);if(f==when+10)m.ula.PressKey(row,col,false);};
    key(500,2,6);if(f==520||f==540)m.ula.PressKey(3,8,true);key(520,1,9);key(540,1,9);
    if(f==550)m.ula.PressKey(3,8,false);key(560,2,9);key(1000,2,9);key(1200,2,9);key(2000,2,9);key(2400,2,9);m.Frame();
  }
  for(unsigned r=0;r<4;++r)for(unsigned c=0;c<10;++c)m.ula.PressKey(r,c,false);
  m.bus.Write(0x9b06,0);m.bus.Write(0x8f0a,0);m.bus.Write(0x8f0f,0);
  for(unsigned a:{0x8f15u,0x8f1du}){m.bus.Write(a,40);m.bus.Write(a+1,64);m.bus.Write(a+2,192);}
  m.cpu.regs.PC=0x65b3;m.cpu.regs.SP=0xfff4;
  // Observe before each normal instruction, without requiring the colour
  // enhancement branch's observer hooks or changing the shared machine core.
  Capture capture;
  for(unsigned f=0;f<100;++f){
    const auto frame=m.FrameCounter();
    do{
      if(m.cpu.regs.PC==0x8e1d || m.cpu.regs.PC==0x8d9a)capture.observe(m);
      m.StepInstruction();
    }while(m.FrameCounter()==frame);
  }
  if(capture.regions<20 || capture.objects.empty())throw std::runtime_error("Opening room did not initialise");
  const std::string dir=argv[2];
  saveLayer(capture.base,dir+"/room.png",false,true);saveLayer(capture.base,dir+"/room-original.png",true,true);
  saveLayer(capture.reference,dir+"/reference-original.png",true,false);
  saveLayer(capture.referenceHeroMask,dir+"/reference-hero-mask.png",true,false);
  json manifest={{"format",1},{"room","8a40"},{"tape_sha256","c0619e7f31047563bfb91ba504e002b1ef09c9955a1147b5bb4e10ff5951c427"},
    {"bounds",{32,8,48,72}},{"spawn",{40,64}},
    {"objects",json::array()}};
  for(const auto &entry:capture.objects){
    const auto &l=entry.second;std::string name="object-"+std::to_string(entry.first);
    saveLayer(l,dir+"/"+name+".png",false,false);saveLayer(l,dir+"/"+name+"-original.png",true,false);
    manifest["objects"].push_back({{"name",name},{"x",l.x},{"y",l.y},{"z",l.z},{"drawing",l.drawing},{"options",l.options}});
  }
  for(unsigned pose=0;pose<8;++pose)for(unsigned original=0;original<2;++original){
    auto *image=al_create_bitmap(24,32);if(!image)throw std::runtime_error("Cannot create sprite bitmap");al_set_target_bitmap(image);al_clear_to_color(al_map_rgba(0,0,0,0));
    bool reflected=m.bus.Read(0xa840+(16+pose)/8)&(128>>(pose&7));
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<24;++x){unsigned sx=reflected?23-x:x,at=0xc530+pose*192+y*3+sx/8,bit=128>>(sx&7);
      bool ink=m.bus.Read(at)&bit,transparent=(m.bus.Read(at+96)&bit)&&!ink;
      if(!transparent)pixel(x,y,original?(ink?0xd7d7d7:0):BatmanMaterials::Colour(16+pose,x,y,ink));
    }
    std::string path=dir+"/hero-"+std::to_string(pose)+(original?"-original":"")+".png";
    if(!al_save_bitmap(path.c_str(),image))throw std::runtime_error("Cannot save sprite");al_destroy_bitmap(image);
  }
  std::ofstream out(dir+"/room.json");out<<manifest.dump(2)<<'\n';if(!out)throw std::runtime_error("Cannot save manifest");
  printf("Extracted opening room: %zu scenery layers, eight Batman poses (%u regions).\n",capture.objects.size(),capture.regions);return 0;
}catch(const std::exception &e){fprintf(stderr,"Extraction failed: %s\n",e.what());return 1;}
