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

/* Owns bytes (through the recorded provider) and optional bounds. Do not copy
 * a live owner except as an explicit move; consumers borrow its mesh view. */
typedef struct RageRuntimeCachedMesh {
    uint32_t assetKey;
    RageRenderAssetSet assetSet;
    RageRuntimeMesh mesh;
    const void *ownedBytes;
    RageRuntimeAssetLocation location;
    RageRuntimeMeshBounds *ownedBounds;
    RageRuntimeFreeFile releaseBytes;
    void *releaseContext;
} RageRuntimeCachedMesh;

/* Zero-initialize before adoption. Ownership transfers only on success;
 * failure leaves both the entry and caller's bytes untouched. An occupied
 * entry cannot be replaced: borrowed mesh pointers live until explicit release.
 * A NULL release callback borrows bytes but still owns prepared bounds. */
int RuntimeCachedMeshAdopt(RageRuntimeCachedMesh *entry, const void *bytes,
                          size_t size, RageRuntimeFreeFile releaseBytes,
                          void *releaseContext);
/* Idempotent; invalidates all borrowed mesh/bounds views of this entry. */
void RuntimeCachedMeshRelease(RageRuntimeCachedMesh *entry);

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
/* Returned entry and its mesh views remain valid until cache release. */
const RageRuntimeCachedMesh *RuntimeMeshCacheFind(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet);
void RuntimeMeshCacheRelease(RageRuntimeMeshCache *cache);

#endif
