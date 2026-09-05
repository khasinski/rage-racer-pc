#include "rmesh_cache.h"

#include <string.h>
#include <stdlib.h>

int RuntimeCachedMeshAdopt(RageRuntimeCachedMesh *entry, const void *bytes,
                          size_t size, RageRuntimeFreeFile releaseBytes,
                          void *releaseContext) {
    RageRuntimeMesh mesh;
    RageRuntimeMeshBounds *bounds;
    if (entry == NULL || entry->ownedBytes != NULL || entry->mesh.bytes != NULL ||
        !RuntimeMeshOpen(&mesh, bytes, size)) return 0;
    bounds = calloc(mesh.meshCount, sizeof(*bounds));
    if (bounds != NULL) RuntimeMeshPrepareBounds(&mesh, bounds, mesh.meshCount);
    entry->mesh = mesh;
    entry->ownedBytes = bytes;
    entry->ownedBounds = bounds;
    entry->releaseBytes = releaseBytes;
    entry->releaseContext = releaseContext;
    return 1;
}

void RuntimeCachedMeshRelease(RageRuntimeCachedMesh *entry) {
    if (entry == NULL) return;
    if (entry->ownedBytes != NULL && entry->releaseBytes != NULL)
        entry->releaseBytes(entry->releaseContext, entry->ownedBytes);
    free(entry->ownedBounds);
    memset(entry, 0, sizeof(*entry));
}

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
    RageRuntimeCachedMesh pending = {0};
    if (!RuntimeCachedMeshAdopt(&pending, bytes, size,
                                cache->freeFile, cache->context)) {
        if (cache->freeFile != NULL) cache->freeFile(cache->context, bytes);
        return NULL;
    }
    pending.assetKey = assetKey;
    pending.assetSet = assetSet;
    pending.location = location;
    cache->entries[cache->count] = pending;
    return &cache->entries[cache->count++];
}

void RuntimeMeshCacheRelease(RageRuntimeMeshCache *cache) {
    uint32_t count;
    uint32_t i;
    if (cache == NULL) return;
    count = cache->count < cache->capacity ? cache->count : cache->capacity;
    if (cache->entries != NULL) {
        for (i = 0; i < count; i++) {
            RuntimeCachedMeshRelease(&cache->entries[i]);
        }
    }
    cache->count = 0;
}
