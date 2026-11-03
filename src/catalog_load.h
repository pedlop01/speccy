#ifndef CATALOG_LOAD_H
#define CATALOG_LOAD_H
#include "game_catalog.h"

namespace GameCatalog {
// Only catalog loads auto-type commands. File > Open keeps its manual behavior.
inline bool Load(SpectrumMachine &machine,const std::string &path,MachineModel preferred,std::string &error) {
  Bytes bytes;std::string name;if(!TapeFiles::ReadMedia(path,bytes,name,error))return false;
  auto dot=name.find_last_of('.');std::string type=dot==std::string::npos?"":Lower(name.substr(dot+1));
  std::unique_ptr<SpectrumMachine> candidate(new SpectrumMachine);
  if(type=="tap" || type=="tzx") {
    if(machine.systemROM.model==preferred)candidate->systemROM=machine.systemROM;
    Snapshots::Image blank;blank.model=preferred;blank.ram.resize(HardwareFor(preferred).memory.RAMBytes());
    if(!candidate->OpenSnapshot(blank,"",error))return false;
  }else candidate->systemROM=machine.systemROM;
  if(!candidate->Open(path,error))return false;
  if(type=="tap" || type=="tzx") {
    candidate->SetCPUMultiplier(machine.CPUMultiplier());candidate->SetFastLoad(machine.fastLoad);
    bool flat=candidate->Configuration().model==MachineModel::Spectrum48;
    for(unsigned frame=0;frame<580;++frame) {
      auto key=[&](unsigned when,unsigned row,unsigned col){if(frame==when)candidate->ula.PressKey(row,col,true);if(frame==when+10)candidate->ula.PressKey(row,col,false);};
      if(flat){key(500,2,6);if(frame==520 || frame==540)candidate->ula.PressKey(3,8,true);key(520,1,9);key(540,1,9);if(frame==550)candidate->ula.PressKey(3,8,false);key(560,2,9);}
      else key(500,2,9);
      if(frame==570 && !candidate->fastLoad)candidate->PlayTape();candidate->Frame();
    }
  } else if(type=="dsk") {
    for(unsigned frame=0;frame<460;++frame){if(frame==450)candidate->ula.PressKey(2,9,true);candidate->Frame();}candidate->ula.PressKey(2,9,false);
  }
  candidate->DiscardAudio();
  return machine.ImportSession(candidate->systemROM,candidate->tape,candidate->tapePath,candidate->CaptureState(),error);
}
}
#endif
