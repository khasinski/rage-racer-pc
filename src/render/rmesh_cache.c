#include "rmesh_cache.h"

#include <string.h>

void RuntimeMeshCacheInit(RageRuntimeMeshCache *cache,
                          const char *indexText, size_t indexSize,
                          RageRuntimeReadFile readFile,
                          RageRuntimeFreeFile freeFile, void *context,
                          RageRuntimeCachedMesh *entries,
                          uint32_t capacity) {
    if (cache == NULL) return;
    memset(cache, 0, sizeof(*cache));
    cache->indexText = indexText;
    cache->indexSize = indexSize;
    cache->readFile = readFile;
    cache->freeFile = freeFile;
    cache->context = context;
    cache->entries = entries;
    cache->capacity = entries != NULL ? capacity : 0;
}

const RageRuntimeCachedMesh *RuntimeMeshCacheFind(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet) {
    RageRuntimeAssetLocation location;
    const void *bytes;
    size_t size;
    uint32_t i;

    if (cache == NULL || cache->entries == NULL ||
        cache->count > cache->capacity) return NULL;
    for (i = 0; i < cache->count; i++) {
        RageRuntimeCachedMesh *entry = &cache->entries[i];
        if (entry->assetKey == assetKey && entry->assetSet == assetSet) {
            return entry;
        }
    }
    if (cache->count >= cache->capacity || cache->readFile == NULL ||
        !RuntimeIndexFind(cache->indexText, cache->indexSize, assetKey,
                              assetSet, &location) ||
        !cache->readFile(cache->context, location.meshPath,
                         location.meshPathLength, &bytes, &size)) {
        return NULL;
    }
    if (!RuntimeMeshOpen(&cache->entries[cache->count].mesh, bytes, size)) {
        if (cache->freeFile != NULL) cache->freeFile(cache->context, bytes);
        return NULL;
    }
    cache->entries[cache->count].assetKey = assetKey;
    cache->entries[cache->count].assetSet = assetSet;
    cache->entries[cache->count].ownedBytes = bytes;
    cache->entries[cache->count].location = location;
    return &cache->entries[cache->count++];
}

void RuntimeMeshCacheRelease(RageRuntimeMeshCache *cache) {
    uint32_t i;
    if (cache == NULL) return;
    if (cache->freeFile != NULL) {
        for (i = 0; i < cache->count; i++) {
            cache->freeFile(cache->context, cache->entries[i].ownedBytes);
        }
    }
    cache->count = 0;
}
