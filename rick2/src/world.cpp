// Class automatically generated by Dev-C++ New Class wizard

#include "world.h" // class's header file


// class constructor
World::World()
{
}

World::World(const char *file, bool tileExtractedOption)
{
  // REVISIT: need to read collision map
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

  printf("Tileset file = %s\n", tileset_file);

  world_image = al_load_bitmap(tileset_file);
  if (!world_image) {
    printf("Error: failed to load tileset\n");    
  }

  tileset_count = tileset.attribute("tilecount").as_int();
  tileset_columns = tileset.attribute("columns").as_int();
  tileset_tile_width = tileset.attribute("tilewidth").as_int();
  tileset_tile_height = tileset.attribute("tileheight").as_int();

  printf("Tileset count = %d\nTileset columns = %d\nTile width = %d\nTile height = %d\n", tileset_count, tileset_columns, tileset_tile_width, tileset_tile_height);
  
  // Initialize world    
  world_tiles = new Tile**[map_width];
  
  for (int x = 0 ; x < map_width ; x++ ) {
      world_tiles[x] = new Tile*[map_height];
      for (int y = 0 ; y < map_height ; y++ ) {
          world_tiles[x][y] = new Tile();
      }
  }

  // First initialize tiles
  // Tile properties is a layer of same size as tiles, so we can reuse the for stament
  // for both of them. REVISIT: add check to verify they have same size?
  pugi::xml_node xml_tile = world_file.child("map").find_child_by_attribute("layer", "name", "Tiles").child("data");
  pugi::xml_node xml_tile_prop = world_file.child("map").find_child_by_attribute("layer", "name", "Collisions").child("data");

  int x = 0;
  int y = 0;
  pugi::xml_node prop = xml_tile_prop.first_child();
  for (pugi::xml_node tile = xml_tile.first_child(); tile; tile = tile.next_sibling()) {
    pugi::xml_attribute tile_attr = tile.first_attribute();
    pugi::xml_attribute prop_attr = prop.first_attribute();

    int tile_id = ((tile_attr.as_int() != 0) ? tile_attr.as_int() - 1: tile_attr.as_int());
    int tile_prop = ((prop_attr.as_int() != 0) ? prop_attr.as_int() - 1: prop_attr.as_int());
    // Save the id of the tile aswell as the coordinates in the tileset bitmap
    world_tiles[x][y]->SetValue(tile_id);
    world_tiles[x][y]->SetType(tile_prop);
    world_tiles[x][y]->SetLeftUpX((tile_id % tileset_columns) * tileset_width);
    world_tiles[x][y]->SetLeftUpY(ceil(tile_id/tileset_columns)*tileset_height);
    world_tiles[x][y]->SetRightDownX((tile_id % tileset_columns) * tileset_width + tileset_width);
    world_tiles[x][y]->SetRightDownY(ceil((tile_id/tileset_columns))*tileset_height + tileset_height);
    if (x == (map_width - 1)) {
      y++;
      x = 0;
    } else {
      x++;
    }
    // Move prop pointer
    prop = prop.next_sibling();
  }

  // Read platforms
  this->InitializePlatforms("../designs/platforms/platforms_level1.xml");
  // Read items
  this->InitializeItems("../designs/items/items_level1.xml");
  // Read dynamic background objects
  this->InitializeDynamicBackObjects("../designs/backgrounds/anim_tiles_level1.xml");

  // REVISIT: adding lasers manually
  Laser* laser1 = new Laser("../designs/lasers/laser_horizontal.xml", 264, 1980, 26, 6, LASER_TYPE_RECURSIVE, 5.0, OBJ_DIR_RIGHT);
  objects.push_back(laser1);

  // REVISIT: adding blocks manually
  Block* block1 = new Block();
  block1->Init("../designs/blocks/level1_bigblock.xml", 736, 2011, 24, 21, true);
  blocks.push_back(block1);
  Block* block2 = new Block();
  block2->Init("../designs/blocks/level1_smallblock.xml", 896, 1411, 24, 21, true);
  blocks.push_back(block2);
  Block* block3 = new Block();
  block3->Init("../designs/blocks/level1_smallblock.xml", 944, 1411, 24, 21, true);
  blocks.push_back(block3);
  Block* block4 = new Block();
  block4->Init("../designs/blocks/level1_bigblock.xml", 512, 1699, 24, 21, true);
  blocks.push_back(block4);
  Block* block5 = new Block();
  block5->Init("../designs/blocks/level1_bigblock.xml", 256, 1667, 24, 21, true);
  blocks.push_back(block5);
  Block* block6 = new Block();
  block6->Init("../designs/blocks/level1_smallblock.xml", 320, 1315, 24, 21, true);
  blocks.push_back(block6);
  Block* block7 = new Block();
  block7->Init("../designs/blocks/level1_smallblock.xml", 416, 1315, 24, 21, false);
  blocks.push_back(block7);
  Block* block8 = new Block();
  block8->Init("../designs/blocks/level1_smallblock.xml", 640, 707, 24, 21, true);
  blocks.push_back(block8);

  shoot_exists = false;
  bomb_exists = false;
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

  // Destroy platforms
  for (vector<Platform*>::iterator it = platforms.begin() ; it != platforms.end(); ++it) {
      delete *it;
  }
  for (list<Object*>::iterator it = objects.begin() ; it != objects.end(); ++it) {
      delete *it;
  }
  for (list<Block*>::iterator it = blocks.begin() ; it != blocks.end(); ++it) {
      delete *it;
  }
}

