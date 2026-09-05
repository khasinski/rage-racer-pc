#ifndef RAGE_RMESH_CACHE_H
#define RAGE_RMESH_CACHE_H

#include <stddef.h>
#include <stdint.h>

#include "rmesh.h"
#include "rmesh_index.h"

typedef int (*RageRuntimeReadFile)(void *context, const char *path,
                                   size_t pathLength, const void **bytes,
                                   size_t *size);
typedef void (*RageRuntimeFreeFile)(void *context, const void *bytes);

typedef struct RageRuntimeCachedMesh {
    uint32_t assetKey;
    RageRenderAssetSet assetSet;
    RageRuntimeMesh mesh;
    const void *ownedBytes;
    RageRuntimeAssetLocation location;
    RageRuntimeMeshBounds *ownedBounds;
} RageRuntimeCachedMesh;

typedef struct RageRuntimeMeshCache {
    const char *indexText;
    size_t indexSize;
    RageRuntimeReadFile readFile;
    RageRuntimeFreeFile freeFile;
    void *context;
    RageRuntimeCachedMesh *entries;
    uint32_t capacity;
    uint32_t count;
} RageRuntimeMeshCache;

void RuntimeMeshCacheInit(RageRuntimeMeshCache *cache,
                              const char *indexText, size_t indexSize,
                              RageRuntimeReadFile readFile,
                              RageRuntimeFreeFile freeFile, void *context,
                              RageRuntimeCachedMesh *entries,
                              uint32_t capacity);
const RageRuntimeCachedMesh *RuntimeMeshCacheFind(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet);
void RuntimeMeshCacheRelease(RageRuntimeMeshCache *cache);

#endif
