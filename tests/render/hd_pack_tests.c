#include "port/hd_pack.h"
#include "port/hd_pack_ui.h"
#include "port/overlay_model.h"
#include "port/modern/modern_assets.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *testStore;static int applied,restarted,layerEnabled;
static int restartSuccess=1;
static int allowHd=1;
int MemoryBudgetAllowHd(void){return allowHd;}
const char *RuntimeConfigGet(const char *key){return !strcmp(key,"hd.store")?testStore:!strcmp(key,"hd.server")?"invalid-test-origin":NULL;}
const char *HostDiscRegion(void){return "PAL";}
int PlatformUserConfigPath(const char *name,char *out,size_t n){(void)name;(void)out;(void)n;return 0;}
const RagePortConfig *PortActiveConfig(void){static RagePortConfig config;return &config;}
int MenuImagesSetHdPack(const char *root,const RageModManifest *m){(void)root;(void)m;return 1;}
int ModernAssetsSetHdPack(const char *root,const RageModManifest *m){applied++;layerEnabled=root&&m;return 1;}
int ModernRestartPresentation(const RagePortConfig *config){(void)config;restarted++;return restartSuccess;}
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s (%s)\n",__LINE__,#x,error);return 1;}}while(0)
static int cancel(void *context,int percent){(void)context;return percent<50;}
int main(void){
 char error[256]={0},root[1000],source[1100],store[1100],path[1200],archive[1100],bad[1100];
 const char *temporary=SDL_getenv("TMPDIR");
 if(!temporary||!*temporary)temporary=SDL_getenv("TEMP");
 if(!temporary||!*temporary)temporary=SDL_getenv("TMP");
 if(!temporary||!*temporary)temporary=".";
 snprintf(root,sizeof(root),"%s/rage-hd-%llu",temporary,(unsigned long long)SDL_GetPerformanceCounter());

 snprintf(source,sizeof(source),"%s/source",root);snprintf(store,sizeof(store),"%s/store",root);snprintf(archive,sizeof(archive),"%s/test.rrhd",root);snprintf(bad,sizeof(bad),"%s/bad.rrhd",root);
 CHECK(SDL_CreateDirectory(source));snprintf(path,sizeof(path),"%s/textures",source);CHECK(SDL_CreateDirectory(path));
 const char *metadata="{\"format\":1,\"name\":\"Test\",\"version\":\"1\",\"region\":\"PAL\"}";
 snprintf(path,sizeof(path),"%s/pack.json",source);CHECK(SDL_SaveFile(path,metadata,strlen(metadata)));
 const char *manifest="[mod]\nname=\"Test\"\n[textures]\n\"ui.hud.page.9.clut.30924\"=\"textures/font.png\"\n";
 snprintf(path,sizeof(path),"%s/mod.toml",source);CHECK(SDL_SaveFile(path,manifest,strlen(manifest)));
 SDL_Surface *surface=SDL_CreateSurface(8,8,SDL_PIXELFORMAT_RGBA32);CHECK(surface);SDL_FillSurfaceRect(surface,NULL,0xffffffffu);snprintf(path,sizeof(path),"%s/textures/font.png",source);CHECK(SDL_SavePNG(surface,path));SDL_DestroySurface(surface);
 CHECK(HdPackCreate(source,archive,error,sizeof(error)));CHECK(!HdPackCreate(source,archive,error,sizeof(error)));
 CHECK(HdPackInstall(archive,store,"PAL",NULL,NULL,error,sizeof(error)));HdPackState a,b;
 CHECK(HdPackReadState(store,"PAL",&a)&&a.enabled);CHECK(HdPackVerify(store,&a,NULL,NULL,error,sizeof(error)));
 CHECK(HdPackSetEnabled(store,&a,0));CHECK(HdPackReadState(store,"PAL",&b)&&!b.enabled);CHECK(!strcmp(a.hash,b.hash));
 CHECK(!HdPackInstall(archive,store,"NTSC-U",NULL,NULL,error,sizeof(error)));CHECK(HdPackReadState(store,"PAL",&b)&&!b.enabled&&!strcmp(a.hash,b.hash));
 CHECK(!HdPackInstall(archive,store,"PAL",cancel,NULL,error,sizeof(error)));CHECK(HdPackReadState(store,"PAL",&b)&&!b.enabled&&!strcmp(a.hash,b.hash));
 size_t n;unsigned char *bytes=SDL_LoadFile(archive,&n);CHECK(bytes&&n>20);bytes[n-1]^=1;CHECK(SDL_SaveFile(bad,bytes,n));SDL_free(bytes);
 CHECK(!HdPackInstall(bad,store,"PAL",NULL,NULL,error,sizeof(error)));CHECK(HdPackReadState(store,"PAL",&b)&&!b.enabled&&!strcmp(a.hash,b.hash));CHECK(HdPackVerify(store,&b,NULL,NULL,error,sizeof(error)));
 CHECK(HdPackInstall(archive,store,"PAL",NULL,NULL,error,sizeof(error)));CHECK(HdPackReadState(store,"PAL",&b)&&b.enabled);
 testStore=store;OverlayModel model={0};allowHd=0;HdPackUiTick(&model);
 CHECK(!model.values.hdEnabled&&applied==0&&restarted==0);
 model.packAction=1;HdPackUiTick(&model);CHECK(applied==0&&strstr(model.packStatus,"Low memory"));
 CHECK(HdPackReadState(store,"PAL",&b)&&b.enabled);
 allowHd=1;model.packAction=1;HdPackUiTick(&model);
 /* First normal toggle disables the previously saved active pack. */
 CHECK(!model.values.hdEnabled);
 model.packAction=1;HdPackUiTick(&model);
 for(int i=0;i<2000&&!model.values.hdEnabled;i++){SDL_Delay(1);HdPackUiTick(&model);}
 CHECK(applied==2&&restarted==2&&layerEnabled&&model.values.hdEnabled);
 model.packAction=1;HdPackUiTick(&model);CHECK(applied==3&&restarted==3&&!layerEnabled&&!model.values.hdEnabled);
 model.packAction=1;HdPackUiTick(&model);for(int i=0;i<2000&&!model.values.hdEnabled;i++){SDL_Delay(1);HdPackUiTick(&model);}
 CHECK(applied==4&&restarted==4&&layerEnabled&&model.values.hdEnabled);
 /* Persisted settings can change even when presentation recreation fails.
  * The UI must distinguish that failure from successful activation. */
 restartSuccess=0;
 model.packAction=1;HdPackUiTick(&model);
 CHECK(!model.values.hdEnabled);
 CHECK(strstr(model.packStatus,"could not refresh textures"));
 CHECK(HdPackReadState(store,"PAL",&b)&&!b.enabled);
 model.packAction=1;HdPackUiTick(&model);
 for(int i=0;i<2000&&!model.values.hdEnabled;i++){SDL_Delay(1);HdPackUiTick(&model);}
 CHECK(model.values.hdEnabled);
 CHECK(strstr(model.packStatus,"could not refresh textures"));
 CHECK(HdPackReadState(store,"PAL",&b)&&b.enabled);
 restartSuccess=1;
 model.packAction=5;HdPackUiTick(&model);
 for(int i=0;i<2000&&model.values.hdBusy;i++){SDL_Delay(1);HdPackUiTick(&model);}
 CHECK(!model.values.hdBusy&&strstr(model.packStatus,"server is not configured"));
 CHECK(HdPackReadState(store,"PAL",&b)&&b.enabled);
 snprintf(path,sizeof(path),"%s/textures/font.png",b.directory);CHECK(SDL_SaveFile(path,"broken",6));CHECK(!HdPackVerify(store,&b,NULL,NULL,error,sizeof(error)));
 /* A full-size shared-texture manifest must survive the native archive path,
  * including the old 512-entry and one-MiB manifest limits. */
 snprintf(path,sizeof(path),"%s/mod.toml",source);
 SDL_IOStream *large=SDL_IOFromFile(path,"wb");CHECK(large);
 CHECK(SDL_IOprintf(large,"[textures]\n")>0);
 for(unsigned i=0;i<RAGE_MOD_MANIFEST_MAX_TEXTURES;i++)
  CHECK(SDL_IOprintf(large,"\"track.big1.terrain.material.%u.variant.2\" = \"textures/font.png\"\n",i)>0);
 CHECK(SDL_CloseIO(large));
 RageModManifest *parsed=HdPackReadManifest(source);CHECK(parsed);
 CHECK(parsed->textureCount==RAGE_MOD_MANIFEST_MAX_TEXTURES);
 char last[96];snprintf(last,sizeof(last),"track.big1.terrain.material.%u.variant.2",RAGE_MOD_MANIFEST_MAX_TEXTURES-1);
 CHECK(ModManifestFindTexture(parsed,last)&&!strcmp(ModManifestFindTexture(parsed,last),"textures/font.png"));free(parsed);
 snprintf(archive,sizeof(archive),"%s/large.rrhd",root);
 snprintf(store,sizeof(store),"%s/large-store",root);
 CHECK(HdPackCreate(source,archive,error,sizeof(error)));
 CHECK(HdPackInstall(archive,store,"PAL",NULL,NULL,error,sizeof(error)));
 CHECK(HdPackReadState(store,"PAL",&a));CHECK(HdPackVerify(store,&a,NULL,NULL,error,sizeof(error)));
 parsed=HdPackReadManifest(a.directory);CHECK(parsed&&parsed->textureCount==RAGE_MOD_MANIFEST_MAX_TEXTURES);
 CHECK(ModManifestFindTexture(parsed,last));free(parsed);
 puts("HD pack checksums, cancellation, region, atomic state and full manifest capacity passed");return 0;
}
