#ifndef SPRITE_H
#define SPRITE_H

typedef class Sprite* sprite_ptr; 
class Sprite {
  public:
    int    x;
    int    y;
    int    width;
    int    height;
    
  public:    
	  Sprite();    // class constructor	
    Sprite(int _x, int _y, int _width, int _height);
	  ~Sprite();   // class desructor
};

#endif // SPRITE_H