void World::InitializePlatforms(const char* file) {
  int platform_id;
  int plat_ini_x;
  int plat_ini_y;
  int plat_width;
  int plat_height;
  int plat_visible;
  int plat_recursive;
  int plat_one_use;
  int plat_ini_state;
  int action_direction;
  int action_desp;
  int action_wait;
  float action_speed;
  int num_actions;
  pugi::xml_document plat_file;

  printf("---------------------------\n");
  printf("| Initializing platforms  |\n");
  printf("---------------------------\n");

  pugi::xml_parse_result result = plat_file.load_file(file);

  if(!result) {
    printf("Error: loading world platform data\n");
  }
  
  for (pugi::xml_node plat = plat_file.child("platforms").first_child();
       plat;
       plat = plat.next_sibling()) {
    // First read attributes
    platform_id = plat.attribute("id").as_int();
    printf("Platform id = %d\n", platform_id);

    pugi::xml_node plat_attrs = plat.child("attributes");
    plat_ini_x = plat_attrs.attribute("ini_x").as_int();
    plat_ini_y = plat_attrs.attribute("ini_y").as_int();
    plat_width = plat_attrs.attribute("width").as_int();
    plat_height = plat_attrs.attribute("height").as_int();
    plat_visible = plat_attrs.attribute("visible").as_int();
    plat_recursive = plat_attrs.attribute("recursive").as_int();
    plat_one_use = plat_attrs.attribute("one_use").as_int();
    plat_ini_state = strcmp(plat_attrs.attribute("ini_state").as_string(), "stop") == 0 ?
                       OBJ_STATE_STOP :
                       OBJ_STATE_MOVING;
    printf(" - File = %s\n", plat_attrs.attribute("file").as_string());
    printf(" - ini_x = %d\n", plat_ini_x);
    printf(" - ini_y = %d\n", plat_ini_y);
    printf(" - width = %d\n", plat_width);
    printf(" - height = %d\n", plat_height);
    printf(" - visible = %d\n", plat_visible);
    printf(" - recursive = %d\n", plat_recursive);
    printf(" - one_use = %d\n", plat_one_use);
    printf(" - ini_state = %s\n", plat_attrs.attribute("ini_state").as_string());

    // Create platform
    Platform* world_platform = new Platform(plat_attrs.attribute("file").as_string(),
                                            platform_id,
                                            plat_ini_state,
                                            plat_ini_x,
                                            plat_ini_y,
                                            plat_width,
                                            plat_height,
                                            plat_visible,
                                            plat_recursive,
                                            plat_one_use);

    printf(" - actions:\n");
    num_actions = 0;
    // Second, get actions
    pugi::xml_node actions = plat.child("actions");
    for (pugi::xml_node action = actions.first_child();
         action;
         action = action.next_sibling()) {
      printf("\t - action %d:\n", num_actions);
      if (strcmp(action.attribute("direction").as_string(), "stop") == 0) {
        action_direction = OBJ_DIR_STOP;
      } else if (strcmp(action.attribute("direction").as_string(), "right") == 0) {
        action_direction = OBJ_DIR_RIGHT;
      } else if (strcmp(action.attribute("direction").as_string(), "left") == 0) {
        action_direction = OBJ_DIR_LEFT;
      } else if (strcmp(action.attribute("direction").as_string(), "up") == 0) {
        action_direction = OBJ_DIR_UP;
      } else if (strcmp(action.attribute("direction").as_string(), "down") == 0) {
        action_direction = OBJ_DIR_DOWN;
      }
      action_desp = action.attribute("desp").as_int();
      action_wait = action.attribute("wait").as_int();
      action_speed = action.attribute("speed").as_float();      
      printf("\t\t - direction=%s\n", action.attribute("direction").as_string());
      printf("\t\t - desp=%d\n", action_desp);
      printf("\t\t - wait=%d\n", action_wait);
      printf("\t\t - speed=%f\n", action_speed);

      world_platform->AddAction(action_direction,
                                action_desp,
                                action_wait,
                                action_speed);
      num_actions++;
    }
    platforms.push_back(world_platform);

  }  

  printf("---------------------------\n");
}

