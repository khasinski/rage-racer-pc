#include "rmesh_cache.h"

#include <string.h>
#include <stdlib.h>

/* Publish only a complete decoded asset. On allocation failure the validated
 * wire reader remains usable; this never changes renderer selection. */
static void PrepareGeometry(RageRuntimeCachedMesh *entry) {
    RageRuntimeMesh *mesh = &entry->mesh;
    RageRuntimeVertex *vertices;
    uint32_t *indices;
    if (mesh->vertexCount == 0 || mesh->indexCount == 0 ||
        sizeof(*vertices) > SIZE_MAX / mesh->vertexCount ||
        sizeof(*indices) > SIZE_MAX / mesh->indexCount) return;
    vertices = malloc((size_t)mesh->vertexCount * sizeof(*vertices));
    indices = malloc((size_t)mesh->indexCount * sizeof(*indices));
    if (vertices == NULL || indices == NULL) {
        free(vertices);
        free(indices);
        return;
    }
    for (uint32_t i = 0; i < mesh->vertexCount; ++i)
        if (!RuntimeMeshVertex(mesh, i, &vertices[i])) goto fail;
    for (uint32_t i = 0; i < mesh->indexCount; ++i)
        if (!RuntimeMeshIndex(mesh, i, &indices[i])) goto fail;
    entry->ownedVertices = vertices;
    entry->ownedIndices = indices;
    mesh->vertices = vertices;
    mesh->indices = indices;
    return;
fail:
    free(vertices);
    free(indices);
}

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
    PrepareGeometry(entry);
    return 1;
}

void RuntimeCachedMeshRelease(RageRuntimeCachedMesh *entry) {
    if (entry == NULL) return;
    if (entry->ownedBytes != NULL && entry->releaseBytes != NULL)
        entry->releaseBytes(entry->releaseContext, entry->ownedBytes);
    free(entry->ownedBounds);
    free(entry->ownedVertices);
    free(entry->ownedIndices);
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

const RageRuntimeCachedMesh *RuntimeMeshCachePeek(const RageRuntimeMeshCache *cache,
    uint32_t assetKey, RageRenderAssetSet assetSet) {
    if (cache == NULL || cache->entries == NULL || cache->count > cache->capacity) return NULL;
    for (uint32_t i = 0; i < cache->count; ++i) {
        const RageRuntimeCachedMesh *entry = &cache->entries[i];
        if (entry->assetKey == assetKey && entry->assetSet == assetSet && entry->mesh.bytes != NULL)
            return entry;
    }
    return NULL;
}

RageRuntimeMeshStatus RuntimeMeshCacheResolve(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet,
    const RageRuntimeCachedMesh **out) {
    RageRuntimeAssetLocation location;
    const void *bytes;
    size_t size;

    if (out == NULL) return RAGE_RUNTIME_MESH_ERROR;
    *out = NULL;
    if (cache == NULL || cache->entries == NULL ||
        cache->count > cache->capacity) return RAGE_RUNTIME_MESH_ERROR;
    const RageRuntimeCachedMesh *resident = RuntimeMeshCachePeek(cache, assetKey, assetSet);
    if (resident != NULL) { *out = resident; return RAGE_RUNTIME_MESH_READY; }
    if (!RuntimeIndexFind(cache->indexText, cache->indexSize, assetKey,
                         assetSet, &location)) return RAGE_RUNTIME_MESH_MISSING;
    if (cache->count >= cache->capacity || cache->readFile == NULL ||
        !cache->readFile(cache->context, location.meshPath,
                         location.meshPathLength, &bytes, &size)) {
        return RAGE_RUNTIME_MESH_ERROR;
    }
    RageRuntimeCachedMesh pending = {0};
    if (!RuntimeCachedMeshAdopt(&pending, bytes, size,
                                cache->freeFile, cache->context)) {
        if (cache->freeFile != NULL) cache->freeFile(cache->context, bytes);
        return RAGE_RUNTIME_MESH_ERROR;
    }
    pending.assetKey = assetKey;
    pending.assetSet = assetSet;
    pending.location = location;
    cache->entries[cache->count] = pending;
    *out = &cache->entries[cache->count++];
    return RAGE_RUNTIME_MESH_READY;
}

const RageRuntimeCachedMesh *RuntimeMeshCacheFind(
    RageRuntimeMeshCache *cache, uint32_t assetKey, RageRenderAssetSet assetSet) {
    const RageRuntimeCachedMesh *result = NULL;
    RuntimeMeshCacheResolve(cache, assetKey, assetSet, &result);
    return result;
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
