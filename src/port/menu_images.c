#include "menu_images.h"
#include "rage/title_menu.h"
#include "mod_assets.h"
#include "render/mod_manifest.h"
#include "render/asset_path.h"
#include "modern/modern_image.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int s_textureOverridesEnabled = 1;

static struct {
    unsigned generation;
    uint8_t checked[TITLE_MENU_QUIT][2],valid[TITLE_MENU_QUIT][2];
    uint8_t pixels[TITLE_MENU_QUIT][2][MENU_IMAGE_WIDTH*MENU_IMAGE_HEIGHT*4];
    SDL_Surface *native[TITLE_MENU_QUIT][2];
} images;
static RageModManifest *hdManifest;
static char hdRoot[1400];
int MenuImagesSetHdPack(const char *directory,const RageModManifest *manifest){
    RageModManifest *copy=NULL;
    if(directory&&manifest){if(strlen(directory)>=sizeof(hdRoot))return 0;copy=malloc(sizeof(*copy));if(!copy)return 0;*copy=*manifest;}
    MenuImagesReset();free(hdManifest);hdManifest=copy;SDL_strlcpy(hdRoot,copy?directory:"",sizeof(hdRoot));return 1;
}

static const char *names[]={"grand-prix","extra-grand-prix","time-attack","custom","duel",
#ifndef __ANDROID__
"online",
#endif
"load-save","options"};
void MenuImagesReset(void){
    for(int i=0;i<TITLE_MENU_QUIT;i++)for(int j=0;j<2;j++)SDL_DestroySurface(images.native[i][j]);
    memset(&images,0,sizeof(images));
}
void MenuImagesSetTextureOverridesEnabled(int enabled){
    enabled=enabled!=0;if(enabled!=s_textureOverridesEnabled){MenuImagesReset();s_textureOverridesEnabled=enabled;}
}
const uint8_t *MenuImage(int item,int selected){
    if(!s_textureOverridesEnabled)return NULL;
    if(item<0||item>=TITLE_MENU_QUIT)return NULL;
    selected=selected!=0;
    unsigned generation=ModAssetsGeneration();
    if(generation!=images.generation){
        MenuImagesReset();images.generation=generation;
    }
    if(images.checked[item][selected])return images.valid[item][selected]?images.pixels[item][selected]:NULL;
    images.checked[item][selected]=1;
    const RageModManifest *manifest=ModAssetsManifest();
    const char *root=ModAssetsDirectory();char key[96],path[1600];
    snprintf(key,sizeof(key),"ui.title.%s%s",names[item],selected?".selected":"");
    const char *relative=hdManifest?ModManifestFindTexture(hdManifest,key):NULL;
    if(relative)root=hdRoot;else relative=manifest?ModManifestFindTexture(manifest,key):NULL;
    if(!root||!relative)return NULL;
    if(!AssetPathIsRelativeFile(relative,strlen(relative))||snprintf(path,sizeof(path),"%s/%s",root,relative)>=(int)sizeof(path))return NULL;
    SDL_PathInfo info;
    if(!SDL_GetPathInfo(path,&info)||info.type!=SDL_PATHTYPE_FILE||info.size>128u*1024u*1024u)return NULL;
    SDL_Surface *loaded=ModernLoadImage(SDL_IOFromFile(path,"rb"),4096),*surface=NULL;
    if(loaded&&loaded->w>=MENU_IMAGE_WIDTH&&loaded->h>=MENU_IMAGE_HEIGHT&&
       (loaded->w%MENU_IMAGE_WIDTH)==0&&(loaded->h%MENU_IMAGE_HEIGHT)==0&&
       (loaded->w/MENU_IMAGE_WIDTH)==(loaded->h/MENU_IMAGE_HEIGHT))
        surface=SDL_ConvertSurface(loaded,SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if(!surface){fprintf(stderr,"rage-port: %s must be a valid 112x16 PNG\n",key);return NULL;}
    int scale=surface->w/MENU_IMAGE_WIDTH;
    if(scale==1){
        for(int y=0;y<MENU_IMAGE_HEIGHT;y++)memcpy(images.pixels[item][selected]+y*MENU_IMAGE_WIDTH*4,(uint8_t*)surface->pixels+y*surface->pitch,MENU_IMAGE_WIDTH*4);
    }else{
        uint32_t area=(uint32_t)scale*(uint32_t)scale;
        for(int y=0;y<MENU_IMAGE_HEIGHT;y++){
            for(int x=0;x<MENU_IMAGE_WIDTH;x++){
                uint32_t sum[4]={0,0,0,0};
                for(int sy=0;sy<scale;sy++){
                    const uint8_t *row=(const uint8_t*)surface->pixels+(y*scale+sy)*surface->pitch+(x*scale)*4;
                    for(int sx=0;sx<scale;sx++){
                        sum[0]+=row[sx*4+0];sum[1]+=row[sx*4+1];
                        sum[2]+=row[sx*4+2];sum[3]+=row[sx*4+3];
                    }
                }
                uint8_t *dst=images.pixels[item][selected]+(y*MENU_IMAGE_WIDTH+x)*4;
                for(int c=0;c<4;c++)dst[c]=(uint8_t)((sum[c]+area/2u)/area);
            }
        }
    }
    images.native[item][selected]=surface;images.valid[item][selected]=1;
    return images.pixels[item][selected];
}

const uint8_t *MenuImageNative(int item,int selected,int *width,int *height){
    if(!width||!height)return NULL;
    *width=*height=0;
    if(!MenuImage(item,selected))return NULL;
    SDL_Surface *surface=images.native[item][selected!=0];
    *width=surface->w;*height=surface->h;
    return surface->pixels;
}
