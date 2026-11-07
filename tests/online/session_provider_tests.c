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
static int getTachometer(void *ctx,RageRetailTachometer *frame){if(ctx!=&world)return 0;memset(frame,0,sizeof(*frame));frame->revision=17;return 1;}
int main(void){
    CHECK(on_camera_size()==sizeof(RenderCamera));CHECK(on_instance_size()==sizeof(RenderMeshInstance));
    CHECK(!SessionProviderActive());CHECK(!SessionProviderWorld(1));
    uint64_t before=SessionProviderRevision();
    RageSessionProvider p={.context=&world,.world=getWorld,.mesh=getMesh,.revision=revision,.tachometer=getTachometer};
    SessionProviderSet(&p);memset(&p,0,sizeof(p));
    CHECK(SessionProviderActive());CHECK(SessionProviderWorld(1)==&world);
    CHECK(SessionProviderRevision()!=before);
    RageRetailTachometer tachometer;CHECK(SessionProviderTachometer(&tachometer)&&tachometer.revision==17);
    RenderMeshInstance instance={.assetKey=0x80000012u,.flags=RAGE_SESSION_INSTANCE};
    CHECK(SessionProviderMesh(&instance)==&mesh);
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
