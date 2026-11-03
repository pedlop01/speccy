#ifndef TZX_H
#define TZX_H
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <zlib.h>

namespace TZX {
inline std::string Text(const std::vector<unsigned char> &v,size_t start,size_t size) {
  std::string out;for(size_t n=start;n<start+size;++n){unsigned c=v[n];if(c<32)c=' ';if(c<128)out+=char(c);else {out+=char(0xc0|(c>>6));out+=char(0x80|(c&63));}}return out;
}
inline bool Signature(const std::vector<unsigned char> &v) {return v.size()>=8 && !memcmp(v.data(),"ZXTape!\x1a",8);}
struct Option {std::string label;int jump;};
struct Block {
  unsigned kind=0,pilot=0,count=0,sync1=0,sync2=0,zero=0,one=0,bits=8,pause=0,sample=0;
  size_t offset=0,start=0,length=0,sourceIndex=0;
  bool initial=false,stop=false,stop48=false;
  std::uint64_t duration=0;
  std::string label;
  std::vector<Option> options;
};
struct Reader {
  const std::vector<unsigned char> &v;size_t p;
  explicit Reader(const std::vector<unsigned char> &data,size_t pos=0):v(data),p(pos){}
  void need(size_t n) const {if(p>v.size() || n>v.size()-p)throw std::runtime_error("Truncated TZX block");}
  unsigned u8(){need(1);return v[p++];}
  unsigned u16(){unsigned a=u8();return a|(u8()<<8);}
  unsigned u24(){unsigned a=u16();return a|(u8()<<16);}
  std::uint32_t u32(){auto a=u16();return a|(std::uint32_t(u16())<<16);}
  void skip(size_t n){need(n);p+=n;}
};
// Absolute EAR levels also represent direct recordings and pauses without
// inserting artificial edges between consecutive TZX blocks.
template<class Emit> bool Pulses(const Block &b,const std::vector<unsigned char> &v,Emit emit) {
  bool level=b.initial;size_t pulses=0;
  auto pulse=[&](unsigned ticks,bool high) {
    if(!ticks || ++pulses>2*1024*1024)throw std::runtime_error("Invalid or oversized TZX pulse sequence");
    emit(ticks,high);
  };
  auto toggle=[&](unsigned ticks){pulse(ticks,level);level=!level;};
  if(b.stop) {pulse(1,level);return level;}
  if(b.kind==0x10 || b.kind==0x11 || b.kind==0x12) for(unsigned n=0;n<b.count;++n)toggle(b.pilot);
  if(b.kind==0x10 || b.kind==0x11) {if(b.sync1)toggle(b.sync1);if(b.sync2)toggle(b.sync2);}
  if(b.kind==0x13) {
    Reader r(v,b.start);for(unsigned n=0;n<b.count;++n)toggle(r.u16());
  }
  if(b.kind==0x10 || b.kind==0x11 || b.kind==0x14) {
    for(size_t n=0;n<b.length;++n)for(unsigned bit=0;bit<(n+1==b.length?b.bits:8);++bit) {
      unsigned ticks=(v[b.start+n]&(128>>bit))?b.one:b.zero;toggle(ticks);toggle(ticks);
    }
  }
  if(b.kind==0x15) {
    unsigned run=0;bool high=false;
    for(size_t n=0;n<b.length;++n)for(unsigned bit=0;bit<(n+1==b.length?b.bits:8);++bit) {
      bool next=(v[b.start+n]&(128>>bit))!=0;
      if(run && (next!=high || run>0xffffffffu-b.sample)) {pulse(run,high);run=0;}
      high=next;run+=b.sample;
    }
    if(run) {pulse(run,high);level=high;}
  }
  if(b.kind==0x18) {
    std::vector<unsigned char> encoded(v.begin()+b.start,v.begin()+b.start+b.length);
    if(b.zero==2) {
      std::vector<unsigned char> unpacked(size_t(b.count)*5+1);
      uLongf size=unpacked.size();uLong input=encoded.size();
      if(uncompress2(unpacked.data(),&size,encoded.data(),&input)!=Z_OK || input!=encoded.size())
        throw std::runtime_error("Invalid compressed TZX CSW stream");
      unpacked.resize(size);encoded.swap(unpacked);
    }
    Reader r(encoded);std::uint64_t phase=0;
    for(unsigned n=0;n<b.count;++n) {
      unsigned samples=r.u8();if(!samples)samples=r.u32();
      std::uint64_t units=std::uint64_t(samples)*3500000+phase,ticks=units/b.sample;phase=units%b.sample;
      if(!samples || !ticks || ticks>0xffffffffu)throw std::runtime_error("Invalid TZX CSW pulse");
      toggle(static_cast<unsigned>(ticks));
    }
    if(r.p!=encoded.size())throw std::runtime_error("TZX CSW pulse count mismatch");
  }
  if(b.kind==0x19) {
    Reader r(v,b.start);unsigned pilotSymbols=r.u32(),pilotWidth=r.u8(),pilotAlphabet=r.u8();
    unsigned dataSymbols=r.u32(),dataWidth=r.u8(),dataAlphabet=r.u8();
    if(pilotSymbols>2*1024*1024 || dataSymbols>2*1024*1024)throw std::runtime_error("Oversized TZX symbol stream");
    struct Symbol {unsigned flags;std::vector<unsigned> ticks;};
    auto table=[&](unsigned count,unsigned width) {
      std::vector<Symbol> symbols;
      for(unsigned n=0;n<count;++n) {
        Symbol symbol;symbol.flags=r.u8();if(symbol.flags>3)throw std::runtime_error("Invalid TZX symbol polarity");
        bool ended=false;
        for(unsigned p=0;p<width;++p){unsigned ticks=r.u16();if(!ticks)ended=true;if(!ended)symbol.ticks.push_back(ticks);}
        symbols.push_back(std::move(symbol));
      }return symbols;
    };
    size_t played=0;
    auto play=[&](const std::vector<Symbol> &alphabet,unsigned code) {
      if(++played>2*1024*1024)throw std::runtime_error("Oversized TZX symbol repetition stream");
      if(code>=alphabet.size())throw std::runtime_error("Invalid TZX symbol index");
      const auto &symbol=alphabet[code];
      if(symbol.flags==1)level=!level;else if(symbol.flags==2)level=false;else if(symbol.flags==3)level=true;
      for(unsigned ticks:symbol.ticks)toggle(ticks);
    };
    if(pilotSymbols) {
      auto alphabet=table(pilotAlphabet?pilotAlphabet:256,pilotWidth);
      for(unsigned n=0;n<pilotSymbols;++n) {
        unsigned code=r.u8(),repeat=r.u16();if(!repeat)throw std::runtime_error("Invalid TZX symbol repetition");
        for(unsigned k=0;k<repeat;++k)play(alphabet,code);
      }
    }
    if(dataSymbols) {
      unsigned count=dataAlphabet?dataAlphabet:256;auto alphabet=table(count,dataWidth);
      unsigned bits=0;while((1u<<bits)<count)++bits;
      size_t bytes=(std::uint64_t(bits)*dataSymbols+7)/8;r.need(bytes);size_t start=r.p,bit=0;
      for(unsigned n=0;n<dataSymbols;++n) {
        unsigned code=0;for(unsigned k=0;k<bits;++k,++bit)code=(code<<1)|((v[start+bit/8]>>(7-bit%8))&1);
        play(alphabet,code);
      }
      r.skip(bytes);
    }
    if(r.p!=b.start+b.length)throw std::runtime_error("Invalid TZX generalized block length");
  }
  if(b.pause) {
    pulse(3500,level);if(b.pause>1)pulse((b.pause-1)*3500,false);level=false;
  }
  return level;
}
inline std::vector<Block> Parse(const std::vector<unsigned char> &v,const std::vector<unsigned> &choices={}) {
  if(!Signature(v) || v.size()<10 || v[8]!=1)throw std::runtime_error("Invalid or unsupported TZX header");
  struct Raw {Block block;int jump=0;unsigned repeats=0;std::vector<int> calls;};
  Reader r(v,10);std::vector<Raw> raw;
  while(r.p<v.size()) {
    if(raw.size()>=16384)throw std::runtime_error("Too many TZX blocks");
    Raw item;auto &b=item.block;b.sourceIndex=raw.size();b.offset=r.p;b.kind=r.u8();
    switch(b.kind) {
      case 0x10:b.pause=r.u16();b.offset=r.p;b.length=r.u16();b.start=r.p;r.skip(b.length);
        if(b.length<2)throw std::runtime_error("Invalid standard TZX data");
        b.pilot=2168;b.count=v[b.start]<128?8063:3223;b.sync1=667;b.sync2=735;b.zero=855;b.one=1710;b.label="Standard data";break;
      case 0x11:b.pilot=r.u16();b.sync1=r.u16();b.sync2=r.u16();b.zero=r.u16();b.one=r.u16();b.count=r.u16();
        b.bits=r.u8();b.pause=r.u16();b.length=r.u24();b.start=r.p;r.skip(b.length);b.label="Turbo data";break;
      case 0x12:b.pilot=r.u16();b.count=r.u16();b.label="Pure tone";break;
      case 0x13:b.count=r.u8();b.start=r.p;r.skip(b.count*2);b.label="Pulse sequence";break;
      case 0x14:b.zero=r.u16();b.one=r.u16();b.bits=r.u8();b.pause=r.u16();b.length=r.u24();b.start=r.p;r.skip(b.length);b.label="Pure data";break;
      case 0x15:b.sample=r.u16();b.pause=r.u16();b.bits=r.u8();b.length=r.u24();b.start=r.p;r.skip(b.length);
        if(!b.sample)throw std::runtime_error("Invalid TZX sample period");b.label="Direct recording";break;
      case 0x18:{auto size=r.u32();if(size<10)throw std::runtime_error("Invalid TZX CSW header");r.need(size);
        b.pause=r.u16();b.sample=r.u24();b.zero=r.u8();b.count=r.u32();b.start=r.p;b.length=size-10;r.skip(b.length);
        if(!b.sample || (b.zero!=1 && b.zero!=2) || !b.count || b.count>2*1024*1024)throw std::runtime_error("Invalid TZX CSW parameters");
        b.label="CSW recording";break;}
      case 0x19:{auto size=r.u32();if(size<14)throw std::runtime_error("Invalid TZX generalized header");r.need(size);
        b.pause=r.u16();b.start=r.p;b.length=size-2;r.skip(b.length);b.label="Generalized data";break;}
      case 0x20:b.pause=r.u16();b.stop=!b.pause;b.label=b.stop?"Stop tape":"Pause";break;
      case 0x21:{unsigned n=r.u8();r.skip(n);break;}
      case 0x22:case 0x25:case 0x27:break;
      case 0x23:{unsigned n=r.u16();item.jump=n<32768?int(n):int(n)-65536;break;}
      case 0x24:item.repeats=r.u16();if(!item.repeats)throw std::runtime_error("Invalid TZX loop count");break;
      case 0x26:{unsigned n=r.u16();for(unsigned i=0;i<n;++i){unsigned x=r.u16();item.calls.push_back(x<32768?int(x):int(x)-65536);}break;}
      case 0x2a:if(r.u32()!=0)throw std::runtime_error("Invalid TZX stop block");b.stop=b.stop48=true;b.label="Stop tape (48K)";break;
      case 0x2b:if(r.u32()!=1)throw std::runtime_error("Invalid TZX signal block");item.jump=r.u8();if(item.jump>1)throw std::runtime_error("Invalid TZX signal level");break;
      case 0x30:{unsigned n=r.u8();r.skip(n);break;}
      case 0x31:r.u8();{unsigned n=r.u8();r.skip(n);break;}
      case 0x32:{unsigned n=r.u16();r.skip(n);break;}
      case 0x33:{unsigned n=r.u8();r.skip(n*3);break;}
      case 0x35:r.skip(16);{auto n=r.u32();r.skip(n);break;}
      case 0x5a:r.skip(9);break;
      case 0x28:{unsigned length=r.u16();r.need(length);size_t end=r.p+length;unsigned count=r.u8();
        if(!count)throw std::runtime_error("Empty TZX selection");
        for(unsigned n=0;n<count;++n){unsigned jump=r.u16(),size=r.u8();r.need(size);std::string label=Text(v,r.p,size);r.skip(size);
          b.options.push_back({label,jump<32768?int(jump):int(jump)-65536});}
        if(r.p!=end)throw std::runtime_error("Invalid TZX selection length");b.stop=true;b.label="Choose tape route";break;}
      default:throw std::runtime_error("Unsupported TZX block: "+std::to_string(b.kind)+" (decimal)");
    }
    if(b.bits<1 || b.bits>8)throw std::runtime_error("Invalid TZX last-byte bit count");
    raw.push_back(std::move(item));
  }
  struct Loop {size_t first;unsigned left;};std::vector<Loop> loops;
  struct Call {size_t origin,next;};std::vector<Call> calls;
  std::vector<Block> blocks;bool level=false;size_t steps=0,totalPulses=0,selection=0;
  auto target=[&](size_t origin,int delta)->size_t {
    auto n=static_cast<long long>(origin)+delta;
    if(n<0 || n>=static_cast<long long>(raw.size()))throw std::runtime_error("TZX jump leaves tape");return static_cast<size_t>(n);
  };
  for(size_t pc=0;pc<raw.size();) {
    if(++steps>100000 || blocks.size()>16384)throw std::runtime_error("TZX control flow exceeds playback limits");
    const auto &item=raw[pc];auto b=item.block;
    if(b.kind==0x28) {
      for(const auto &option:b.options)target(pc,option.jump);
      if(selection<choices.size()) {
        unsigned choice=choices[selection++];if(choice>=b.options.size())throw std::runtime_error("Invalid TZX route");
        pc=target(pc,b.options[choice].jump);continue;
      }
      b.initial=level;b.duration=1;blocks.push_back(std::move(b));return blocks;
    }
    if(!b.label.empty()) {
      b.initial=level;b.duration=0;
      level=Pulses(b,v,[&](unsigned n,bool){b.duration+=n;if(++totalPulses>32*1024*1024)throw std::runtime_error("TZX exceeds pulse budget");});
      if(!b.duration)throw std::runtime_error("Empty TZX signal block");
      blocks.push_back(std::move(b));
    }
    switch(item.block.kind) {
      case 0x23:pc=target(pc,item.jump);continue;
      case 0x24:if(loops.size()>=64)throw std::runtime_error("TZX loops are too deeply nested");loops.push_back({pc+1,item.repeats});break;
      case 0x25:if(loops.empty())throw std::runtime_error("Unmatched TZX loop end");if(--loops.back().left){pc=loops.back().first;continue;}loops.pop_back();break;
      case 0x26:if(!item.calls.empty()) {
        if(calls.size()>=64)throw std::runtime_error("TZX calls are too deeply nested");calls.push_back({pc,1});pc=target(pc,item.calls[0]);continue;
      }break;
      case 0x27:{if(calls.empty())throw std::runtime_error("Unmatched TZX return");auto &call=calls.back();const auto &sequence=raw[call.origin].calls;
        if(call.next<sequence.size()){pc=target(call.origin,sequence[call.next++]);continue;}
        pc=call.origin+1;calls.pop_back();continue;}
      case 0x2b:level=item.jump!=0;break;
    }
    ++pc;
  }
  if(selection!=choices.size())throw std::runtime_error("Unused TZX route choices");
  if(!loops.empty() || !calls.empty())throw std::runtime_error("Unfinished TZX loop or call");
  if(blocks.empty())throw std::runtime_error("TZX contains no playable blocks");
  return blocks;
}
}
#endif
