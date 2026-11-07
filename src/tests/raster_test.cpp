#include <cassert>
#include <cstdio>
#include "../emulator.h"

static void Write(ULA &u,unsigned address,unsigned char value) {
  static_cast<BusComponentBase *>(static_cast<ULAMemory *>(&u))->Write(address,value);
}
static unsigned Pixel(const ULA &u,unsigned x,unsigned y) {
  auto rgb=u.CaptureRGB();auto p=&rgb[((y+24)*320+x+32)*3];return unsigned(p[0])<<16|unsigned(p[1])<<8|p[2];
}
static void At(ULA &u,unsigned target) {
  auto now=u.CaptureState().frameTStates;assert(target>=now);bool irq=false;u.AddCycles(target-now,irq);
}
int main() {
  // Erase after the beam: what was scanned must survive until the next frame.
  ULA u;bool irq=false;Write(u,0x5800,7);Write(u,0x4000,0xff);
  unsigned first=u.Timing().firstDisplayTstate;
  At(u,first+1);assert(Pixel(u,0,0)==0xd7d7d7);
  Write(u,0x4000,0);assert(Pixel(u,0,0)==0xd7d7d7);
  // A new attribute changes future scanlines, not the previous line of a cell.
  Write(u,0x5800,2);Write(u,0x4100,0xff);
  At(u,first+u.Timing().tstatesPerLine+1);
  assert(Pixel(u,0,0)==0xd7d7d7 && Pixel(u,0,1)==0xd70000);
  // A write before a later column is scanned is visible in the same row.
  Write(u,0x5801,4);Write(u,0x4101,0xff);
  At(u,first+u.Timing().tstatesPerLine+5);assert(Pixel(u,8,1)==0x00d700);
  auto saved=u.CaptureState();auto scanned=u.CaptureRGB();
  Write(u,0x4100,0);u.AddCycles(u.Timing().TStatesPerFrame(),irq);
  u.RestoreState(saved);assert(u.CaptureRGB()==scanned);
  At(u,u.Timing().TStatesPerFrame());At(u,first+1);assert(Pixel(u,0,0)==0);

  // FLASH changes the next scan, without repainting the completed frame.
  ULA flash;Write(flash,0x5800,0x87);Write(flash,0x4000,0xff);
  flash.AddCycles(flash.Timing().TStatesPerFrame()*16,irq);
  assert(Pixel(flash,0,0)==0xd7d7d7);
  flash.AddCycles(flash.Timing().TStatesPerFrame(),irq);assert(Pixel(flash,0,0)==0);

  // CPU mapping and displayed bank are independent, including a mid-row swap.
  ULA banked;SpectrumMemory memory(banked);SpectrumIO io(banked,memory);ROMSet rom;std::string error;
  assert(LoadROMSet(FindROM("128-0.rom"),rom,error));banked.Configure(rom.model);assert(memory.Configure(rom,error));
  memory.Write(0x5800,2);memory.Write(0x4000,0xff);memory.Write(0x5801,2);memory.Write(0x4001,0xff);
  io.Write(0x7ffd,7);memory.Write(0xd800,4);memory.Write(0xc000,0xff);memory.Write(0xd801,4);memory.Write(0xc001,0xff);
  first=banked.Timing().firstDisplayTstate;At(banked,first+1);assert(Pixel(banked,0,0)==0xd70000);
  io.Write(0x7ffd,8);assert(Pixel(banked,0,0)==0xd70000);
  At(banked,first+5);assert(Pixel(banked,0,0)==0xd70000 && Pixel(banked,8,0)==0x00d700);

  // Splitting device time into instructions must not alter the raster or FLASH.
  for(auto model:{MachineModel::Spectrum48,MachineModel::Spectrum128,MachineModel::Plus2,MachineModel::Plus3}) {
    ULA whole,split;whole.Configure(model);split.Configure(model);
    for(unsigned a=0;a<6912;++a){Write(whole,0x4000+a,a*17);Write(split,0x4000+a,a*17);}
    unsigned frame=whole.Timing().TStatesPerFrame();whole.AddCycles(frame*17,irq);
    for(unsigned left=frame*17;left;){unsigned step=std::min(left,17u);split.AddCycles(step,irq);left-=step;}
    assert(whole.CaptureRGB()==split.CaptureRGB());
  }
  puts("Raster: timed pixels/attributes, erase after scan, mid-row shadow switch, state restoration and split timing passed");
}
