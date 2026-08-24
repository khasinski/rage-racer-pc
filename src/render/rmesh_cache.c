#include "rmesh_cache.h"

#include <string.h>

void RageRuntimeMeshCacheInit(RageRuntimeMeshCache *cache,
                              const char *indexText, size_t indexSize,
                              RageRuntimeReadFile readFile,
                              RageRuntimeFreeFile freeFile, void *context,
                              RageRuntimeCachedMesh *entries,
                              uint32_t capacity) {
    memset(cache, 0, sizeof(*cache));
    cache->indexText = indexText;
    cache->indexSize = indexSize;
    cache->readFile = readFile;
    cache->freeFile = freeFile;
    cache->context = context;
    cache->entries = entries;
    cache->capacity = capacity;
}

const RageRuntimeCachedMesh *RageRuntimeMeshCacheFind(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet) {
    RageRuntimeAssetLocation location;
    const void *bytes;
    size_t size;
    uint32_t i;

    if (cache == 0) return 0;
    for (i = 0; i < cache->count; i++) {
        RageRuntimeCachedMesh *entry = &cache->entries[i];
        if (entry->assetKey == assetKey && entry->assetSet == assetSet) {
            return entry;
        }
    }
    if (cache->count == cache->capacity || cache->readFile == 0 ||
        !RageRuntimeIndexFind(cache->indexText, cache->indexSize, assetKey,
                              assetSet, &location) ||
        !cache->readFile(cache->context, location.meshPath,
                         location.meshPathLength, &bytes, &size)) {
        return 0;
    }
    if (!RageRuntimeMeshOpen(&cache->entries[cache->count].mesh, bytes, size)) {
        if (cache->freeFile != 0) cache->freeFile(cache->context, bytes);
        return 0;
    }
    cache->entries[cache->count].assetKey = assetKey;
    cache->entries[cache->count].assetSet = assetSet;
    cache->entries[cache->count].ownedBytes = bytes;
    cache->entries[cache->count].location = location;
    return &cache->entries[cache->count++];
}

void RageRuntimeMeshCacheRelease(RageRuntimeMeshCache *cache) {
    uint32_t i;
    if (cache == 0) return;
    if (cache->freeFile != 0) {
        for (i = 0; i < cache->count; i++) {
            cache->freeFile(cache->context, cache->entries[i].ownedBytes);
        }
    }
    cache->count = 0;
}

