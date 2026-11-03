#include "port/modern/modern_image.h"
#include <webp/encode.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,SDL_GetError());return 1;}}while(0)
int main(void){
    const unsigned char rgba[]={12,34,56,0, 230,100,25,128, 0,255,90,255, 255,0,255,0};
    WebPConfig config;WebPPicture picture;WebPMemoryWriter writer;
    CHECK(WebPConfigInit(&config)&&WebPConfigLosslessPreset(&config,6)&&WebPPictureInit(&picture));
    config.exact=1;picture.use_argb=1;picture.width=2;picture.height=2;
    WebPMemoryWriterInit(&writer);picture.writer=WebPMemoryWrite;picture.custom_ptr=&writer;
    CHECK(WebPPictureImportRGBA(&picture,rgba,8)&&WebPEncode(&config,&picture));WebPPictureFree(&picture);
    SDL_Surface *surface=ModernLoadImage(SDL_IOFromConstMem(writer.mem,writer.size),2);
    CHECK(surface&&surface->w==2&&surface->h==2);
    CHECK(!memcmp(surface->pixels,rgba,8)&&!memcmp((unsigned char*)surface->pixels+surface->pitch,rgba+8,8));
    SDL_DestroySurface(surface);
    CHECK(!ModernLoadImage(SDL_IOFromConstMem(writer.mem,writer.size),1));
    CHECK(!ModernLoadImage(SDL_IOFromConstMem(writer.mem,writer.size-1),2));
    writer.mem[4]^=1;CHECK(!ModernLoadImage(SDL_IOFromConstMem(writer.mem,writer.size),2));
    CHECK(!ModernLoadImage(SDL_IOFromConstMem("not an image",12),4096));
    WebPMemoryWriterClear(&writer);
    puts("WebP exact RGBA, alpha, dimensions and truncated/header validation passed");return 0;
}
