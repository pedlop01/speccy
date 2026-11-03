#ifndef DISK_H
#define DISK_H
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace Disks {
struct Sector {unsigned char c,h,r,n,st1,st2;std::vector<unsigned char> data;};
struct Track {std::vector<Sector> sectors;unsigned char gap=0x4e,filler=0xe5;};
struct Image {
  unsigned cylinders=0,heads=0;std::vector<Track> tracks;
  static Image Parse(const std::vector<unsigned char> &v) {
    auto need=[&](size_t p,size_t n){if(p>v.size() || n>v.size()-p)throw std::runtime_error("Truncated DSK image");};
    need(0,256);bool extended=!memcmp(v.data(),"EXTENDED CPC DSK File\r\nDisk-Info\r\n",34);
    if(!extended && memcmp(v.data(),"MV - CPCEMU Disk-File\r\nDisk-Info\r\n",34))throw std::runtime_error("Not a standard or extended DSK image");
    Image disk;disk.cylinders=v[48];disk.heads=v[49];
    if(!disk.cylinders || disk.cylinders>102 || !disk.heads || disk.heads>2 || disk.cylinders*disk.heads>204)throw std::runtime_error("Invalid DSK geometry");
    unsigned fixed=v[50]|(v[51]<<8);size_t pos=256;disk.tracks.resize(disk.cylinders*disk.heads);
    for(size_t i=0;i<disk.tracks.size();++i) {
      size_t size=extended?unsigned(v[52+i])*256:fixed;
      if(!size && extended)continue;if(size<256)throw std::runtime_error("Invalid DSK track size");need(pos,size);
      if(memcmp(v.data()+pos,"Track-Info\r\n",12))throw std::runtime_error("Invalid DSK track header");
      unsigned count=v[pos+21],nominal=v[pos+20];if(count>29 || nominal>7)throw std::runtime_error("Invalid DSK sector table");
      auto &track=disk.tracks[i];track.gap=v[pos+22];track.filler=v[pos+23];size_t offset=pos+256;
      for(unsigned n=0;n<count;++n) {
        size_t p=pos+24+n*8;unsigned code=v[p+3];if(code>7)throw std::runtime_error("Unsupported DSK sector size");
        size_t bytes=extended?(v[p+6]|(v[p+7]<<8)):(128u<<nominal);
        if(!bytes)bytes=128u<<code;
        if(offset>pos+size || bytes>pos+size-offset)throw std::runtime_error("DSK sector exceeds track");
        Sector sector{v[p],v[p+1],v[p+2],v[p+3],v[p+4],v[p+5],{v.begin()+offset,v.begin()+offset+bytes}};
        track.sectors.push_back(std::move(sector));offset+=bytes;
      }
      pos+=size;
    }
    if(pos!=v.size())throw std::runtime_error("Unexpected data after DSK tracks");return disk;
  }
  std::vector<unsigned char> Encode() const {
    std::vector<unsigned char> out(256);memcpy(out.data(),"EXTENDED CPC DSK File\r\nDisk-Info\r\n",34);memcpy(out.data()+34,"Gomas",5);
    out[48]=cylinders;out[49]=heads;
    for(size_t index=0;index<tracks.size();++index) {
      const auto &track=tracks[index];if(track.sectors.empty())continue;
      size_t bytes=256;for(const auto &sector:track.sectors)bytes+=sector.data.size();bytes=(bytes+255)&~size_t(255);
      if(bytes>65280 || track.sectors.size()>29)throw std::runtime_error("DSK track cannot be encoded");
      out[52+index]=bytes/256;size_t pos=out.size();out.resize(pos+bytes);memcpy(out.data()+pos,"Track-Info\r\n",12);
      out[pos+16]=index/heads;out[pos+17]=index%heads;out[pos+20]=track.sectors.front().n;
      out[pos+21]=track.sectors.size();out[pos+22]=track.gap;out[pos+23]=track.filler;size_t data=pos+256;
      for(size_t n=0;n<track.sectors.size();++n) {
        const auto &sector=track.sectors[n];if(sector.data.empty() || sector.data.size()>65535)throw std::runtime_error("Invalid DSK sector length");
        size_t p=pos+24+n*8;unsigned char fields[]={sector.c,sector.h,sector.r,sector.n,sector.st1,sector.st2};std::copy(fields,fields+6,out.begin()+p);
        out[p+6]=sector.data.size();out[p+7]=sector.data.size()>>8;std::copy(sector.data.begin(),sector.data.end(),out.begin()+data);data+=sector.data.size();
      }
    }return out;
  }
};
struct Drive {std::shared_ptr<const Image> image;std::string path;bool writeProtected=false,dirty=false;};
// Non-DMA NEC uPD765 interface used by the +3. Sector data is transferred
// through the data register, with rationally timed byte-ready gaps.
class Controller {
public:
  enum class Phase {Command,Read,Write,Result,Format};
  struct State {
    std::array<Drive,2> drives;
    std::array<unsigned,4> cylinder{},target{},seek{};
    std::array<unsigned,2> rotation{};
    std::deque<unsigned> interrupts;
    std::vector<unsigned char> command,buffer,result;
    size_t position=0,sector=0;
    unsigned ready=0,unit=0,head=0,endSector=0,ncode=0,st0=0,st1=0,st2=0,lastR=0,formatCount=0;
    bool motor=false,deleted=false,multi=false;
    Phase phase=Phase::Command;
  } state;
  void Reset(){auto drives=state.drives;state=State();state.drives=std::move(drives);for(unsigned unit=0;unit<4;++unit)state.interrupts.push_back((0xc0|unit)<<8);}
  bool Insert(unsigned unit,Image image,const std::string &path){if(unit>1)return false;state.drives[unit].image=std::make_shared<const Image>(std::move(image));state.drives[unit].path=path;state.drives[unit].writeProtected=false;state.drives[unit].dirty=false;Abort();return true;}
  void Eject(unsigned unit){if(unit<2){state.drives[unit]=Drive();Abort();}}
  void Motor(bool on){state.motor=on;}
  unsigned Status() const {
    unsigned status=0x80;for(unsigned n=0;n<4;++n)if(state.seek[n])status|=1<<n;
    if(state.ready)status&=~0x80;
    if(state.phase==Phase::Result)return status|0x50;
    if(state.phase==Phase::Read)return status|0x70;
    if(state.phase==Phase::Write || state.phase==Phase::Format)return status|0x30;
    return status|(state.command.empty()?0:0x10);
  }
  void Advance(unsigned cycles){state.ready=cycles>=state.ready?0:state.ready-cycles;for(unsigned n=0;n<4;++n)if(state.seek[n]){
    if(cycles>=state.seek[n]){state.seek[n]=0;state.cylinder[n]=state.cylinder[n^2]=state.target[n];if(state.interrupts.size()>=32)state.interrupts.pop_front();state.interrupts.push_back(((0x20|n)<<8)|state.cylinder[n]);}
    else state.seek[n]-=cycles;
  }}
  unsigned char Read() {
    if(state.ready)return 0xff;
    if(state.phase==Phase::Result){auto value=state.result[state.position++];if(state.position==state.result.size())Abort();return value;}
    if(state.phase!=Phase::Read)return 0xff;
    auto value=state.buffer[state.position++];
    if(state.position==state.buffer.size())NextSector();else state.ready=114;
    return value;
  }
  void Write(unsigned char value) {
    if(state.ready)return;
    if(state.phase==Phase::Write || state.phase==Phase::Format) {
      state.buffer[state.position++]=value;
      if(state.position==state.buffer.size()) {
        if(state.phase==Phase::Format){FormatTrack();return;}
        auto disk=std::make_shared<Image>(*state.drives[state.unit].image);auto &sector=DiskTrack(*disk).sectors[state.sector];
        if(sector.data.size()<state.buffer.size())sector.data.resize(state.buffer.size());std::copy(state.buffer.begin(),state.buffer.end(),sector.data.begin());
        sector.st1=0;sector.st2=state.deleted?0x40:0;state.drives[state.unit].image=disk;state.drives[state.unit].dirty=true;NextSector();
      }else state.ready=114;
      return;
    }
    if(state.phase!=Phase::Command)return;
    state.command.push_back(value);unsigned op=state.command[0]&31;
    static const unsigned sizes[32]={1,1,9,3,2,9,9,2,1,9,2,1,9,6,1,3,1,9,1,1,1,1,1,1,1,9,1,1,1,1,1,1};
    if(state.command.size()>=sizes[op])Execute(op);
  }
  State CaptureState() const{return state;}
  bool RestoreState(const State &s) {
    if(s.st0>255 || s.st1>255 || s.st2>255)return false;
    for(auto irq:s.interrupts)if(irq>65535)return false;
    if(s.ncode>7 || s.lastR>255 || s.endSector>255 || s.formatCount>29)return false;
    if((s.phase==Phase::Read || s.phase==Phase::Write) && s.command.size()!=9)return false;
    if(s.phase==Phase::Format && s.command.size()!=6)return false;
    if(static_cast<unsigned>(s.phase)>4 || s.unit>3 || s.head>1 || s.command.size()>9 || s.buffer.size()>65536 || s.result.size()>7 || s.interrupts.size()>32 || s.ready>114)return false;
    for(unsigned n=0;n<4;++n)if(s.cylinder[n]>255 || s.target[n]>255 || s.seek[n]>1000000)return false;
    if((s.phase==Phase::Read || s.phase==Phase::Write || s.phase==Phase::Format) && (s.position>=s.buffer.size() || s.unit>1 || !s.drives[s.unit].image))return false;
    if(s.phase==Phase::Result && s.position>=s.result.size())return false;
    if(s.phase==Phase::Write && (s.cylinder[s.unit]>=s.drives[s.unit].image->cylinders || s.head>=s.drives[s.unit].image->heads ||
       s.sector>=s.drives[s.unit].image->tracks[s.cylinder[s.unit]*s.drives[s.unit].image->heads+s.head].sectors.size()))return false;
    state=s;return true;
  }
private:
  void Abort(){state.phase=Phase::Command;state.command.clear();state.buffer.clear();state.result.clear();state.position=0;state.ready=0;}
  void Result(std::vector<unsigned char> bytes){state.result=std::move(bytes);state.phase=Phase::Result;state.position=0;state.ready=0;}
  bool Ready() const{return state.unit<2 && state.motor && state.drives[state.unit].image;}
  Track &DiskTrack(Image &disk){return disk.tracks[state.cylinder[state.unit]*disk.heads+state.head];}
  const Track *CurrentTrack() const {
    if(!Ready())return nullptr;const auto &disk=*state.drives[state.unit].image;
    return state.cylinder[state.unit]<disk.cylinders && state.head<disk.heads?&disk.tracks[state.cylinder[state.unit]*disk.heads+state.head]:nullptr;
  }
  void Finish(){Result({static_cast<unsigned char>(state.st0),static_cast<unsigned char>(state.st1),static_cast<unsigned char>(state.st2),
    static_cast<unsigned char>(state.cylinder[state.unit]),static_cast<unsigned char>(state.head),static_cast<unsigned char>(state.lastR),static_cast<unsigned char>(state.ncode)});}
  void StartSector(unsigned id) {
    state.lastR=id;const auto *track=CurrentTrack();
    if(!track){state.st0|=0x40;state.st1|=4;Finish();return;}
    size_t found=track->sectors.size();
    for(size_t n=0;n<track->sectors.size();++n){size_t index=(state.rotation[state.unit]+n)%track->sectors.size();const auto &s=track->sectors[index];
      if(s.r==id && s.h==state.head && s.n==state.ncode){found=index;break;}}
    if(found==track->sectors.size()){state.st0|=0x40;state.st1|=4;Finish();return;}
    state.sector=found;state.rotation[state.unit]=(found+1)%track->sectors.size();const auto &sector=track->sectors[found];
    if(sector.c!=state.command[2]){state.st0|=0x40;state.st1|=4;state.st2|=0x10;Finish();return;}
    state.st1|=sector.st1;state.st2|=sector.st2;
    if(state.phase==Phase::Write && state.drives[state.unit].writeProtected){state.st0|=0x40;state.st1|=2;Finish();return;}
    size_t size=state.ncode?128u<<state.ncode:state.command[8];if(!size)size=128;
    if(size>16384 || sector.data.empty()){state.st0|=0x40;state.st1|=4;Finish();return;}
    state.buffer.resize(size);
    if(state.phase==Phase::Read)for(size_t n=0;n<size;++n)state.buffer[n]=sector.data[n%sector.data.size()];
    state.position=0;state.ready=114;
  }
  void NextSector() {
    if(state.st1 || (state.st2&~0x40)){state.st0|=0x40;Finish();return;}
    if(state.lastR<state.endSector){StartSector(state.lastR+1);return;}
    if(state.multi && state.head==0 && Ready() && state.drives[state.unit].image->heads>1){state.head=1;state.st0|=4;state.multi=false;StartSector(1);return;}
    state.st1|=0x80;state.st0|=0x40;Finish(); // End-of-cylinder without terminal count.
  }
  void FormatTrack() {
    if(!CurrentTrack()){state.st0|=0x40;state.st1|=4;Finish();return;}
    auto disk=std::make_shared<Image>(*state.drives[state.unit].image);auto &track=DiskTrack(*disk);track.sectors.clear();
    track.gap=state.command[4];track.filler=state.command[5];
    size_t bytes=256;
    for(unsigned n=0;n<state.formatCount;++n){size_t p=n*4;unsigned code=state.buffer[p+3];if(code>7){state.st0|=0x40;state.st1|=4;Finish();return;}
      bytes+=128u<<code;if(bytes>65280){state.st0|=0x40;state.st1|=4;Finish();return;}
      Sector sector{state.buffer[p],state.buffer[p+1],state.buffer[p+2],state.buffer[p+3],0,0,std::vector<unsigned char>(128u<<code,track.filler)};track.sectors.push_back(std::move(sector));}
    state.drives[state.unit].image=disk;state.drives[state.unit].dirty=true;state.lastR=track.sectors.empty()?0:track.sectors.back().r;Finish();
  }
  void Execute(unsigned op) {
    const auto &c=state.command;
    if(op==3){Abort();return;}
    if(op==8){if(state.interrupts.empty()){Result({0x80});return;}unsigned irq=state.interrupts.front();state.interrupts.pop_front();Result({static_cast<unsigned char>(irq>>8),static_cast<unsigned char>(irq)});return;}
    if(op==4){unsigned raw=c[1]&3,unit=raw&1,head=(c[1]>>2)&1;state.unit=unit;
      unsigned status=raw|(head<<2)|(state.cylinder[unit]==0?0x10:0)|(Ready()?0x20:0)|(unit<2 && state.drives[unit].writeProtected?0x40:0);
      Result({static_cast<unsigned char>(status)});return;}
    if(op==7 || op==15){unsigned unit=c[1]&3;state.seek[unit^2]=0;state.target[unit]=op==7?0:c[2];state.seek[unit]=(unsigned(std::abs(int(state.cylinder[unit&1])-int(state.target[unit])))+1)*3500;Abort();return;}
    if(op!=6 && op!=5 && op!=12 && op!=9 && op!=2 && op!=10 && op!=13){Result({0x80});return;}
    state.unit=c[1]&1;state.head=(c[1]>>2)&1;state.st0=(c[1]&3)|(state.head<<2);state.st1=state.st2=0;
    if(!Ready()){state.st0|=0x48;state.st1=4;Finish();return;}
    if(op==10){const auto *track=CurrentTrack();if(!track || track->sectors.empty()){state.st0|=0x40;state.st1=4;Finish();return;}
      const auto &sector=track->sectors[state.rotation[state.unit]++%track->sectors.size()];state.lastR=sector.r;state.ncode=sector.n;Finish();return;}
    if(op==13){state.ncode=c[2];state.formatCount=c[3];if(!state.formatCount || state.formatCount>29 || state.drives[state.unit].writeProtected){state.st0|=0x40;state.st1=state.drives[state.unit].writeProtected?2:4;Finish();return;}
      state.phase=Phase::Format;state.buffer.assign(state.formatCount*4,0);state.position=0;state.ready=114;return;}
    state.ncode=c[5];if(state.ncode>7){state.st0|=0x40;state.st1=4;Finish();return;}
    state.endSector=c[6];state.multi=(c[0]&128)!=0;state.deleted=op==9 || op==12;
    state.phase=(op==5 || op==9)?Phase::Write:Phase::Read;StartSector(c[4]);
  }
};
}
#endif
