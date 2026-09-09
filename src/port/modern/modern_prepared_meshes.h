#ifndef RAGE_MODERN_PREPARED_MESHES_H
#define RAGE_MODERN_PREPARED_MESHES_H

#include "render/render_world.h"
#include "render/rmesh.h"

#include <stdint.h>

typedef const RageRuntimeMesh *(*ModernPreparedMeshResolve)(
    void *context, const RageRenderMeshInstance *instance);

typedef struct ModernPreparedMeshes {
    const RageRenderWorld *world;
    const RageRuntimeMesh **items;
    uint32_t count;
    uint32_t capacity;
} ModernPreparedMeshes;

/* Snapshot all main-pass mesh lookups once, before building either camera.
 * Returns zero without changing a usable cache when allocation fails. */
int ModernPreparedMeshesPrepare(ModernPreparedMeshes *cache,
                                const RageRenderWorld *world,
                                ModernPreparedMeshResolve resolve,
                                void *context);
const RageRuntimeMesh *ModernPreparedMeshesLookup(
    const ModernPreparedMeshes *cache,
    const RageRenderMeshInstance *instance);
void ModernPreparedMeshesRelease(ModernPreparedMeshes *cache);

#endif
