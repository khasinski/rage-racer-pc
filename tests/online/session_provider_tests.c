#include "render/session_provider.h"
#include "online/core_api.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static RenderWorld world;
static RageRuntimeCachedMesh mesh;
static const RenderWorld *getWorld(void *ctx,float aspect){return ctx==&world&&aspect>0?ctx:NULL;}
static const RageRuntimeCachedMesh *getMesh(void *ctx,const RenderMeshInstance *i){return ctx==&world&&i->assetKey==0x80000012u?&mesh:NULL;}
static uint64_t revision(void *ctx){(void)ctx;return 7;}
static uint64_t content=11;
static uint64_t contentRevision(void *ctx){(void)ctx;return content;}
static uint64_t materialRevision(void *ctx,const RenderMeshInstance *i,uint32_t m,uint8_t v){(void)ctx;(void)m;(void)v;return i->assetSet==RAGE_RENDER_ASSET_TERRAIN?content:0;}
static int getTachometer(void *ctx,RageRetailTachometer *frame){if(ctx!=&world)return 0;memset(frame,0,sizeof(*frame));frame->revision=17;return 1;}
int main(void){
    CHECK(on_camera_size()==sizeof(RenderCamera));CHECK(on_instance_size()==sizeof(RenderMeshInstance));
    CHECK(!SessionProviderActive());CHECK(!SessionProviderWorld(1));
    uint64_t before=SessionProviderRevision();
    RageSessionProvider p={.context=&world,.world=getWorld,.mesh=getMesh,.revision=revision,.tachometer=getTachometer,.contentRevision=contentRevision,.materialRevision=materialRevision};
    SessionProviderSet(&p);memset(&p,0,sizeof(p));
    CHECK(SessionProviderActive());CHECK(SessionProviderWorld(1)==&world);
    CHECK(SessionProviderRevision()!=before);
    RageRetailTachometer tachometer;CHECK(SessionProviderTachometer(&tachometer)&&tachometer.revision==17);
    RenderMeshInstance instance={.assetKey=0x80000012u,.flags=RAGE_SESSION_INSTANCE};
    CHECK(SessionProviderMesh(&instance)==&mesh);
    uint64_t stable=SessionProviderRevision();content++;
    CHECK(SessionProviderRevision()==stable&&SessionProviderContentRevision()==12);
    CHECK(SessionProviderMaterialRevision(&instance,0,0)==0);
    RenderMeshInstance terrain={.assetSet=RAGE_RENDER_ASSET_TERRAIN};
    CHECK(SessionProviderMaterialRevision(&terrain,0,0)==12);
    instance.assetKey++;CHECK(!SessionProviderMesh(&instance));
    RageRenderMaterial definition={0};void *pixels=(void*)&world;uint32_t w=19,h=17;
    CHECK(!SessionProviderMaterial(&instance,0,0,&definition,&pixels,&w,&h));
    CHECK(pixels==&world&&w==19&&h==17);
    before=SessionProviderRevision();SessionProviderSet(NULL);
    CHECK(!SessionProviderActive()&&!SessionProviderMesh(&instance));
    CHECK(!SessionProviderTachometer(&tachometer));
    CHECK(SessionProviderRevision()!=before);
    puts("online renderer ABI, provider ownership and removal passed");return 0;
}
