#ifndef RAGE_SESSION_PROVIDER_H
#define RAGE_SESSION_PROVIDER_H
#include "render_world.h"
#include "rmesh_cache.h"
#include "render_material.h"
/* An explicit race owner supplies immutable meshes and decoded material
 * pixels. No legacy global/importer fallback is allowed for its instances. */
typedef struct RageSessionProvider {
    void *context;
    const RenderWorld *(*world)(void *,float aspect);
    const RageRuntimeCachedMesh *(*mesh)(void *,const RenderMeshInstance *);
    int (*material)(void *,const RenderMeshInstance *,uint32_t,uint8_t,RageRenderMaterial *,void **,uint32_t *,uint32_t *);
    int (*sky)(void *,void **,uint32_t *,uint32_t *);
    uint64_t (*revision)(void *);
} RageSessionProvider;
void SessionProviderSet(const RageSessionProvider *provider);
int SessionProviderActive(void);
const RenderWorld *SessionProviderWorld(float aspect);
const RageRuntimeCachedMesh *SessionProviderMesh(const RenderMeshInstance *instance);
int SessionProviderMaterial(const RenderMeshInstance *,uint32_t,uint8_t,RageRenderMaterial *,void **,uint32_t *,uint32_t *);
int SessionProviderSky(void **,uint32_t *,uint32_t *);
uint64_t SessionProviderRevision(void);
#endif
