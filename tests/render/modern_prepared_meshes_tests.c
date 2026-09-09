#include "modern_prepared_meshes.h"

#include <assert.h>
#include <string.h>

static RageRuntimeMesh s_meshes[4];

static const RageRuntimeMesh *Resolve(void *context,
                                      const RageRenderMeshInstance *instance) {
    (void)context;
    assert(instance->assetKey < 4);
    return &s_meshes[instance->assetKey];
}

int main(void) {
    ModernPreparedMeshes cache = {0};
    RageRenderMeshInstance firstInstances[2] = {
        {.assetKey = 1, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 2, .pass = RAGE_RENDER_PASS_MIRROR},
    };
    RageRenderMeshInstance secondInstances[3] = {
        {.assetKey = 3, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 2, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 1, .pass = RAGE_RENDER_PASS_MIRROR},
    };
    RageRenderMeshInstance outsider = {.assetKey = 1};
    RageRenderWorld first = {.instances = firstInstances, .instanceCount = 2};
    RageRenderWorld second = {.instances = secondInstances, .instanceCount = 3};

    memset(s_meshes, 0, sizeof(s_meshes));
    assert(!ModernPreparedMeshesPrepare(NULL, &first, Resolve, NULL));
    assert(!ModernPreparedMeshesPrepare(&cache, NULL, Resolve, NULL));
    assert(ModernPreparedMeshesPrepare(&cache, &first, Resolve, NULL));
    assert(cache.count == 2 && cache.capacity >= 2);
    assert(ModernPreparedMeshesLookup(&cache, &firstInstances[0]) ==
           &s_meshes[1]);
    assert(ModernPreparedMeshesLookup(&cache, &firstInstances[1]) == NULL);
    assert(ModernPreparedMeshesLookup(&cache, &outsider) == NULL);

    assert(ModernPreparedMeshesPrepare(&cache, &second, Resolve, NULL));
    assert(cache.count == 3 && cache.capacity >= 3);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[0]) ==
           &s_meshes[3]);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[1]) ==
           &s_meshes[2]);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[2]) == NULL);

    ModernPreparedMeshesRelease(&cache);
    assert(cache.items == NULL && cache.world == NULL && cache.count == 0 &&
           cache.capacity == 0);
    return 0;
}
