#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh_cache.h"

static int failures;
static int reads, frees;
#define EXPECT(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__, #value); \
} } while (0)

static void write_u32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static int read_file(void *context, const char *path, size_t pathLength,
                     const void **bytes, size_t *size) {
    static uint8_t mesh[96];
    (void)context;
    if (pathLength != strlen("models/a.rmesh") ||
        memcmp(path, "models/a.rmesh", pathLength) != 0) return 0;
    memset(mesh, 0, sizeof(mesh));
    memcpy(mesh, "RRMESH1", 7);
    write_u32(mesh + 8, 1); write_u32(mesh + 12, 1);
    write_u32(mesh + 16, 1); write_u32(mesh + 20, 6);
    write_u32(mesh + 24, 0); write_u32(mesh + 28, 6);
    *bytes = mesh; *size = sizeof(mesh); reads++;
    return 1;
}

static void free_file(void *context, const void *bytes) {
    (void)context; (void)bytes; frees++;
}

int main(void) {
    static const char index[] = "10 model models/a.rmesh models/a.rmat\n";
    RageRuntimeCachedMesh entries[1];
    RageRuntimeMeshCache cache;
    const RageRuntimeCachedMesh *first;

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                             free_file, 0, entries, 1);
    first = RuntimeMeshCacheFind(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK);
    EXPECT(first != 0 && first->mesh.meshCount == 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                    RAGE_RENDER_ASSET_MODEL_BANK) == first);
    EXPECT(reads == 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 11,
                                    RAGE_RENDER_ASSET_MODEL_BANK) == 0);
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 1 && cache.count == 0);

    RuntimeMeshCacheInit(NULL, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, NULL, 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(reads == 1);

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    cache.count = 2;
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(reads == 1);
    entries[0].ownedBytes = entries;
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 2 && cache.count == 0);

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, NULL, 1);
    cache.count = 2;
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 2 && cache.count == 0);
    RuntimeMeshCacheRelease(NULL);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
