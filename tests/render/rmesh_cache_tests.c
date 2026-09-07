#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/rmesh_cache.h"

static int failures;
static int reads, frees;
static int readCalls;
static int failNextRead;
static int corruptNextRead;
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
    if (corruptNextRead) { mesh[0] = 0; corruptNextRead = 0; }
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

static void test_resolve_status(void) {
    static const char index[] =
        "# rage-rmesh-index v2\n"
        "10 model models/a.rmesh models/a.rmat\n"
        "11 model models/a.rmesh models/a.rmat\n";
    RageRuntimeCachedMesh entries[1];
    RageRuntimeMeshCache cache;
    const RageRuntimeCachedMesh *result = entries;
    int calls = readCalls, released = frees;
    EXPECT(RuntimeIndexValidate(index, sizeof(index) - 1, NULL));
    RuntimeMeshCacheInit(&cache, index, sizeof(index) - 1, read_file,
                         free_file, NULL, entries, 1);
    EXPECT(RuntimeMeshCacheResolve(NULL, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_ERROR);
    EXPECT(result == NULL);
    EXPECT(RuntimeMeshCacheResolve(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   NULL) == RAGE_RUNTIME_MESH_ERROR);
    result = entries;
    EXPECT(RuntimeMeshCacheResolve(&cache, 99, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_MISSING);
    EXPECT(result == NULL && readCalls == calls);
    failNextRead = 1;
    result = entries;
    EXPECT(RuntimeMeshCacheResolve(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_ERROR);
    EXPECT(result == NULL && cache.count == 0 && frees == released);
    corruptNextRead = 1;
    result = entries;
    EXPECT(RuntimeMeshCacheResolve(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_ERROR);
    EXPECT(result == NULL && cache.count == 0 && frees == released + 1);
    EXPECT(RuntimeMeshCacheResolve(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_READY);
    EXPECT(result == entries && cache.count == 1);
    calls = readCalls;
    EXPECT(RuntimeMeshCacheResolve(&cache, 10, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_READY);
    EXPECT(result == entries && readCalls == calls);
    EXPECT(RuntimeMeshCacheResolve(&cache, 11, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_ERROR);
    EXPECT(result == NULL && cache.count == 1 && readCalls == calls);
    result = entries;
    EXPECT(RuntimeMeshCacheResolve(&cache, 99, RAGE_RENDER_ASSET_MODEL_BANK,
                                   &result) == RAGE_RUNTIME_MESH_MISSING);
    EXPECT(result == NULL && readCalls == calls);
    RuntimeMeshCacheRelease(&cache);
    EXPECT(frees == released + 2);
}

static void test_decoded_geometry_lifetime(void) {
    uint8_t bytes[164] = {0};
    RageRuntimeCachedMesh owner = {0};
    RageRuntimeMesh wire;
    const uint32_t order[3] = {2, 0, 1};
    EXPECT(RuntimeMeshEncodeHeader(bytes, sizeof(bytes), 1, 3, 3));
    write_u32(bytes + 28, 3);
    for (unsigned i = 0; i < 3; ++i) {
        RageRuntimeVertex vertex = {0};
        vertex.position[0] = (float)i - 1.25f;
        vertex.position[2] = -10.5f;
        vertex.normal[1] = 1;
        vertex.uv[0] = (float)i * 0.25f;
        vertex.uv[1] = -0.5f;
        vertex.color[0] = (uint8_t)(i * 97);
        vertex.color[3] = 255;
        vertex.material = i == 2 ? UINT32_MAX : RAGE_RUNTIME_MATERIAL_SCROLL_U | i;
        EXPECT(RuntimeVertexEncode(bytes + 32 + i * 40, 40, &vertex));
        write_u32(bytes + 152 + i * 4, order[i]);
    }
    EXPECT(RuntimeMeshOpen(&wire, bytes, sizeof(bytes)));
    EXPECT(wire.vertices == NULL && wire.indices == NULL);
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        EXPECT(RuntimeCachedMeshAdopt(&owner, bytes, sizeof(bytes), NULL, NULL));
        EXPECT(owner.mesh.vertices == owner.ownedVertices && owner.ownedVertices != NULL);
        EXPECT(owner.mesh.indices == owner.ownedIndices && owner.ownedIndices != NULL);
        for (unsigned i = 0; i < 3; ++i) {
            RageRuntimeVertex reference, decoded;
            uint32_t index = UINT32_MAX;
            EXPECT(RuntimeMeshVertex(&wire, i, &reference));
            EXPECT(RuntimeMeshVertex(&owner.mesh, i, &decoded));
            EXPECT(memcmp(&reference, &decoded, sizeof(reference)) == 0);
            EXPECT(RuntimeMeshIndex(&owner.mesh, i, &index) && index == order[i]);
        }
        RageRuntimeVertex invalid;
        uint32_t invalidIndex = UINT32_MAX;
        EXPECT(!RuntimeMeshVertex(&owner.mesh, 3, &invalid));
        EXPECT(!RuntimeMeshIndex(&owner.mesh, 3, &invalidIndex) && invalidIndex == 0);
        RageRuntimeMesh reopened = owner.mesh;
        EXPECT(RuntimeMeshOpen(&reopened, bytes, sizeof(bytes)));
        EXPECT(reopened.vertices == NULL && reopened.indices == NULL);
        EXPECT(!RuntimeMeshOpen(&reopened, bytes, 1));
        EXPECT(reopened.vertices == NULL && reopened.indices == NULL);
        RuntimeCachedMeshRelease(&owner);
        EXPECT(owner.ownedVertices == NULL && owner.ownedIndices == NULL);
        EXPECT(owner.mesh.vertices == NULL && owner.mesh.indices == NULL);
        RuntimeCachedMeshRelease(&owner);
    }
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
    test_resolve_status();
    test_decoded_geometry_lifetime();
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