void World::InitializeItems(const char* file) {
  int item_id;
  int item_ini_x;
  int item_ini_y;
  int item_width;
  int item_height;  
  pugi::xml_document item_file;

  printf("---------------------------\n");
  printf("| Initializing items      |\n");
  printf("---------------------------\n");

  pugi::xml_parse_result result = item_file.load_file(file);

  if(!result) {
    printf("Error: loading world items data\n");
  }
 
  for (pugi::xml_node item = item_file.child("items").first_child();
       item;
       item = item.next_sibling()) {
    // First read attributes
    item_id = item.attribute("id").as_int();
    printf("Item id = %d\n", item_id);

    pugi::xml_node item_attrs = item.child("attributes");
    item_ini_x  = item_attrs.attribute("ini_x").as_int();
    item_ini_y  = item_attrs.attribute("ini_y").as_int();
    item_width  = item_attrs.attribute("width").as_int();
    item_height = item_attrs.attribute("height").as_int();

    printf(" - File = %s\n", item_attrs.attribute("file").as_string());
    printf(" - ini_x = %d\n", item_ini_x);
    printf(" - ini_y = %d\n", item_ini_y);
    printf(" - width = %d\n", item_width);
    printf(" - height = %d\n", item_height);

    // Create item
    Item* world_item = new Item(item_id);
    world_item->Init(item_attrs.attribute("file").as_string(),
                     item_ini_x, item_ini_y,
                     item_width, item_height,
                     true, true,
                     OBJ_STATE_STOP, OBJ_DIR_STOP,
                     0.1, 3.0, 1.0,
                     0.1, 3.0, 1.0);

    objects.push_back(world_item);
  }  

  printf("---------------------------\n");
}

void World::InitializeDynamicBackObjects(const char* file) {
  int dyn_obj_id;
  int dyn_obj_ini_x;
  int dyn_obj_ini_y;
  int dyn_obj_width;
  int dyn_obj_height;
  int dyn_obj_skip_num_anims;
  pugi::xml_document dyn_obj_file;

  printf("------------------------------------------\n");
  printf("| Initializing dynamic backgound objects |\n");
  printf("------------------------------------------\n");

  pugi::xml_parse_result result = dyn_obj_file.load_file(file);

  if(!result) {
    printf("Error: loading world dynamic background objects data\n");
  }
 
  for (pugi::xml_node dyn_obj = dyn_obj_file.child("anim_objects").first_child();
       dyn_obj;
       dyn_obj = dyn_obj.next_sibling()) {
    // First read attributes
    dyn_obj_id = dyn_obj.attribute("id").as_int();
    printf("Dynamic object id = %d\n", dyn_obj_id);

    pugi::xml_node dyn_obj_attrs = dyn_obj.child("attributes");
    dyn_obj_ini_x                = dyn_obj_attrs.attribute("ini_x").as_int();
    dyn_obj_ini_y                = dyn_obj_attrs.attribute("ini_y").as_int();
    dyn_obj_width                = dyn_obj_attrs.attribute("width").as_int();
    dyn_obj_height               = dyn_obj_attrs.attribute("height").as_int();
    dyn_obj_skip_num_anims       = dyn_obj_attrs.attribute("skip_num_anims").as_int();

    printf(" - File = %s\n", dyn_obj_attrs.attribute("file").as_string());
    printf(" - ini_x = %d\n", dyn_obj_ini_x);
    printf(" - ini_y = %d\n", dyn_obj_ini_y);
    printf(" - width = %d\n", dyn_obj_width);
    printf(" - height = %d\n", dyn_obj_height);
    printf(" - skip_anims = %d\n", dyn_obj_skip_num_anims);

    // Create dyn_obj
    StaticObject* world_dyn_obj = new StaticObject(dyn_obj_id);
    world_dyn_obj->Init(dyn_obj_attrs.attribute("file").as_string(),
                        dyn_obj_ini_x, dyn_obj_ini_y,
                        dyn_obj_width, dyn_obj_height,
                        dyn_obj_skip_num_anims);

    objects.push_back(world_dyn_obj);
  }  

  printf("---------------------------\n");
}

int World::GetMapWidth() {
   return map_width;
}

int World::GetMapHeight() {
  return map_height;
}

int World::GetTilesetWidth() {
  return tileset_width;
}

