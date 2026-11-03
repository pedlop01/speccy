#include <cassert>
#include <cstdio>
#include "../debug_view.h"
#include "../save_state.h"
static std::vector<unsigned char> Core(const SpectrumMachine &m){SaveStates::Writer w;SaveStates::WriteCore(w,m.CaptureState());return w.bytes;}
static void Loop(SpectrumMachine &m){
  m.cpu.regs.PC=0x8000;m.cpu.regs.SP=0xbfff;
  const unsigned char code[]={0x03,0x3e,0x10,0xd3,0xfe,0xc3,0,0x80};
  for(unsigned n=0;n<sizeof(code);++n)m.bus.Write(0x8000+n,code[n]);
}
int main(){
  assert(al_init());al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);std::string error;
  for(const char *rom:{"48.rom","128-0.rom","plus2-0.rom","plus3-0.rom"}){
    SpectrumMachine normal,step;
    assert(normal.LoadROM(FindROM(rom),error) && step.LoadROM(FindROM(rom),error));
    assert(normal.Open("cobra.tap",error) && step.Open("cobra.tap",error));
    normal.SetFastLoad(false);step.SetFastLoad(false);assert(normal.PlayTape() && step.PlayTape());
    for(auto *m:{&normal,&step}){Loop(*m);m->EnableRewind();m->SetCPUMultiplier(2);if(m->ula.ay.Enabled()){
      m->io.Write(0xfffd,0);m->io.Write(0xbffd,17);m->io.Write(0xfffd,8);m->io.Write(0xbffd,15);
    }}
    if(step.Configuration().model==MachineModel::Plus3)for(auto *m:{&normal,&step}){
      Disks::Image disk;disk.cylinders=1;disk.heads=1;disk.tracks.resize(1);disk.tracks[0].sectors.push_back({0,0,1,2,0,0,std::vector<unsigned char>(512,0xa5)});
      m->io.floppy.Insert(0,disk,"debug-test.dsk");m->io.Write(0x1ffd,8);
      for(unsigned char byte:{0x46,0,0,0,1,2,1,0x2a,0xff})m->io.Write(0x3ffd,byte);
      assert(m->io.floppy.state.ready==114);
    }
    assert(step.StepInstruction()==6 && step.cpu.regs.PC==0x8001 && step.cpu.regs.BC==1);
    // Start the reference at the same instruction and compare an exact frame.
    if(step.Configuration().model==MachineModel::Plus3)assert(step.io.floppy.state.ready==111);
    assert(normal.StepInstruction()==6);assert(Core(normal)==Core(step));
    for(unsigned frame=0;frame<20;++frame){auto target=step.FrameCounter()+1;normal.Frame();unsigned count=0;
      while(step.FrameCounter()<target){assert(step.StepInstruction()>0);assert(++count<100000);}
      assert(Core(normal)==Core(step));
    }
    // CPU and physical RAM views do not page memory or consume peripheral data.
    auto before=Core(step);for(unsigned address=0;address<65536;++address)assert(DebugView::ReadByte(step,-1,address)==step.memory.Read(address));
    for(unsigned bank=0;bank<step.Configuration().memory.ramBanks;++bank)for(unsigned address=0;address<16384;address+=37)
      assert(DebugView::ReadByte(step,bank,address)==step.memory.ReadBank(bank,address));
    assert(Core(step)==before);
    // Continue from a partial frame and compare subsequent audio exactly.
    for(unsigned n=0;n<7;++n){normal.StepInstruction();step.StepInstruction();}
    std::vector<short>a(13007),b(a.size());normal.ReadAudio(a.data(),a.size());step.ReadAudio(b.data(),b.size());assert(a==b && Core(normal)==Core(step));
    assert(step.CanRewind() && step.RewindOneSecond());
  }
  // Interrupt acceptance at the frame boundary is included in the step path.
  SpectrumMachine irq;assert(irq.LoadROM(FindROM("48.rom"),error));Loop(irq);irq.cpu.regs.iff1A=irq.cpu.regs.iff1B=1;irq.cpu.regs.IM=1;
  bool frame=false;irq.Advance(irq.Configuration().timing.TStatesPerFrame()-2,frame);assert(!frame);
  assert(irq.StepInstruction()==19 && irq.FrameCounter()==1 && irq.cpu.regs.PC==0x38);
  puts("Debugger: single instructions, frame/IRQ timing, overclock, tape/AY/audio, side-effect-free RAM inspection and rewind passed");
}
