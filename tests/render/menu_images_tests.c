#include "port/menu_images.h"
#include "render/mod_manifest.h"
#include "game/menu_internal.h"
#include "game/prim.h"
#include <SDL3/SDL.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
static RageModManifest manifest;
static unsigned generation=1,calls;
static const char *root;
unsigned ModAssetsGeneration(void){return generation;}
const char *ModAssetsDirectory(void){return root;}
const RageModManifest *ModAssetsManifest(void){return &manifest;}
u8 *AddTilePrim(GameOrderingTableEntry *ot,u8 *packet,s32 x,s32 y,s32 w,s32 h,s32 r,s32 g,s32 b){
    (void)ot;(void)y;assert(x>=10&&w>0&&h==1);assert(r==17&&g==34&&b==51);if(calls==0){assert(x==11);assert(w==111);}calls++;return packet;
}
static void WriteImageSize(const char *path,int width,int height){
    SDL_Surface *s=SDL_CreateSurface(width,height,SDL_PIXELFORMAT_RGBA32);assert(s);
    assert(SDL_FillSurfaceRect(s,NULL,SDL_MapRGBA(SDL_GetPixelFormatDetails(s->format),NULL,17,34,51,255)));
    int scale=width>=112&&height>=16&&width%112==0&&height%16==0&&width/112==height/16?width/112:1;
    for(int y=0;y<scale;y++)for(int x=0;x<scale;x++)((uint8_t*)s->pixels+y*s->pitch)[x*4+3]=0;
    assert(SDL_SavePNG(s,path));SDL_DestroySurface(s);
}
static void WriteImage(const char *path,int width){
    WriteImageSize(path,width,16);
}
int main(int argc,char **argv){
    assert(argc==2);root=argv[1];assert(SDL_CreateDirectory(root));char path[1024];
    snprintf(path,sizeof(path),"%s/custom.png",root);WriteImage(path,112);
    assert(MenuImage(-1,0)==NULL);assert(MenuImage(7,0)==NULL);assert(MenuImage(3,0)==NULL);
    const char text[]="[textures]\n\"ui.title.custom\" = \"custom.png\"\n";
    assert(ModManifestParse(text,strlen(text),&manifest));generation++;
    const uint8_t *p=MenuImage(3,0);assert(p&&p[0]==17&&p[1]==34&&p[2]==51&&p[3]==0);
    assert(MenuImage(3,1)==NULL);assert(MenuImage(2,0)==NULL);
    u8 packet[1];assert(DrawTitleMenuLabel(NULL,packet,TITLE_MENU_CUSTOM,10,20,16,0)==packet);assert(calls==16);
    calls=0;DrawTitleMenuLabel(NULL,packet,TITLE_MENU_CUSTOM,10,20,3,0);assert(calls==3);
    assert(MenuImage(3,0)==p);WriteImage(path,113);assert(MenuImage(3,0)==p);
    generation++;assert(MenuImage(3,0)==NULL);WriteImage(path,112);assert(MenuImage(3,0)==NULL);
    generation++;assert(MenuImage(3,0)!=NULL);
    WriteImageSize(path,224,32);generation++;
    p=MenuImage(3,0);assert(p&&p[0]==17&&p[1]==34&&p[2]==51&&p[3]==0);
    calls=0;assert(DrawTitleMenuLabel(NULL,packet,TITLE_MENU_CUSTOM,10,20,16,0)==packet);assert(calls==16);
    WriteImageSize(path,448,64);generation++;
    p=MenuImage(3,0);assert(p&&p[0]==17&&p[1]==34&&p[2]==51&&p[3]==0);
    assert(SDL_RemovePath(path));assert(SDL_RemovePath(root));
    puts("menu PNG runtime/cache/draw tests passed");return 0;
}
