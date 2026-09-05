#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh_cache.h"

static int failures;
static int reads, frees;
static int readCalls;
static int failNextRead;
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
    ++readCalls;
    if (failNextRead) {
        failNextRead = 0;
        return 0;
    }
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

typedef struct OwnerProbe { const void *expected; int releases; } OwnerProbe;
static void release_owned(void *context, const void *bytes) {
    OwnerProbe *probe = context;
    EXPECT(probe->expected == bytes);
    ++probe->releases;
    free((void *)bytes);
}

static void test_ownership(void) {
    RageRuntimeCachedMesh owner = {0};
    const void *bytes;
    size_t size;
    uint8_t invalid[96] = {0};
    OwnerProbe probe = {0};
    EXPECT(read_file(NULL, "models/a.rmesh", strlen("models/a.rmesh"), &bytes, &size));
    const void *borrowedBytes = bytes;
    void *allocated = malloc(size);
    EXPECT(allocated != NULL);
    if (allocated == NULL) return;
    memcpy(allocated, bytes, size);
    bytes = allocated;
    probe.expected = bytes;
    owner.assetKey = 123;
    EXPECT(!RuntimeCachedMeshAdopt(&owner, invalid, sizeof(invalid), release_owned, &probe));
    EXPECT(owner.assetKey == 123 && owner.ownedBytes == NULL && probe.releases == 0);
    EXPECT(RuntimeCachedMeshAdopt(&owner, bytes, size, release_owned, &probe));
    EXPECT(owner.mesh.bytes == bytes && owner.mesh.bounds == owner.ownedBounds);
    const RageRuntimeMeshBounds *borrowedBounds = owner.mesh.bounds;
    EXPECT(!RuntimeCachedMeshAdopt(&owner, bytes, size, release_owned, &probe));
    EXPECT(owner.mesh.bounds == borrowedBounds && probe.releases == 0);
    RuntimeCachedMeshRelease(&owner);
    EXPECT(probe.releases == 1 && owner.mesh.bytes == NULL && owner.ownedBounds == NULL);
    RuntimeCachedMeshRelease(&owner);
    EXPECT(probe.releases == 1);
    EXPECT(RuntimeCachedMeshAdopt(&owner, borrowedBytes, size, NULL, NULL));
    RuntimeCachedMeshRelease(&owner);
    EXPECT(probe.releases == 1);
    EXPECT(!RuntimeCachedMeshAdopt(NULL, borrowedBytes, size, release_owned, &probe));
    RuntimeCachedMeshRelease(NULL);
}

static void test_failed_prepare(void) {
    static const char index[] = "10 model models/a.rmesh models/a.rmat\n";
    RageRuntimeCachedMesh entries[1];
    RageRuntimeMeshCache cache;
    int before = readCalls;
    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    failNextRead = 1;
    EXPECT(RuntimeMeshCacheFind(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(readCalls == before + 1 && cache.count == 0);
    /* Main, mirror and completeness checks must not retry a failed prepare.
     * The provider would now succeed, but no geometry was built this frame. */
    for (int consumer = 0; consumer < 3; ++consumer)
        EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(readCalls == before + 1 && cache.count == 0);
    /* An explicit subsequent preparation may recover. */
    const RageRuntimeCachedMesh *mesh = RuntimeMeshCacheFind(
        &cache, 10, RAGE_RENDER_ASSET_MODEL_BANK);
    EXPECT(mesh != NULL && readCalls == before + 2 && cache.count == 1);
    for (int consumer = 0; consumer < 3; ++consumer)
        EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == mesh);
    EXPECT(readCalls == before + 2);
    RuntimeMeshCacheRelease(&cache);
}

int main(void) {
    static const char index[] = "10 model models/a.rmesh models/a.rmat\n";
    RageRuntimeCachedMesh entries[1];
    RageRuntimeMeshCache cache;
    const RageRuntimeCachedMesh *first;

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                             free_file, 0, entries, 1);
    EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(readCalls == 0 && cache.count == 0);
    first = RuntimeMeshCacheFind(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK);
    EXPECT(first != 0 && first->mesh.meshCount == 1);
    EXPECT(first != NULL && first->ownedBounds != NULL &&
           first->mesh.bounds == first->ownedBounds && first->ownedBounds[0].valid);
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                    RAGE_RENDER_ASSET_MODEL_BANK) == first);
    EXPECT(reads == 1);
    int calls = readCalls;
    EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == first);
    EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_TERRAIN) == NULL);
    EXPECT(RuntimeMeshCachePeek(&cache, 999, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(RuntimeMeshCachePeek(NULL, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(readCalls == calls && cache.count == 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 11,
                                    RAGE_RENDER_ASSET_MODEL_BANK) == 0);
    cache.freeFile = NULL; /* Existing entries retain their original owner. */
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 1 && cache.count == 0);
    EXPECT(RuntimeMeshCachePeek(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(entries[0].ownedBounds == NULL && entries[0].mesh.bounds == NULL);
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 1);

    RuntimeMeshCacheInit(NULL, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, NULL, 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(reads == 1);

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    EXPECT(RuntimeMeshCacheFind(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) != NULL);
    cache.count = 2;
    EXPECT(RuntimeMeshCacheFind(&cache, 10,
                                RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
    EXPECT(reads == 2);
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 2 && cache.count == 0);

    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, NULL, 1);
    cache.count = 2;
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == 2 && cache.count == 0);
    RuntimeMeshCacheRelease(NULL);
    test_ownership();
    test_failed_prepare();
    {
        static const char unsafeIndex[] = "10 model ../a.rmesh models/a.rmat\n";
        int before = readCalls;
        RuntimeMeshCacheInit(&cache, unsafeIndex, sizeof(unsafeIndex) - 1,
                             read_file, free_file, NULL, entries, 1);
        EXPECT(RuntimeMeshCacheFind(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK) == NULL);
        EXPECT(readCalls == before && cache.count == 0);
        RuntimeMeshCacheRelease(&cache);
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
