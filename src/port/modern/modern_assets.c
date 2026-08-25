#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "modern_assets.h"

enum { MODERN_ASSET_CACHE_CAPACITY = 4096 };

static char s_root[1024];
static void *s_indexBytes;
static size_t s_indexSize;
static RageRuntimeCachedMesh s_entries[MODERN_ASSET_CACHE_CAPACITY];
static RageRuntimeMeshCache s_cache;
static int s_initialized;
static int s_ready;
/* Material sidecars are read synchronously on the render thread. Copy the
 * selected relative path out before releasing their transient file buffer. */
static char s_materialPath[1024];

static int ModernAssetReadFile(void *context, const char *path,
                               size_t pathLength, const void **bytes,
                               size_t *size) {
    char fullPath[sizeof(s_root) + 1024];
    size_t rootLength = strlen(s_root);
    void *file;
    (void)context;
    if (rootLength + 1 + pathLength + 1 > sizeof(fullPath)) return 0;
    memcpy(fullPath, s_root, rootLength);
    fullPath[rootLength] = '/';
    memcpy(fullPath + rootLength + 1, path, pathLength);
    fullPath[rootLength + 1 + pathLength] = '\0';
    file = SDL_LoadFile(fullPath, size);
    if (file == NULL) return 0;
    *bytes = file;
    return 1;
}

static void ModernAssetFreeFile(void *context, const void *bytes) {
    (void)context;
    SDL_free((void *)bytes);
}

void ModernAssetsInit(void) {
    const char *root;
    char indexPath[sizeof(s_root) + 32];
    size_t rootLength;

    if (s_initialized) return;
    s_initialized = 1;
    root = getenv("RAGE_PORT_MODERN_ASSETS");
    if (root == NULL || root[0] == '\0') return;
    rootLength = strlen(root);
    if (rootLength == 0 || rootLength >= sizeof(s_root) ||
        rootLength + sizeof("/runtime-index.txt") > sizeof(indexPath)) {
        fprintf(stderr, "rage-port: native asset path is too long\n");
        return;
    }
    memcpy(s_root, root, rootLength + 1);
    snprintf(indexPath, sizeof(indexPath), "%s/runtime-index.txt", s_root);
    s_indexBytes = SDL_LoadFile(indexPath, &s_indexSize);
    if (s_indexBytes == NULL) {
        fprintf(stderr, "rage-port: native asset index unavailable: %s\n", indexPath);
        return;
    }
    RageRuntimeMeshCacheInit(&s_cache, s_indexBytes, s_indexSize,
                             ModernAssetReadFile, ModernAssetFreeFile, NULL,
                             s_entries, MODERN_ASSET_CACHE_CAPACITY);
    s_ready = 1;
    fprintf(stderr, "rage-port: native asset cache %s\n", s_root);
}

void ModernAssetsShutdown(void) {
    RageRuntimeMeshCacheRelease(&s_cache);
    if (s_indexBytes != NULL) SDL_free(s_indexBytes);
    s_indexBytes = NULL;
    s_indexSize = 0;
    s_ready = 0;
    s_initialized = 0;
    s_root[0] = '\0';
}

const RageRuntimeCachedMesh *ModernAssetsFind(
    const RageRenderMeshInstance *instance) {
    if (!s_ready || instance == NULL) return NULL;
    return RageRuntimeMeshCacheFind(&s_cache, instance->assetKey,
                                    instance->assetSet);
}

int ModernAssetsReady(void) {
    return s_ready;
}

uint32_t ModernAssetsCachedMeshCount(void) {
    return s_ready ? s_cache.count : 0;
}

const RageRuntimeMesh *ModernAssetsMeshLookup(
    void *context, const RageRenderMeshInstance *instance) {
    const RageRuntimeCachedMesh *cached;
    (void)context;
    cached = ModernAssetsFind(instance);
    return cached != NULL ? &cached->mesh : NULL;
}

