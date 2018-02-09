// Class automatically generated by Dev-C++ New Class wizard

#include "mundo.h" // class's header file

// class constructor
mundo::mundo()
{
}

mundo::mundo(char *file, bool tileExtractedOption)
{
    char aux_file[80];                  
    char path[250];
                  
    sprintf(aux_file, "%s", file);
    
    cout << "==============================================" << endl;                  
    cout << " Reading world file " << aux_file << endl;
    screen_orig_x = 0;
    screen_orig_y = 0;
    screen_tiles_x = 0;
    screen_tiles_y = 0;
    tiles_orig_x  = 0;
    tiles_orig_y  = 0;
    tiles_m_x     = 0;
    tiles_m_y     = 0;
    tiles_size_x  = 0;
    tiles_size_y  = 0;
    tiles_in_tiles_image_x = 0;
    
    io.obrirFitxer(aux_file, 0);
    io.llegirParaula(buffer);            // Name
    io.llegirParaula(buffer);            // Name value
    io.llegirParaula(buffer);            // Image
    io.llegirParaula(path_world_image);  // Image path
    io.llegirParaula(buffer);            // screen_x
    io.llegirEnter(&screen_orig_x);      // screen_x value
    io.llegirParaula(buffer);            // screen_y
    io.llegirEnter(&screen_orig_y);      // screen_y value
    io.llegirParaula(buffer);            // tiles_orig_size_x
    io.llegirEnter(&tiles_orig_x);       // tiles_orig_size_x value
    io.llegirParaula(buffer);            // tiles_orig_size_y
    io.llegirEnter(&tiles_orig_y);       // tiles_orig_size_y value
    io.llegirParaula(buffer);            // screen_tiles_x
    io.llegirEnter(&screen_tiles_x);     // screen_tiles_x value
    io.llegirParaula(buffer);            // screen_tiles_y     
    io.llegirEnter(&screen_tiles_y);     // screen_tiles_y value     
    io.llegirParaula(buffer);            // tiles_size_x
    io.llegirEnter(&tiles_size_x);       // tiles_size_x value
    io.llegirParaula(buffer);            // tiles_size_y
    io.llegirEnter(&tiles_size_y);       // tiles_size_y value  
    io.llegirParaula(buffer);            // num_scroll_planes
    io.llegirEnter(&num_scroll_planes);  // num_scroll_planes value
      
    tileSizeRatio = tiles_size_x / tiles_orig_x;  // FIXME: assuming x is equal to y
    cout << "\t Not explicit info of world" << endl;
    cout << "\t Tile size ratio = " << tileSizeRatio << endl; 

    string directory = chopToDirectory(aux_file);                  
    
    scroll_planes = new BITMAP*[num_scroll_planes];
    scroll_size_x = new int[num_scroll_planes];
    scroll_size_y = new int[num_scroll_planes];
    scroll_vel_x  = new int[num_scroll_planes];
    scroll_vel_y  = new int[num_scroll_planes];
    for( int i = 0 ; i < num_scroll_planes ; i++ )
    {
        io.llegirParaula(buffer);        // scroll
        io.llegirParaula(buffer);        // scroll_path  
        sprintf(path, "%s%s", directory.c_str(), buffer);              
        cout << path << endl;
        scroll_planes[i] = load_bmp(path, NULL);
        assert(scroll_planes[i] != NULL);
        io.llegirParaula(buffer);             // scroll_size_x
        io.llegirEnter(&(scroll_size_x[i]));  // scroll_size_x value
        io.llegirParaula(buffer);             // scroll_size_y
        io.llegirEnter(&(scroll_size_y[i]));  // scroll_size_y value
        io.llegirParaula(buffer);             // scroll_vel_x
        io.llegirEnter(&(scroll_vel_x[i]));   // scroll_vel_x value
        io.llegirParaula(buffer);             // scroll_vel_y
        io.llegirEnter(&(scroll_vel_y[i]));   // scroll_vel_y value        
    }    

    io.llegirParaula(buffer);            // num_scroll_planes_front
    io.llegirEnter(&num_scroll_planes_front);  // num_scroll_planes_front value    

    scroll_planes_front = new BITMAP*[num_scroll_planes_front];
    scroll_size_x_front = new int[num_scroll_planes_front];
    scroll_size_y_front = new int[num_scroll_planes_front];
    scroll_vel_front_x  = new int[num_scroll_planes_front];
    scroll_vel_front_y  = new int[num_scroll_planes_front];    
    for( int i = 0 ; i < num_scroll_planes_front ; i++ )
    {
        io.llegirParaula(buffer);        // scroll
        io.llegirParaula(buffer);        // scroll_path    

        sprintf(path, "%s%s", directory.c_str(), buffer);                    
        scroll_planes_front[i] = load_bmp(path, NULL);
        assert(scroll_planes_front[i] != NULL);
        io.llegirParaula(buffer);             // scroll_size_x
        io.llegirEnter(&(scroll_size_x_front[i]));  // scroll_size_x value
        io.llegirParaula(buffer);             // scroll_size_y
        io.llegirEnter(&(scroll_size_y_front[i]));  // scroll_size_y value
        io.llegirParaula(buffer);             // scroll_vel
        io.llegirEnter(&(scroll_vel_front_x[i]));     // scroll_vel value
        io.llegirParaula(buffer);             // scroll_vel
        io.llegirEnter(&(scroll_vel_front_y[i]));     // scroll_vel value        
    }    
        
    sprintf(path, "%s%s", directory.c_str(), path_world_image);
    
    world_image = load_bmp(path, NULL);    
    assert( world_image != NULL );
     
    tiles_m_x = screen_orig_x / tiles_orig_x;
    tiles_m_y = screen_orig_y / tiles_orig_y;
    cout << "\t Tiles in screen x = " << tiles_m_x << endl;
    cout << "\t Tiles in screen y = " << tiles_m_x << endl;
         
    tiles_mundo = new tile**[tiles_m_y];
    for( int i = 0 ; i < tiles_m_y ; i++ )
    {
        tiles_mundo[i] = new tile*[tiles_m_x];
        for( int j = 0 ; j < tiles_m_x ; j++ )
        {
            tiles_mundo[i][j] = new tile();
        }
    }

    io.llegirParaula(buffer);
    for( int i = 0 ; i < tiles_m_y ; i++ )
    {
        for( int j = 0 ; j < tiles_m_x ; j++ )
        {
            int val;
            io.llegirEnter(&val);
            tiles_mundo[i][j]->SetValue(val);
            val = 0;
            if( tileExtractedOption == false )
            {
                io.llegirEnter(&val);
            }            
            tiles_mundo[i][j]->SetCollisionable(val);
            val = 0;
            io.llegirEnter(&val);
            tiles_mundo[i][j]->SetPintado(val);
        }         
    }    
    
    screen_new_x = tiles_m_x * tiles_size_x;
    screen_new_y = tiles_m_y * tiles_size_y;
    cout << "\t World size x = " << screen_new_x << endl;
    cout << "\t World size y = " << screen_new_y << endl;
        
    cout << "==============================================" << endl;
}

