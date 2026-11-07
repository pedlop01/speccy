#ifndef CATALOG_ART_H
#define CATALOG_ART_H
#include <allegro5/allegro_image.h>
#include <gif_lib.h>
#include "game_catalog.h"

// Use a static first frame for animated screenshots. Streaming GIF decoding
// avoids allocating every frame of potentially very long gameplay recordings.
inline ALLEGRO_BITMAP *LoadCatalogArtwork(const std::string &path) {
  try {
    auto bytes=GameCatalog::Read(path,4*1024*1024);
    auto type=GameCatalog::ArtworkType(bytes);
    struct Flags {int value=al_get_new_bitmap_flags();~Flags(){al_set_new_bitmap_flags(value);}} flags;
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
    ALLEGRO_BITMAP *bitmap=nullptr;
    if(type!="gif")bitmap=al_load_bitmap(path.c_str());
    else {
      int error=0;
      auto close=[](GifFileType *file){int error=0;DGifCloseFile(file,&error);};
      std::unique_ptr<GifFileType,decltype(close)> gif(DGifOpenFileName(path.c_str(),&error),close);
      if(gif) {
        const unsigned width=gif->SWidth,height=gif->SHeight;
        std::vector<unsigned char> pixels(size_t(width)*height*4,0);
        for(size_t n=3;n<pixels.size();n+=4)pixels[n]=255;
        GifRecordType record;bool decoded=false;int transparent=-1;
        while(DGifGetRecordType(gif.get(),&record)==GIF_OK && record!=TERMINATE_RECORD_TYPE) {
          if(record==EXTENSION_RECORD_TYPE) {
            int code;GifByteType *data=nullptr;
            if(DGifGetExtension(gif.get(),&code,&data)!=GIF_OK)break;
            if(code==GRAPHICS_EXT_FUNC_CODE && data && data[0]>=4 && (data[1]&1))transparent=data[4];
            bool valid=true;
            while(data)if(DGifGetExtensionNext(gif.get(),&data)!=GIF_OK){valid=false;break;}
            if(!valid)break;
          }else if(record==IMAGE_DESC_RECORD_TYPE) {
            if(DGifGetImageDesc(gif.get())!=GIF_OK)break;
            const auto &image=gif->Image;
            if(image.Width<=0 || image.Height<=0 || image.Left<0 || image.Top<0 ||
                unsigned(image.Left+image.Width)>width || unsigned(image.Top+image.Height)>height)break;
            auto *map=image.ColorMap?image.ColorMap:gif->SColorMap;if(!map)break;
            std::vector<GifPixelType> row(image.Width);bool valid=true;
            auto line=[&](int y){
              if(DGifGetLine(gif.get(),row.data(),image.Width)!=GIF_OK){valid=false;return;}
              for(int x=0;x<image.Width;++x){int index=row[x];if(index==transparent)continue;
                if(index>=map->ColorCount){valid=false;return;}auto color=map->Colors[index];
                size_t p=(size_t(image.Top+y)*width+image.Left+x)*4;
                pixels[p]=color.Red;pixels[p+1]=color.Green;pixels[p+2]=color.Blue;}
            };
            if(image.Interlace) {const int starts[]={0,4,2,1},steps[]={8,8,4,2};
              for(int pass=0;pass<4 && valid;++pass)for(int y=starts[pass];y<image.Height && valid;y+=steps[pass])line(y);
            }else for(int y=0;y<image.Height && valid;++y)line(y);
            decoded=valid;break;
          }
        }
        if(decoded) {
          bitmap=al_create_bitmap(width,height);
          if(bitmap){auto *lock=al_lock_bitmap(bitmap,ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,ALLEGRO_LOCK_WRITEONLY);
            if(lock){for(unsigned y=0;y<height;++y)memcpy(static_cast<char *>(lock->data)+ptrdiff_t(y)*lock->pitch,pixels.data()+size_t(y)*width*4,width*4);al_unlock_bitmap(bitmap);}
            else{al_destroy_bitmap(bitmap);bitmap=nullptr;}}
        }
      }
    }
    return bitmap;
  }catch(const std::exception &){return nullptr;}
}
#endif
