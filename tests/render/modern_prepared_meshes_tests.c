#include "modern_prepared_meshes.h"

#include <assert.h>
#include <string.h>

static RageRuntimeMesh s_meshes[4];

static const RageRuntimeMesh *Resolve(void *context,
                                      const RenderMeshInstance *instance) {
    if (context) ++*(unsigned *)context;
    assert(instance->assetKey < 4);
    return &s_meshes[instance->assetKey];
}

int main(void) {
    ModernPreparedMeshes cache = {0};
    unsigned resolutions = 0;
    RenderMeshInstance firstInstances[2] = {
        {.assetKey = 1, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 2, .pass = RAGE_RENDER_PASS_MIRROR},
    };
    RenderMeshInstance secondInstances[3] = {
        {.assetKey = 3, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 2, .pass = RAGE_RENDER_PASS_MAIN},
        {.assetKey = 1, .pass = RAGE_RENDER_PASS_MIRROR},
    };
    RenderMeshInstance outsider = {.assetKey = 1};
    RenderWorld first = {.instances = firstInstances, .instanceCount = 2, .instanceCapacity = 2};
    RenderWorld second = {.instances = secondInstances, .instanceCount = 3, .instanceCapacity = 3};

    memset(s_meshes, 0, sizeof(s_meshes));
    assert(!ModernPreparedMeshesPrepare(NULL, &first, Resolve, NULL));
    assert(!ModernPreparedMeshesPrepare(&cache, NULL, Resolve, NULL));
    assert(ModernPreparedMeshesPrepare(&cache, &first, Resolve, &resolutions));
    assert(resolutions == 1);
    assert(cache.count == 2 && cache.capacity >= 2);
    assert(ModernPreparedMeshesLookup(&cache, &firstInstances[0]) ==
           &s_meshes[1]);
    assert(ModernPreparedMeshesLookup(&cache, &firstInstances[1]) == NULL);
    assert(ModernPreparedMeshesLookup(&cache, &outsider) == NULL);
    for (unsigned lookup = 0; lookup < 100; ++lookup)
        assert(ModernPreparedMeshesLookup(&cache, &firstInstances[0]) == &s_meshes[1]);
    assert(resolutions == 1); /* Validation reuses prepared pointers, not resolver calls. */

    assert(ModernPreparedMeshesPrepare(&cache, &second, Resolve, &resolutions));
    assert(resolutions == 3);
    assert(cache.count == 3 && cache.capacity >= 3);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[0]) ==
           &s_meshes[3]);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[1]) ==
           &s_meshes[2]);
    assert(ModernPreparedMeshesLookup(&cache, &secondInstances[2]) == NULL);

RenderMeshInstance unrelated = {0};
assert(ModernPreparedMeshesLookup(&cache, &unrelated) == NULL);
RenderWorld invalid = {.instanceCount = 1, .instanceCapacity = 1};
const RenderWorld *savedWorld = cache.world;
assert(!ModernPreparedMeshesPrepare(&cache, &invalid, Resolve, NULL));
assert(cache.world == savedWorld);
    ModernPreparedMeshesRelease(&cache);
    assert(cache.items == NULL && cache.world == NULL && cache.count == 0 &&
           cache.capacity == 0);
    return 0;
}