// class destructor
mundo::~mundo()
{
    // Delete tiles
    for( int i = 0 ; i < tiles_m_y ; i++ )
    {
        for( int j = 0 ; j < tiles_m_x ; j++ )
        {
            delete tiles_mundo[i][j];
        }
        delete tiles_mundo[i];
    }
    delete tiles_mundo;
    
    // Delete scrolls
    for( int i = 0 ; i < num_scroll_planes_front ; i++ )
    {
         destroy_bitmap(scroll_planes_front[i]);
    }
    delete scroll_planes_front;
    delete scroll_size_x_front;
    delete scroll_size_y_front;
    delete scroll_vel_front_x;
    delete scroll_vel_front_y;      
    for( int i = 0 ; i < num_scroll_planes ; i++ )
    {    
        destroy_bitmap(scroll_planes[i]);
    }        
    delete scroll_planes;
    delete scroll_size_x;
    delete scroll_size_y;
    delete scroll_vel_x;
    delete scroll_vel_y;    
    
    destroy_bitmap(world_image);
}

int mundo::GetTileValueByCoord(int x, int y)
{
    int tile_x = div(x, tiles_size_x).quot;
    int tile_y = div(y, tiles_size_y).quot;
 
    assert( tile_x < tiles_m_x && tile_x >= 0);   
    assert( tile_y < tiles_m_y && tile_y >= 0);

    return tiles_mundo[tile_y][tile_x]->GetValue();
}

tile* mundo::GetTileByCoord(int x, int y)
{
    int tile_x = div(x, tiles_size_x).quot;
    int tile_y = div(y, tiles_size_y).quot;

    assert( tile_x < tiles_m_x && tile_x >= 0);   
    assert( tile_y < tiles_m_y && tile_y >= 0);

    return tiles_mundo[tile_y][tile_x];
}

void mundo::SaveWorldInFile(char *file)
{
    entradaSortida io;
     
    io.obrirFitxer(file, ESCRIPTURA);

    for( int i = 0 ; i < tiles_m_y ; i++ )
    {
        for( int j = 0 ; j < tiles_m_x ; j++ )
        {
            io.escriureEnter(tiles_mundo[i][j]->GetValue());
            io.escriureEspai();
            io.escriureEnter(tiles_mundo[i][j]->GetCollisionable());
            io.escriureEspai();
            io.escriureEnter(tiles_mundo[i][j]->GetPintado());            
            if( j < tiles_m_x - 1)
                io.escriureParaula(", ");
        }
        io.escriureSaltDeLinia();
    }     
}
