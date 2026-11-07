#include <cassert>
#include <cstdio>
#include <limits>
#include "../monitor_sync.h"
int main() {
  for(auto model:{MachineModel::Spectrum48,MachineModel::Spectrum128,MachineModel::Plus2,MachineModel::Plus3}) {
    auto timing=HardwareFor(model).timing;
    for(double hz:{50.0,100.0,144.0,143.86,240.0}){auto p=MonitorSync::Choose(hz,timing);assert(p.active && p.interval && std::abs(p.ratio-1)<=0.05);assert(std::abs(p.fps*p.interval-hz)<1e-6);}
    assert(MonitorSync::Choose(144,timing).interval==3 && MonitorSync::Choose(144,timing).fps==48);
    for(double hz:{0.0,60.0,120.0,165.0,std::numeric_limits<double>::quiet_NaN()})assert(!MonitorSync::Choose(hz,timing).active);
  }
  for(unsigned count:{1u,7u,958u,1000u,4000u}) {
    auto out=MonitorSync::Resample(std::vector<short>(959,12345),count);
    assert(out.size()==count);for(auto x:out)assert(x==12345);
  }
  assert(MonitorSync::Resample({-1000,1000,-1000,1000},2)==std::vector<short>({0,0}));
  assert(MonitorSync::Resample({-32768,32767},2)==std::vector<short>({-32768,32767}));
  assert(MonitorSync::Resample({1,2},0).empty());
  auto plan=MonitorSync::Choose(144,MachineTimings::PAL48);MonitorPacer pacer;pacer.Reset(0,plan);
  assert(pacer.Delay(0)<1.0/plan.fps);
  const double deadline=1.0/plan.fps;
  pacer.WaitResult(deadline-0.003,true);assert(!pacer.CanWait() && pacer.Remaining(deadline-0.003)>0);
  pacer.Presented(deadline+0.001);assert(std::abs(pacer.Delay(deadline+0.001)-(deadline-0.001))<1e-8);
  pacer.Reset(0,plan);pacer.WaitResult(deadline,false);assert(!pacer.CanWait());
  pacer.Reset(0,plan);pacer.WaitResult(deadline-0.0002,true);assert(pacer.CanWait());
  pacer.Presented(deadline);assert(pacer.Delay(deadline)<deadline);
  assert(al_init());std::string error;
  for(const char *rom:{"48.rom","128-0.rom"})for(double speed:{0.25,0.5,1.0,2.0,4.0,8.0,16.0}) {
    SpectrumMachine m;assert(m.LoadROM(FindROM(rom),error));m.SetCPUMultiplier(2);
    Playback playback(m);assert(playback.SetSpeed(speed));MonitorSync sync;
    sync.Configure(MonitorSync::Choose(144,m.Configuration().timing));auto frame=m.FrameCounter();
    for(unsigned tick=0;tick<16;++tick){sync.Tick(m,playback);std::vector<short> samples(1000);sync.Read(samples.data(),samples.size());}
    assert(m.FrameCounter()-frame==unsigned(16*speed) && m.CPUMultiplier()==2);
    // Reading host audio cannot advance the CPU or move the tape ahead.
    frame=m.FrameCounter();std::vector<short> samples(48000);sync.Read(samples.data(),samples.size());assert(m.FrameCounter()==frame);
    playback.paused=true;sync.Reset();sync.Tick(m,playback);assert(sync.Buffered()==0 && m.FrameCounter()==frame);
    sync.Read(samples.data(),samples.size());for(auto x:samples)assert(x==0);
  }
  SpectrumMachine m;assert(m.LoadROM(FindROM("48.rom"),error));Playback playback(m);MonitorSync sync;
  sync.Configure(MonitorSync::Choose(144,m.Configuration().timing));
  // Simulate a DAC clock 0.1% faster than the display clock. The filtered
  // resampling correction should keep the FIFO bounded without changing FPS.
  unsigned empty=0;
  for(unsigned tick=0;tick<1500;++tick){sync.Tick(m,playback);if(tick>300 && sync.Buffered()<1001)++empty;std::vector<short> samples(1001);sync.Read(samples.data(),samples.size());}
  assert(m.FrameCounter()==1500 && empty<5 && sync.Buffered()<2000);
  for(unsigned tick=0;tick<50;++tick)sync.Tick(m,playback);assert(sync.Buffered()<=AUDIO_SAMPLE_RATE/4);
  puts("Monitor sync: refresh/divisors, 5% limit, resampling, 0.25x-16x, model clocks, pause/reset, drift and bounded buffering passed");
}