static int ModernAssetsFindMaterial(const RageRenderMeshInstance *instance,
                                    uint32_t material, uint8_t variant,
                                    const char **pathOut,
                                    size_t *pathLengthOut,
                                    ModernAssetMaterialSource *sourceOut) {
    const RageRuntimeCachedMesh *cached;
    const void *mapBytes;
    size_t mapSize, lineStart = 0;
    const char *path = NULL;
    size_t pathLength = 0, i;
    uint32_t tpage = 0, clut = 0;
    uint32_t windowWidthU = 256, windowWidthV = 256;
    uint32_t windowOffsetU = 0, windowOffsetV = 0;
    int found = 0;
    if (pathOut == NULL || pathLengthOut == NULL || instance == NULL) return 0;
    cached = ModernAssetsFind(instance);
    if (cached == NULL || cached->location.materialPathLength == 1 ||
        cached->location.materialPath[0] == '-' ||
        !ModernAssetReadFile(NULL, cached->location.materialPath,
                             cached->location.materialPathLength, &mapBytes,
                             &mapSize)) return 0;
    for (i = 0; i <= mapSize; i++) {
        if (i == mapSize || ((const char *)mapBytes)[i] == '\n') {
            const char *line = (const char *)mapBytes + lineStart;
            size_t length = i - lineStart, number = 0, cursor = 0;
            uint32_t value = 0, sourceTpage = 0, sourceClut = 0;
            while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9') {
                value = value * 10u + (uint32_t)(line[cursor++] - '0'); number++;
            }
            if (number && cursor < length && line[cursor] == ' ' && value == material) {
                size_t start;
                cursor++;
                start = cursor;
                while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9') {
                    sourceTpage = sourceTpage * 10u + (uint32_t)(line[cursor++] - '0');
                }
                if (cursor > start && cursor < length && line[cursor] == ' ') {
                    size_t clutStart;
                    cursor++;
                    clutStart = cursor;
                    while (cursor < length && line[cursor] >= '0' && line[cursor] <= '9') {
                        sourceClut = sourceClut * 10u + (uint32_t)(line[cursor++] - '0');
                    }
                    if (cursor > clutStart && cursor < length && line[cursor] == ' ') {
                        size_t primaryStart, primaryLength;
                        size_t selectedStart = 0, selectedLength = 0;
                        uint32_t pathIndex = 0;
                        cursor++;
                        /* v3 sidecar appends the decoded PS1 texture window
                         * before the paths. Older sidecars begin with a path
                         * and therefore retain the full-page defaults. */
                        if (cursor < length && line[cursor] >= '0' &&
                            line[cursor] <= '9') {
                            uint32_t *windowValues[4] = {
                                &windowWidthU, &windowWidthV,
                                &windowOffsetU, &windowOffsetV};
                            uint32_t windowIndex;
                            for (windowIndex = 0; windowIndex < 4;
                                 windowIndex++) {
                                uint32_t parsed = 0;
                                size_t valueStart = cursor;
                                while (cursor < length &&
                                       line[cursor] >= '0' &&
                                       line[cursor] <= '9') {
                                    parsed = parsed * 10u +
                                        (uint32_t)(line[cursor++] - '0');
                                }
                                if (cursor == valueStart || cursor >= length ||
                                    line[cursor] != ' ') break;
                                *windowValues[windowIndex] = parsed;
                                cursor++;
                            }
                            if (windowIndex != 4) break;
                        }
                        primaryStart = cursor;
                        while (cursor < length && line[cursor] != ' ') cursor++;
                        primaryLength = cursor - primaryStart;
                        selectedStart = primaryStart;
                        selectedLength = primaryLength;
                        while (cursor < length) {
                            size_t candidateStart, candidateLength;
                            while (cursor < length && line[cursor] == ' ')
                                cursor++;
                            candidateStart = cursor;
                            while (cursor < length && line[cursor] != ' ')
                                cursor++;
                            candidateLength = cursor - candidateStart;
                            pathIndex++;
                            if (pathIndex == variant && candidateLength != 0) {
                                selectedStart = candidateStart;
                                selectedLength = candidateLength;
                            }
                        }
                        path = line + selectedStart;
                        pathLength = selectedLength;
                        tpage = sourceTpage; clut = sourceClut; found = 1;
                    }
                } else {
                    /* v1 sidecar: `material path`. Keep existing extracted
                     * assets usable while the richer contract rolls out. */
                    path = line + start; pathLength = length - start; found = 1;
                }
                break;
            }
            lineStart = i + 1;
        }
    }
    if (found && pathLength < sizeof(s_materialPath)) {
        memcpy(s_materialPath, path, pathLength);
        s_materialPath[pathLength] = '\0';
        *pathOut = s_materialPath; *pathLengthOut = pathLength;
        if (sourceOut != NULL) {
            sourceOut->tpage = tpage;
            sourceOut->clut = clut;
            sourceOut->windowWidthU = windowWidthU;
            sourceOut->windowWidthV = windowWidthV;
            sourceOut->windowOffsetU = windowOffsetU;
            sourceOut->windowOffsetV = windowOffsetV;
        }
        if (getenv("RAGE_PORT_MODERN_ASSET_TRACE") != NULL) {
            fprintf(stderr,
                    "rage-port: native material asset=%u set=%u material=%u "
                    "variant=%u path=%s\n",
                    instance->assetKey, (unsigned)instance->assetSet, material,
                    variant, s_materialPath);
        }
    } else {
        found = 0;
    }
    ModernAssetFreeFile(NULL, mapBytes);
    return found;
}

int ModernAssetsLoadMaterialPixels(const RageRenderMeshInstance *instance,
                                   uint32_t material, uint8_t variant,
                                   const void **bytes,
                                   size_t *size,
                                   ModernAssetMaterialSource *source) {
    const char *path;
    size_t pathLength;
    if (bytes == NULL || size == NULL) return 0;
    *bytes = NULL; *size = 0;
    if (!ModernAssetsFindMaterial(instance, material, variant,
                                  &path, &pathLength,
                                  source) ||
        !ModernAssetReadFile(NULL, path, pathLength, bytes, size) ||
        *size != 256u * 256u * 4u) {
        if (*bytes != NULL) ModernAssetFreeFile(NULL, *bytes);
        *bytes = NULL; *size = 0; return 0;
    }
    return 1;
}

void ModernAssetsFreeMaterialPixels(const void *bytes) {
    if (bytes != NULL) ModernAssetFreeFile(NULL, bytes);
}

void ModernAssetsWarmWorld(const RageRenderWorld *world) {
    uint32_t i;
    if (!s_ready || world == NULL) return;
    for (i = 0; i < world->instanceCount; i++) {
        (void)ModernAssetsFind(&world->instances[i]);
    }
}
