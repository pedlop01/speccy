// Class automatically generated by Dev-C++ New Class wizard

#include "world.h" // class's header file

// class constructor
World::World()
{
}

World::World(char *file, bool tileExtractedOption)
{
  char aux_file[100];
  char tileset_file[100];

  pugi::xml_parse_result result = world_file.load_file(file);

  if(!result) {
      printf("Error: loading world data\n");      
  }  

  pugi::xml_node map = world_file.child("map");

  map_width = map.attribute("width").as_int();
  map_height = map.attribute("height").as_int();
  tileset_width = map.attribute("tilewidth").as_int();
  tileset_height = map.attribute("tileheight").as_int();

  pugi::xml_node tileset = world_file.child("map").child("tileset");

  sprintf(aux_file, "%s", file);
  sprintf(tileset_file, "%s/%s", chopToDirectory(aux_file).c_str(), tileset.attribute("name").as_string());
    
  world_image = al_load_bitmap(tileset_file);
  if (!world_image) {
    printf("Error: failed to load tileset\n");    
  }

  tileset_count = tileset.attribute("tilecount").as_int();
  tileset_columns = tileset.attribute("columns").as_int();
  tileset_tile_width = tileset.attribute("tilewidth").as_int();
  tileset_tile_height = tileset.attribute("tileheight").as_int();

  pugi::xml_node tile_layer = world_file.child("map").child("layer").child("data");

  // Initialize world    
  world_tiles = new Tile**[map_width];
  
  for (int x = 0 ; x < map_width ; x++ ) {
      world_tiles[x] = new Tile*[map_height];
      for (int y = 0 ; y < map_height ; y++ ) {
          world_tiles[x][y] = new Tile();
      }
  }

  int x = 0;
  int y = 0;
  for (pugi::xml_node tile = tile_layer.first_child(); tile; tile = tile.next_sibling()) {
    for (pugi::xml_attribute attr = tile.first_attribute(); attr; attr = attr.next_attribute()) {
      int tile_id = ((attr.as_int() != 0) ? attr.as_int() - 1: attr.as_int());
      // Save the id of the tile aswell as the coordinates in the tileset bitmap
      world_tiles[x][y]->SetValue(tile_id);
      world_tiles[x][y]->SetLeftUpX((tile_id % tileset_columns) * tileset_width);      
      world_tiles[x][y]->SetLeftUpY(ceil((tile_id/tileset_columns))*tileset_height);
      world_tiles[x][y]->SetRightDownX((tile_id % tileset_columns) * tileset_width + tileset_width);
      world_tiles[x][y]->SetRightDownY(ceil((tile_id/tileset_columns))*tileset_height + tileset_height);
      if (x == (map_width - 1)) {
        y++;
        x = 0;
      } else {
        x++;
      }
    }    
  }

}

// class destructor
World::~World()
{
  // Delete tiles
//  for( int i = 0 ; i < tiles_m_y ; i++ ) {
//    for( int j = 0 ; j < tiles_m_x ; j++ ) {
//      delete tiles_mundo[i][j];
//    }
//    delete tiles_mundo[i];
//  }
//  delete tiles_mundo;
//    
//  al_destroy_bitmap(world_image);
}

int World::GetTileValueByCoord(int x, int y)
{
//  int tile_x = div(x, tiles_size_x).quot;
//  int tile_y = div(y, tiles_size_y).quot;
//
//  assert( tile_x < tiles_m_x && tile_x >= 0);   
//  assert( tile_y < tiles_m_y && tile_y >= 0);
//
//  return tiles_mundo[tile_y][tile_x]->GetValue();
}

Tile* World::GetTileByCoord(int x, int y)
{
//  int tile_x = div(x, tiles_size_x).quot;
//  int tile_y = div(y, tiles_size_y).quot;
//
//  assert( tile_x < tiles_m_x && tile_x >= 0);   
//  assert( tile_y < tiles_m_y && tile_y >= 0);
//
//  return tiles_mundo[tile_y][tile_x];
}