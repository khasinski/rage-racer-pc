#include "session_provider.h"
#include <string.h>
static RageSessionProvider s_provider;
static uint64_t s_epoch;
void SessionProviderSet(const RageSessionProvider*p){if(p)s_provider=*p;else memset(&s_provider,0,sizeof(s_provider));++s_epoch;}
int SessionProviderActive(void){return s_provider.world!=NULL;}
const RenderWorld *SessionProviderWorld(float aspect){return s_provider.world?s_provider.world(s_provider.context,aspect):NULL;}
const RageRuntimeCachedMesh *SessionProviderMesh(const RenderMeshInstance*i){return s_provider.mesh?s_provider.mesh(s_provider.context,i):NULL;}
int SessionProviderMaterial(const RenderMeshInstance*i,uint32_t m,uint8_t v,RageRenderMaterial*d,void**p,uint32_t*w,uint32_t*h){return s_provider.material&&s_provider.material(s_provider.context,i,m,v,d,p,w,h);}
int SessionProviderSky(void**p,uint32_t*w,uint32_t*h){return s_provider.sky&&s_provider.sky(s_provider.context,p,w,h);}
uint64_t SessionProviderRevision(void){return (s_epoch<<32)^(s_provider.revision?s_provider.revision(s_provider.context):0);}
int SessionProviderTachometer(RageRetailTachometer *frame){return frame&&s_provider.tachometer&&s_provider.tachometer(s_provider.context,frame);}
