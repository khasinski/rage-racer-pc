#include "menu_images.h"
#include "mod_assets.h"
#include "render/mod_manifest.h"
#include "render/asset_path.h"
#include "modern/modern_png.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static struct {
    unsigned generation;
    uint8_t checked[7][2],valid[7][2];
    uint8_t pixels[7][2][MENU_IMAGE_WIDTH*MENU_IMAGE_HEIGHT*4];
} images;
static const char *names[]={"grand-prix","extra-grand-prix","time-attack","custom","duel","load-save","options"};
const uint8_t *MenuImage(int item,int selected){
    if(item<0||item>=7)return NULL;
    selected=selected!=0;
    unsigned generation=ModAssetsGeneration();
    if(generation!=images.generation){memset(&images,0,sizeof(images));images.generation=generation;}
    if(images.checked[item][selected])return images.valid[item][selected]?images.pixels[item][selected]:NULL;
    images.checked[item][selected]=1;
    const RageModManifest *manifest=ModAssetsManifest();
    const char *root=ModAssetsDirectory();char key[96],path[1600];
    snprintf(key,sizeof(key),"ui.title.%s%s",names[item],selected?".selected":"");
    const char *relative=manifest?ModManifestFindTexture(manifest,key):NULL;
    if(!root||!relative)return NULL;
    if(!AssetPathIsRelativeFile(relative,strlen(relative))||snprintf(path,sizeof(path),"%s/%s",root,relative)>=(int)sizeof(path))return NULL;
    SDL_PathInfo info;
    if(!SDL_GetPathInfo(path,&info)||info.type!=SDL_PATHTYPE_FILE||info.size>128u*1024u*1024u)return NULL;
    SDL_Surface *loaded=ModernLoadPNG(SDL_IOFromFile(path,"rb"),MENU_IMAGE_WIDTH),*surface=NULL;
    if(loaded&&loaded->w==MENU_IMAGE_WIDTH&&loaded->h==MENU_IMAGE_HEIGHT)surface=SDL_ConvertSurface(loaded,SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if(!surface){fprintf(stderr,"rage-port: %s must be a valid 112x16 PNG\n",key);return NULL;}
    for(int y=0;y<MENU_IMAGE_HEIGHT;y++)memcpy(images.pixels[item][selected]+y*MENU_IMAGE_WIDTH*4,(uint8_t*)surface->pixels+y*surface->pitch,MENU_IMAGE_WIDTH*4);
    SDL_DestroySurface(surface);images.valid[item][selected]=1;
    return images.pixels[item][selected];
}
