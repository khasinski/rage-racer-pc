#include "modern_prepared_meshes.h"

#include <SDL3/SDL.h>

#include <stddef.h>
#include <string.h>

int ModernPreparedMeshesPrepare(ModernPreparedMeshes *cache,
                                const RageRenderWorld *world,
                                ModernPreparedMeshResolve resolve,
                                void *context) {
    const RageRuntimeMesh **items;
    size_t bytes;
    uint32_t i;
    if (!cache || !world || !resolve) return 0;
    if (world->instanceCount > cache->capacity) {
        if (!SDL_size_mul_check_overflow(world->instanceCount,
                                         sizeof(*cache->items), &bytes))
            return 0;
        items = SDL_realloc(cache->items, bytes);
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
    const RageRenderMeshInstance *instance) {
    ptrdiff_t index;
    if (!cache || !cache->world || !instance) return NULL;
    index = instance - cache->world->instances;
    if (index < 0 || (uint32_t)index >= cache->count) return NULL;
    return cache->items[index];
}

void ModernPreparedMeshesRelease(ModernPreparedMeshes *cache) {
    if (!cache) return;
    SDL_free(cache->items);
    memset(cache, 0, sizeof(*cache));
}
