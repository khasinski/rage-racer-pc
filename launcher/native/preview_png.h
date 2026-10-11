#include <SDL3/SDL.h>
#include <stdint.h>
/* SDL's compiled PNG writer compresses the shared decoded RGBA pixels.
 * Surface borrows the pixels only for this synchronous save. */
static int WritePng(const char *path,const uint8_t *rgba,uint32_t w,uint32_t h){
    if(!path||!rgba||!w||!h||w>4096||h>4096)return 0;
    SDL_Surface *surface=SDL_CreateSurfaceFrom((int)w,(int)h,SDL_PIXELFORMAT_RGBA32,(void *)rgba,(int)w*4);
    if(!surface)return 0;
    int ok=SDL_SavePNG(surface,path);SDL_DestroySurface(surface);return ok;
}
