#ifndef MONITOR_SYNC_H
#define MONITOR_SYNC_H
#include <algorithm>
#include <cmath>
#include <deque>
#include <vector>
#include "playback.h"

// Optional display-clocked playback. All emulated devices retain their model
// clocks; only the wall-clock duration of a complete frame changes.
class MonitorSync {
public:
  struct Plan {double refresh=0,fps=0,ratio=1;unsigned interval=0;bool active=false;};
  static Plan Choose(double refresh,const MachineTiming &timing) {
    Plan p;p.refresh=refresh;
    if(!std::isfinite(refresh) || refresh<20 || refresh>1000)return p;
    const double native=double(timing.cpuClockHz)/timing.TStatesPerFrame();
    p.interval=std::max(1u,unsigned(std::lround(refresh/native)));
    p.fps=refresh/p.interval;p.ratio=p.fps/native;
    p.active=std::abs(p.ratio-1)<=0.05;return p;
  }
  void Configure(Plan value) {plan=value;Reset();}
  void Reset() {audio.clear();samplePhase=errorAverage=0;hold=0;}
  const Plan &Configuration() const {return plan;}
  size_t Buffered() const {return audio.size();}
  void Read(short *output,size_t count) {
    while(count--){*output++=audio.empty()?0:audio.front();if(!audio.empty())audio.pop_front();}
  }
  void Tick(SpectrumMachine &machine,const Playback &playback) {
    if(!plan.active || playback.paused || playback.Rewinding())return;
    if(hold){--hold;return;}
    const double speed=playback.Speed();unsigned repeat=speed<1?unsigned(1/speed):1;
    const unsigned frames=speed<1?1:unsigned(speed);hold=repeat-1;
    std::vector<short> source;source.reserve(frames*SAMPLES_PER_FRAME);
    machine.DiscardAudio();
    for(unsigned n=0;n<frames;++n){machine.Frame();source.insert(source.end(),machine.ula.FrameAudio,machine.ula.FrameAudio+machine.ula.FrameAudioCount);}
    if(source.empty())return;
    const double nominal=double(AUDIO_SAMPLE_RATE)*repeat/plan.fps;
    // A small, filtered rate correction absorbs physical audio/display clock
    // drift without changing how many machine frames each presentation runs.
    errorAverage+=0.02*(double(audio.size())-nominal/2-errorAverage);
    const double correction=std::max(-0.005,std::min(0.005,errorAverage/(AUDIO_SAMPLE_RATE*2)));
    samplePhase+=nominal*(1-correction);const unsigned count=unsigned(samplePhase);samplePhase-=count;
    const auto samples=Resample(source,count);
    audio.insert(audio.end(),samples.begin(),samples.end());
    // Bound buffering after a stall/device interruption; never accumulate an
    // indefinitely increasing latency. Normal playback uses far less than this.
    while(audio.size()>AUDIO_SAMPLE_RATE/4)audio.pop_front();
  }
  static std::vector<short> Resample(const std::vector<short> &input,unsigned count) {
    std::vector<short> out(count);if(input.empty() || !count)return out;
    const double scale=double(input.size())/count;
    for(unsigned n=0;n<count;++n) {
      const double end=(n+1)*scale;double at=n*scale,sum=0;
      while(at<end) {
        const size_t index=std::min(input.size()-1,size_t(at));
        const double next=std::min(end,double(index+1));
        if(next<=at)break;
        sum+=input[index]*(next-at);at=next;
      }
      out[n]=short(std::max(-32768L,std::min(32767L,std::lround(sum/scale))));
    }
    return out;
  }
private:
  Plan plan;std::deque<short> audio;double samplePhase=0,errorAverage=0;unsigned hold=0;
};
// Some window-system drivers report a retrace wait as successful without
// blocking. Detect that case and retain absolute deadlines instead of running
// the machine faster by the early-wakeup guard on every frame.
class MonitorPacer {
public:
  void Reset(double now,const MonitorSync::Plan &p) {
    period=p.active?1.0/p.fps:0.02;guard=p.active?0.5/p.refresh:0;
    next=now+period;canWait=true;
  }
  bool CanWait() const {return canWait;}
  void WaitResult(double now,bool success) {
    if(!success || now<next-0.001)canWait=false;
  }
  double Remaining(double now) const {return std::max(0.0,next-now);}
  void Presented(double now) {
    next=canWait?now+period:next+period;
    if(next<=now)next=now+period;
  }
  double Delay(double now) const {return std::max(0.001,next-now-(canWait?guard:0));}
private:
  double next=0,period=0.02,guard=0;bool canWait=true;
};
#endif
