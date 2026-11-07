#ifndef CATALOG_ART_FIXTURE_H
#define CATALOG_ART_FIXTURE_H
#include "../game_catalog.h"
inline GameCatalog::Bytes PreviewPNG() {
  return {137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,2,0,0,0,144,119,83,222,0,0,0,12,73,68,65,84,120,156,99,184,99,100,3,0,3,56,1,75,21,74,160,255,0,0,0,0,73,69,78,68,174,66,96,130};
}
inline GameCatalog::Bytes PreviewGIF() {
  return {'G','I','F','8','9','a',1,0,1,0,128,0,0,40,200,110,255,255,255,44,0,0,0,0,1,0,1,0,0,2,2,68,1,0,59};
}
#endif
