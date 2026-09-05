#include "rmesh_cache.h"

#include <string.h>
#include <stdlib.h>

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
    RageRuntimeCachedMesh *entry = &cache->entries[cache->count];
    entry->ownedBounds = calloc(entry->mesh.meshCount, sizeof(*entry->ownedBounds));
    if (entry->ownedBounds != NULL)
        RuntimeMeshPrepareBounds(&entry->mesh, entry->ownedBounds, entry->mesh.meshCount);
    return &cache->entries[cache->count++];
}

void RuntimeMeshCacheRelease(RageRuntimeMeshCache *cache) {
    uint32_t count;
    uint32_t i;
    if (cache == NULL) return;
    count = cache->count < cache->capacity ? cache->count : cache->capacity;
    if (cache->entries != NULL) {
        for (i = 0; i < count; i++) {
            if (cache->freeFile != NULL)
                cache->freeFile(cache->context, cache->entries[i].ownedBytes);
            free(cache->entries[i].ownedBounds);
            memset(&cache->entries[i], 0, sizeof(cache->entries[i]));
        }
    }
    cache->count = 0;
}
