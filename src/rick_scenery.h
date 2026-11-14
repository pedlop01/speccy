#ifndef RICK_SCENERY_H
#define RICK_SCENERY_H

// Authored materials for Hyde Park's original 8x8 tiles, shared across rooms.
// Zero means unchecked. Actor masks protect non-solid background tiles.
namespace RickScenery {
// Only recolour existing outdoor art. Empty pixels never gain new scenery.
inline bool OutdoorDetail(unsigned room,unsigned tile,unsigned y,unsigned scroll) {
  const bool outdoor=room==0 || room==3 || (room==2 && scroll==112 && y>=72);
  return outdoor && (tile==187 || tile==188 || (tile>=228 && tile<=230) ||
    tile==243 || tile==244 || tile==249);
}
inline unsigned Colour(unsigned tile,bool ink) {
  if(tile==174)return ink?0x26364b:0x1e2b3e; // original diagonal exterior hatching
  if(tile>=175 && tile<=180)return ink?0x9caec0:0x354353; // steel roof and wall
  if(tile==27 || tile==28 || tile==29 || tile==30 || tile==245 || tile==246)
    return ink?0xb99462:0x332b25; // riveted cross-braced supports and capitals
  if(tile>=181 && tile<=183)return ink?0x72944e:0x17251c; // grass edge
  if(tile>=184 && tile<=186)return ink?0x6b6445:0x29291f; // soil/root texture
  if(tile>=35 && tile<=48)return ink?0x30323e:0x695866; // riveted interior panels
  if(tile>=73 && tile<=77)return ink?0x293745:0x647d8c; // brickwork and mortar
  if(tile>=78 && tile<=83)return ink?0x93afba:0x384c58; // wall vents
  if(tile>=84 && tile<=93)return ink?0x9bafb6:0x425b68; // inset metal doors
  if(tile>=61 && tile<=72)return ink?0xc5a766:0x252b32; // framed wall display
  if((tile>=118 && tile<=125) || (tile>=128 && tile<=131) ||
     tile==134 || tile==135 || tile==138 || tile==139 || tile==142 || tile==143)
    return ink?0xb0c3bc:0x3f5d5a; // safe pipe bends, joints and straight runs
  if(tile==160 || tile==161)return ink?0xc9a568:0x32302c; // ladder rungs
  if((tile>=49 && tile<=60) || (tile>=102 && tile<=117) ||
     tile==150 || tile==151 || (tile>=156 && tile<=159) ||
     (tile>=195 && tile<=197) || (tile>=231 && tile<=234) || tile==255)
    return ink?0x9cabb1:0x3c4850; // patterned platforms, bracing and grilles
  if(tile>=164 && tile<=169)return ink?0xc0a477:0x4e4437; // platform end caps
  return 0;
}
inline unsigned Pixel(unsigned room,unsigned tile,unsigned /*x*/,unsigned y,bool ink,unsigned scroll=0) {
  if(!OutdoorDetail(room,tile,y,scroll))return Colour(tile,ink);
  if(!ink)return 0; // preserve original PAPER between branches/building details
  if(tile==187 || tile==188)return 0xa18f74; // small original background structures
  if(tile>=228 && tile<=230)return y<168?0x547c59:0x71935c;
  return 0x49605c; // original fine texture
}
}
#endif
