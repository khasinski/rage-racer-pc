#include "modern_prepared_meshes.h"

#include <stdlib.h>

#include <stddef.h>
#include <string.h>

int ModernPreparedMeshesPrepare(ModernPreparedMeshes *cache,
                                const RenderWorld *world,
                                ModernPreparedMeshResolve resolve,
                                void *context) {
    const RageRuntimeMesh **items;
    size_t bytes;
    uint32_t i;
    if (!cache || !world || !resolve || world->instanceCount > world->instanceCapacity ||
        (world->instanceCount && !world->instances)) return 0;
    if (world->instanceCount > cache->capacity) {
        if (world->instanceCount && sizeof(*cache->items) > SIZE_MAX / world->instanceCount) return 0;
        bytes = (size_t)world->instanceCount * sizeof(*cache->items);
        items = realloc(cache->items, bytes);
        if (!items) return 0;
        cache->items = items;
        cache->capacity = world->instanceCount;
    }
    for (i = 0; i < world->instanceCount; ++i)
        cache->items[i] = world->instances[i].pass == RAGE_RENDER_PASS_MAIN
            ? resolve(context, &world->instances[i]) : NULL;
    cache->world = world;
    cache->count = world->instanceCount;
    return 1;
}

const RageRuntimeMesh *ModernPreparedMeshesLookup(
    const ModernPreparedMeshes *cache,
    const RenderMeshInstance *instance) {
    uintptr_t offset;
    if (!cache || !cache->world || !cache->world->instances || !instance) return NULL;
    uintptr_t base = (uintptr_t)cache->world->instances, address = (uintptr_t)instance;
    if (address < base) return NULL;
    offset = address - base;
    if (offset % sizeof(*instance) || offset / sizeof(*instance) >= cache->count) return NULL;
    return cache->items[offset / sizeof(*instance)];
}

void ModernPreparedMeshesRelease(ModernPreparedMeshes *cache) {
    if (!cache) return;
    free(cache->items);
    memset(cache, 0, sizeof(*cache));
}
