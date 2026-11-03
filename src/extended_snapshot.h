#ifndef EXTENDED_SNAPSHOT_H
#define EXTENDED_SNAPSHOT_H
#include <cstring>
#include <map>
#include <zlib.h>
#include "snapshot.h"
#include "tape.h"

namespace Snapshots {
inline std::vector<unsigned char> Inflate(const std::vector<unsigned char> &data,size_t expected) {
  if(!expected || expected>16*1024*1024)throw std::runtime_error("Invalid compressed snapshot size");
  std::vector<unsigned char> out(expected);uLongf count=expected;uLong input=data.size();
  if(uncompress2(out.data(),&count,data.data(),&input)!=Z_OK || count!=expected || input!=data.size())
    throw std::runtime_error("Invalid snapshot zlib data");
  return out;
}
inline Image SZX(const std::vector<unsigned char> &v) {
  Reader r(v);r.need(8);if(memcmp(v.data(),"ZXST",4) || v[4]!=1)throw std::runtime_error("Invalid or unsupported SZX header");
  Image s;
  switch(v[6]){case 1:s.model=MachineModel::Spectrum48;break;case 2:s.model=MachineModel::Spectrum128;break;case 3:s.model=MachineModel::Plus2;break;
    case 5:s.model=MachineModel::Plus3;break;default:throw std::runtime_error("Unsupported SZX machine");}
  if(v[7]&1)throw std::runtime_error("Alternate SZX hardware timings are not supported");
  r.pos=8;s.ram.resize(HardwareFor(s.model).memory.RAMBytes());bool pages[8]={},cpu=false,ula=false,ay=false,tape=false;
  while(r.pos<v.size()) {
    auto id=r.take(4);unsigned length=r.word();length|=r.word()<<16;r.need(length);
    auto body=r.take(length);Reader b(body);std::string tag(id.begin(),id.end());
    if(tag=="Z80R") {
      if(cpu || length<37)throw std::runtime_error("Invalid SZX CPU block");cpu=true;auto &z=s.registers;
      z.AF=b.word();z.BC=b.word();z.DE=b.word();z.HL=b.word();z.altAF=b.word();z.altBC=b.word();z.altDE=b.word();z.altHL=b.word();
      z.IX=b.word();z.IY=b.word();z.SP=b.word();z.PC=b.word();unsigned i=b.byte();z.IR=(i<<8)|b.byte();
      z.iff1A=b.byte()!=0;z.iff1B=b.byte()!=0;z.IM=b.byte();s.frameTStates=b.word();s.frameTStates|=b.word()<<16;
      b.byte();unsigned flags=b.byte();s.halted=(flags&2)!=0;s.suppressInterrupt=(flags&1)!=0;s.memPtr=b.word();
      if(z.IM>2 || s.frameTStates>=HardwareFor(s.model).timing.TStatesPerFrame())throw std::runtime_error("Invalid SZX CPU state");
    } else if(tag=="SPCR") {
      if(ula || length<8)throw std::runtime_error("Invalid SZX ULA block");ula=true;
      s.border=b.byte();s.paging=b.byte();s.pagingPlus3=b.byte();s.ioData=b.byte();s.hasIO=true;
      if(s.border>7 || (s.model==MachineModel::Spectrum48 && s.paging))throw std::runtime_error("Invalid SZX paging");
    } else if(tag=="RAMP") {
      unsigned flags=b.word(),page=b.byte();if(flags&~1)throw std::runtime_error("Unsupported SZX RAM flags");
      int bank=s.model==MachineModel::Spectrum48?(page==5?0:page==2?1:page==0?2:-1):page<8?int(page):-1;
      if(bank<0 || pages[bank])throw std::runtime_error("Invalid or duplicate SZX RAM page");pages[bank]=true;
      auto data=b.take(body.size()-b.pos);if(flags&1)data=Inflate(data,16384);
      if(data.size()!=16384)throw std::runtime_error("Invalid SZX RAM page size");
      std::copy(data.begin(),data.end(),s.ram.begin()+bank*16384);
    } else if(tag==std::string("AY\0\0",4)) {
      if(ay || length!=18)throw std::runtime_error("Invalid SZX AY block");ay=true;unsigned flags=b.byte();
      if(s.model==MachineModel::Spectrum48 || flags&1)throw std::runtime_error("SZX AY expansion is not supported");
      s.selectedAY=b.byte()&15;for(auto &value:s.ay)value=b.byte();
    } else if(tag=="TAPE") {
      if(tape || length<28)throw std::runtime_error("Invalid SZX tape block");tape=true;s.tapeBlock=b.word();unsigned flags=b.word();
      unsigned unpacked=b.word();unpacked|=b.word()<<16;unsigned packed=b.word();packed|=b.word()<<16;
      auto extension=b.take(16);
      if(flags&~3)throw std::runtime_error("Unknown SZX tape flags");
      if(!(flags&1)) {
        auto filename=b.take(body.size()-b.pos);auto end=std::find(filename.begin(),filename.end(),0);
        if(end==filename.end() || end==filename.begin() || filename.size()>4096)throw std::runtime_error("Invalid linked SZX tape filename");
        s.externalTape.assign(filename.begin(),end);continue;
      }
      auto data=b.take(body.size()-b.pos);if(data.size()!=packed)throw std::runtime_error("Invalid embedded SZX tape length");
      if(flags&2)data=Inflate(data,unpacked);else if(data.size()!=unpacked)throw std::runtime_error("Invalid SZX tape size");
      if(!ValidateTape(data))throw std::runtime_error("Unsupported or invalid embedded SZX tape");s.tape=std::move(data);
    } else if(tag==std::string("+3\0\0",4)) {
      if(length<2 || s.model!=MachineModel::Plus3 || b.byte()>2)throw std::runtime_error("Invalid SZX +3 drive configuration");
      bool motor=b.byte()!=0;s.pagingPlus3=(s.pagingPlus3&~8)|(motor?8:0);
    } else if(tag==std::string("DSK\0",4)) {
      unsigned flags=b.word(),unit=b.byte(),size=b.word();size|=b.word()<<16;
      if(unit>1 || s.model!=MachineModel::Plus3 || s.disks[unit].image || !(flags&2) || (flags&~7))throw std::runtime_error("Unsupported SZX disk reference");
      if(flags&4)throw std::runtime_error("Flipped SZX disk images are not supported");
      auto data=b.take(body.size()-b.pos);if(flags&1)data=Inflate(data,size);else if(data.size()!=size)throw std::runtime_error("Invalid SZX disk size");
      s.disks[unit].image=std::make_shared<const Disks::Image>(Disks::Image::Parse(data));s.disks[unit].path="Embedded SZX disk";
    } else if(tag==std::string("ROM\0",4)) {
      unsigned flags=b.word(),size=b.word();size|=b.word()<<16;
      auto data=b.take(body.size()-b.pos);if(flags&1)data=Inflate(data,size);
      if(flags&~1 || data.size()!=size || size!=HardwareFor(s.model).memory.ROMBytes() || !s.romPages.empty())throw std::runtime_error("Invalid SZX ROM block");
      for(size_t p=0;p<size;p+=16384)s.romPages.emplace_back(data.begin()+p,data.begin()+p+16384);
    } else if(tag=="B128" || tag==std::string("IF1\0",4) || tag=="IF2R" || tag=="MFCE" || tag=="DIDE" || tag=="OPUS" || tag=="PLSD") {
      throw std::runtime_error("SZX requires an unsupported peripheral: "+tag);
    }
    // Unknown extension/metadata chunks are length-delimited by the SZX spec.
  }
  if(!cpu || !ula)throw std::runtime_error("SZX is missing CPU or ULA state");
  for(unsigned n=0;n<HardwareFor(s.model).memory.ramBanks;++n)if(!pages[n])throw std::runtime_error("SZX is missing RAM pages");
  return s;
}
inline Image SLT(const std::vector<unsigned char> &v) {
  Reader r(v);r.need(32);if(Word(v,6)!=0)throw std::runtime_error("SLT requires a Z80 v2/v3 snapshot");
  unsigned header=Word(v,30);if(header!=23 && header!=54 && header!=55)throw std::runtime_error("Invalid SLT snapshot header");
  r.pos=32;r.need(header);r.pos+=header;
  while(true) {
    r.need(3);if(v[r.pos]==0 && v[r.pos+1]==0 && v[r.pos+2]==0)break;
    unsigned length=r.word();r.byte();r.take(length==65535?16384:length);
  }
  size_t end=r.pos;r.pos+=3;auto signature=r.take(3);if(memcmp(signature.data(),"SLT",3))throw std::runtime_error("Missing SLT signature");
  Image s=Z80({v.begin(),v.begin()+end});
  struct Entry{unsigned type,id,length;};std::vector<Entry> entries;
  while(true) {
    unsigned type=r.word(),id=r.word(),length=r.word();length|=r.word()<<16;
    if(!type)break;if(entries.size()>=4096)throw std::runtime_error("Too many SLT entries");entries.push_back({type,id,length});
  }
  for(const auto &entry:entries) {
    auto packed=r.take(entry.length);if(entry.type!=1)continue;
    if(entry.id>255 || s.levels.count(entry.id))throw std::runtime_error("Invalid or duplicate SLT level");
    Reader data(packed);std::vector<unsigned char> level;
    while(data.pos<packed.size()) {
      unsigned value=data.byte(),count=1;
      if(value==0xed && data.pos<packed.size() && packed[data.pos]==0xed){++data.pos;count=data.byte();value=data.byte();}
      if(!count || count>65536-level.size())throw std::runtime_error("Invalid SLT RLE data");level.insert(level.end(),count,value);
    }
    if(level.empty())throw std::runtime_error("Empty SLT level");s.levels.emplace(entry.id,std::move(level));
  }
  if(r.pos!=v.size())throw std::runtime_error("Trailing SLT data");return s;
}
}
#endif
