#ifndef SNAPSHOT_H
#define SNAPSHOT_H
#include <array>
#include <map>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include "z80.h"
#include "machine_config.h"
#include "disk.h"

namespace Snapshots {
struct Image {
  MachineModel model=MachineModel::Spectrum48;
  Z80Registers registers{};
  std::vector<unsigned char> ram;
  unsigned char paging=0,border=0,selectedAY=0;
  std::array<unsigned char,16> ay{};
  unsigned frameTStates=0,memPtr=0,tapeBlock=0;
  unsigned char pagingPlus3=0,ioData=0;
  bool halted=false,suppressInterrupt=false,hasIO=false;
  std::vector<unsigned char> tape;
  std::string externalTape;
  std::vector<std::vector<unsigned char>> romPages;
  std::map<unsigned,std::vector<unsigned char>> levels;
  std::array<Disks::Drive,2> disks;
};
struct Reader {
  const std::vector<unsigned char> &bytes;
  size_t pos;
  Reader(const std::vector<unsigned char> &v,size_t p=0):bytes(v),pos(p){}
  void need(size_t n) const {if(pos>bytes.size() || n>bytes.size()-pos) throw std::runtime_error("Truncated snapshot");}
  unsigned byte(){need(1);return bytes[pos++];}
  unsigned word(){unsigned lo=byte();return lo|(byte()<<8);}
  std::vector<unsigned char> take(size_t n){need(n);auto first=bytes.begin()+pos;pos+=n;return {first,first+n};}
};
inline unsigned Word(const std::vector<unsigned char> &v,size_t p) {Reader r(v,p);return r.word();}
inline Image SNA(const std::vector<unsigned char> &v) {
  if(v.size()!=49179 && v.size()!=131103 && v.size()!=147487) throw std::runtime_error("Invalid SNA size");
  Image s;Reader r(v);auto &z=s.registers;
  unsigned i=r.byte();z.altHL=r.word();z.altDE=r.word();z.altBC=r.word();z.altAF=r.word();
  z.HL=r.word();z.DE=r.word();z.BC=r.word();z.IY=r.word();z.IX=r.word();
  unsigned iff=r.byte();z.iff1A=z.iff1B=(iff&4)!=0;z.IR=(i<<8)|r.byte();
  z.AF=r.word();z.SP=r.word();z.IM=r.byte();s.border=r.byte();
  if(z.IM>2 || s.border>7) throw std::runtime_error("Invalid SNA registers");
  if(v.size()==49179) {
    s.ram=r.take(49152);
    if(z.SP<0x4000 || z.SP==0xffff) throw std::runtime_error("SNA stack does not contain a valid PC");
    z.PC=Word(s.ram,z.SP-0x4000);z.SP=static_cast<unsigned short>(z.SP+2);
  } else {
    s.model=MachineModel::Spectrum128;s.ram.resize(131072);
    z.PC=Word(v,49179);s.paging=v[49181];unsigned bank=s.paging&7;
    if(v[49182]) throw std::runtime_error("TR-DOS snapshots are not supported");
    bool loaded[8]={};unsigned slots[]={5,2,bank};
    for(unsigned n:slots) {
      auto page=r.take(16384);
      if(loaded[n] && !std::equal(page.begin(),page.end(),s.ram.begin()+n*16384))
        throw std::runtime_error("Conflicting duplicate SNA bank");
      std::copy(page.begin(),page.end(),s.ram.begin()+n*16384);loaded[n]=true;
    }
    r.pos=49183;
    for(unsigned n=0;n<8;++n) if(!loaded[n]) {auto page=r.take(16384);std::copy(page.begin(),page.end(),s.ram.begin()+n*16384);}
    if(r.pos!=v.size()) throw std::runtime_error("SNA bank count does not match paging");
  }
  return s;
}
inline std::vector<unsigned char> Expand(const std::vector<unsigned char> &v,size_t expected,bool endMarker=false) {
  Reader r(v);std::vector<unsigned char> out;out.reserve(expected);
  while(out.size()<expected) {
    unsigned value=r.byte();
    if(value==0xed && r.pos<v.size() && v[r.pos]==0xed) {
      ++r.pos;unsigned count=r.byte(),fill=r.byte();
      if(!count || count>expected-out.size()) throw std::runtime_error("Invalid Z80 RLE run");
      out.insert(out.end(),count,fill);
    } else out.push_back(value);
  }
  if(endMarker && v.size()-r.pos==4 && v[r.pos]==0 && v[r.pos+1]==0xed && v[r.pos+2]==0xed && v[r.pos+3]==0) r.pos+=4;
  if(r.pos!=v.size()) throw std::runtime_error("Invalid Z80 compressed block size");
  return out;
}
inline Image Z80(const std::vector<unsigned char> &v) {
  Reader r(v);r.need(30);Image s;auto &z=s.registers;
  z.AF=(r.byte()<<8);z.AF|=r.byte();z.BC=r.word();z.HL=r.word();z.PC=r.word();z.SP=r.word();
  unsigned i=r.byte(),refresh=r.byte(),flags=r.byte();if(flags==255) flags=1;
  z.IR=(i<<8)|(refresh&127)|((flags&1)<<7);s.border=(flags>>1)&7;
  z.DE=r.word();z.altBC=r.word();z.altDE=r.word();z.altHL=r.word();
  z.altAF=(r.byte()<<8);z.altAF|=r.byte();z.IY=r.word();z.IX=r.word();
  z.iff1A=r.byte()!=0;z.iff1B=r.byte()!=0;z.IM=r.byte()&3;
  if(z.IM>2) throw std::runtime_error("Invalid Z80 interrupt mode");
  if(z.PC) {
    if(flags&16) throw std::runtime_error("SamRam snapshots are not supported");
    auto ram=r.take(v.size()-r.pos);s.ram=(flags&32)?Expand(ram,49152,true):ram;
    if(s.ram.size()!=49152) throw std::runtime_error("Invalid Z80 v1 RAM size");
    return s;
  }
  unsigned length=r.word();if(length!=23 && length!=54 && length!=55) throw std::runtime_error("Unsupported Z80 header version");
  r.need(length);size_t end=r.pos+length;z.PC=r.word();unsigned mode=r.byte();s.paging=r.byte();unsigned interfaceROM=r.byte(),devices=r.byte();
  if(interfaceROM || (length>=54 && (v[59] || v[60]))) throw std::runtime_error("Snapshot requires an unsupported expansion ROM");
  if(mode==0 || mode==1) s.model=MachineModel::Spectrum48;
  else if((length==23 && (mode==3 || mode==4)) || (length!=23 && (mode==4 || mode==5 || mode==6))) s.model=MachineModel::Spectrum128;
  else if(mode==12) s.model=MachineModel::Plus2;
  else if(mode==7 || mode==8)s.model=MachineModel::Plus3;
  else throw std::runtime_error("Unsupported Z80 hardware mode: "+std::to_string(mode));
  if(devices&128) {
    if(s.model==MachineModel::Spectrum48) throw std::runtime_error("16K snapshots are not supported");
    if(s.model==MachineModel::Plus3)throw std::runtime_error("+2A snapshots are not supported");
    s.model=MachineModel::Plus2;
  }
  if(s.model==MachineModel::Spectrum48 && (devices&4)) throw std::runtime_error("48K AY expansion is not supported");
  s.selectedAY=r.byte()&15;for(auto &value:s.ay) value=r.byte();
  if(length>=54) {
    unsigned low=r.word(),high=r.byte(),quarter=HardwareFor(s.model).timing.TStatesPerFrame()/4;
    if(low>=quarter || high>3) throw std::runtime_error("Invalid Z80 video phase");
    s.frameTStates=((high+1)%4)*quarter+(quarter-1-low);
  }
  if(length==55)s.pagingPlus3=v[86];
  r.pos=end;bool flat=s.model==MachineModel::Spectrum48;
  if(flat) s.paging=0;
  s.ram.resize(flat?49152:131072);bool loaded[8]={};
  while(r.pos<v.size()) {
    unsigned size=r.word(),page=r.byte();size_t count=size==65535?16384:size;
    auto data=r.take(count);if(size!=65535) data=Expand(data,16384);
    int bank=flat?(page==8?0:page==4?1:page==5?2:-1):(page>=3 && page<=10?int(page)-3:-1);
    if(bank<0) {
      if(page==0 || page==1 || page==2 || page==11) continue; // Optional ROM pages; use the installed ROM set.
      throw std::runtime_error("Unexpected Z80 memory page");
    }
    if(loaded[bank]) throw std::runtime_error("Duplicate Z80 RAM page");
    loaded[bank]=true;std::copy(data.begin(),data.end(),s.ram.begin()+bank*16384);
  }
  for(unsigned n=0;n<(flat?3u:8u);++n) if(!loaded[n]) throw std::runtime_error("Z80 snapshot is missing RAM pages");
  return s;
}
}
#endif