int World::GetTilesetHeight() {
  return tileset_height;
}

int World::GetTilesetCount() {
  return tileset_count;
}

int World::GetTilesetColumns() {
  return tileset_columns;
}

int World::GetTilesetTileWidth() {
  return tileset_tile_width;
}

int World::GetTilesetTileHeight() {
  return tileset_tile_height;
}

Tile* World::GetTile(int x, int y) {
  return world_tiles[x][y];
}

bool World::IsTileCollisionable(int x, int y) {
  return (world_tiles[x][y]->GetType() == TILE_COL);
}

bool World::IsTileCollisionableDown(int x, int y) {
  return ((world_tiles[x][y]->GetType() == TILE_COL) ||
          (world_tiles[x][y]->GetType() == TILE_COL_DOWN) ||
          (world_tiles[x][y]->GetType() == TILE_STAIRS_TOP));
}

int World::GetTileValue(int x, int y) {
  return world_tiles[x][y]->GetValue();
}

int World::GetTileValueByCoord(int x, int y)
{
  int tile_x = x / tileset_tile_width;
  int tile_y = y / tileset_tile_height;

  return world_tiles[tile_x][tile_y]->GetValue();
}

Tile* World::GetTileByCoord(int x, int y)
{
  int tile_x = x / tileset_tile_width;
  int tile_y = y / tileset_tile_height;

  return world_tiles[tile_x][tile_y];
}

void World::WorldStep(Character* player) {
  // Perform an step of all elements belonging to the world level
  for (vector<Platform*>::iterator it = platforms.begin() ; it != platforms.end(); ++it) {
      (*it)->PlatformStep();
      // REVISIT: remove this code
      if (player->GetState() == RICK_STATE_SHOOTING) {
        (*it)->SetTrigger();
      }
  }

  // Blocks
  for (list<Block*>::iterator it = blocks.begin() ; it != blocks.end(); ++it) {
    Block* block = *it;
    if (block->GetState() == OBJ_STATE_DEAD) {
      delete block;
      it = blocks.erase(it);
    } else if (block->GetActive()) {
      block->ObjectStep(this, player);
    }
  }
  // Global objects
  for (list<Object*>::iterator it = objects.begin() ; it != objects.end(); ++it) {
    Object* object = *it;
    if (object->GetState() == OBJ_STATE_DEAD) {
      //printf("[WorldStep] Object dead %d\n", object->GetId());
      // REVISIT: need to add STATIC OBJECT here? Same for other objects that may persists (LASER)
      switch (object->GetType()) {
        case OBJ_ITEM:
          delete ((Item*)object);
          break;
        case OBJ_SHOOT:
          delete ((Shoot*)object);
          shoot_exists = false;
          break;
        case OBJ_BOMB:
          delete ((Bomb*)object);
          bomb_exists = false;
          break;
        default:
          printf("[WARNING] Unknown object type to be deleted in World!\n");
          break;
      }
      it = objects.erase(it);                            // Remove element if it is dead.
      printf("[WorldStep] Removed object from object list\n");
    } else if (object->GetActive()) {
      //printf("Object active id = %d, type = %d\n", object->GetId(), object->GetType());
      switch (object->GetType()) {
        case OBJ_ITEM:
          ((Item*)object)->ObjectStep(this, player);
          break;
        case OBJ_STATIC:
          ((StaticObject*)object)->StaticObjectStep();
          break;
        case OBJ_LASER:
          ((Laser*)object)->ObjectStep(this, player);
          break;
        case OBJ_SHOOT:
          ((Shoot*)object)->ObjectStep(this, player);
          break;
        case OBJ_BOMB:
          ((Bomb*)object)->ObjectStep(this, player);
          break;
        default:
          break;
      }
    } else {
      printf("Object inactive %d\n", object->GetId());
    }
  }
  //printf("[WorldStep] End function\n");
}

void World::CreateNewShoot(int x, int y, int direction) {
  // Allow only one shoot to be created right now
  if (!shoot_exists) {
    Shoot* shoot = new Shoot("../designs/shoot/shoot.xml", x, y, 12, 6, direction);
    objects.push_back(shoot);
    shoot_exists = true;
  }
}

void World::CreateNewBomb(int x, int y, int direction) {
  // Allow only one bomb to be created right now
  if (!bomb_exists) {    
    printf("CreateNewBomb x=%d, y=%d\n", x, y);
    Bomb* shoot = new Bomb("../designs/bomb/bomb.xml", x, y - 1, 25, 22, direction);
    // REVISIT: not sure why height is 16. May it be 17?
    shoot->SetBoundingBox(8, 10, 10, 13);
    objects.push_back(shoot);
    bomb_exists = true;
  }
}