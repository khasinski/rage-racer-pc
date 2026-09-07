#include "render/render_geometry_pack.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    RageNativeGeometryPack pack = {0};
    RageNativeGpuVertex source[256] = {0};
    for (unsigned i = 0; i < 256; ++i) source[i].position[0] = (float)(i % 3);
    CHECK(!RenderGeometryPackBuild(NULL, source, 3));
    CHECK(RenderGeometryPackBuild(&pack, source, 3));
    CHECK(pack.vertexCount == 3 && pack.capacity < 256);
    CHECK(RenderGeometryPackBuild(&pack, source, 256));
    CHECK(pack.vertexCount == 3 && pack.indexCount == 256);
    for (unsigned i = 0; i < 256; ++i) {
        CHECK(pack.indices[i] == i % 3);
        CHECK(memcmp(&source[i], &pack.vertices[pack.indices[i]], sizeof(source[i])) == 0);
    }
    /* Any changed attribute must retain a distinct GPU vertex. */
    for (size_t byte = 0; byte < sizeof(source[0]); ++byte) {
        memset(source, 0, sizeof(source));
        ((unsigned char *)&source[1])[byte] = 1;
        CHECK(RenderGeometryPackBuild(&pack, source, 3));
        CHECK(pack.vertexCount == 2 && pack.indices[0] == pack.indices[2]);
        CHECK(pack.indices[1] != pack.indices[0]);
    }
    for (unsigned i = 0; i < 256; ++i) source[i].position[0] = (float)i;
    CHECK(RenderGeometryPackBuild(&pack, source, 256));
    CHECK(pack.vertexCount == 256);
    for (unsigned i = 0; i < 256; ++i)
        CHECK(memcmp(&source[i], &pack.vertices[pack.indices[i]], sizeof(source[i])) == 0);
    CHECK(!RenderGeometryPackBuild(&pack, pack.vertices, 1));
    CHECK(pack.indexCount == 0 && pack.vertexCount == 0);
    CHECK(!RenderGeometryPackBuild(&pack, NULL, 1));
    CHECK(pack.indexCount == 0 && pack.vertexCount == 0);
    CHECK(!RenderGeometryPackBuild(&pack, source, UINT32_MAX));
    CHECK(RenderGeometryPackBuild(&pack, source, 256));
    CHECK(RenderGeometryPackBuild(&pack, NULL, 0));
    CHECK(pack.indexCount == 0 && pack.vertexCount == 0);
    RenderGeometryPackRelease(&pack); RenderGeometryPackRelease(&pack);
    CHECK(pack.vertices == NULL && pack.indices == NULL && pack.slots == NULL);
    CHECK(RenderGeometryPackBuild(&pack, source, 256));
    RenderGeometryPackRelease(&pack);
    memset(source, 0, sizeof(source));
    for (unsigned i = 0; i < 128; ++i) source[i].position[0] = (float)i;
    CHECK(RenderGeometryPackAppend(&pack, source, 32, 128));
    uint64_t generation = pack.generation;
    CHECK(RenderGeometryPackAppend(&pack, source, 32, 128));
    CHECK(pack.vertexCount == 32 && pack.generation == generation);
    CHECK(RenderGeometryPackAppend(&pack, source + 32, 32, 128));
    CHECK(pack.vertexCount == 64 && pack.generation == generation);
    CHECK(RenderGeometryPackAppend(&pack, source, 32, 128));
    CHECK(pack.capacity == 128 && pack.vertexCount == 64);
    CHECK(pack.generation == generation);
    for (unsigned i = 0; i < 32; ++i) CHECK(pack.indices[i] == i);
    CHECK(!RenderGeometryPackAppend(&pack, source, 129, 128));
    CHECK(pack.indexCount == 0 && pack.vertexCount == 64);
    CHECK(RenderGeometryPackAppend(&pack, source, 128, 128));
    CHECK(pack.generation != generation && pack.vertexCount == 128);
    for (unsigned frame = 0; frame < 20; ++frame) {
        for (unsigned i = 0; i < 64; ++i) source[i].position[0] = (float)(frame * 64 + i);
        CHECK(RenderGeometryPackAppend(&pack, source, 64, 128));
        CHECK(pack.vertexCount <= 128 && pack.indexCount == 64);
        for (unsigned i = 0; i < 64; ++i)
            CHECK(memcmp(&source[i], &pack.vertices[pack.indices[i]], sizeof(source[i])) == 0);
    }
    RenderGeometryPackRelease(&pack);
    uint8_t retain[64];
    memset(source, 0, sizeof(source));
    for (unsigned i = 0; i < 64; ++i) {
        retain[i] = (uint8_t)(i % 2 == 0);
        source[i].position[0] = (float)i;
    }
    CHECK(RenderGeometryPackAppendSelected(&pack, source, 64, retain, 128));
    generation = pack.generation;
    for (unsigned frame = 0; frame < 20; ++frame) {
        for (unsigned i = 1; i < 64; i += 2) source[i].position[0] = (float)(frame * 100 + i);
        CHECK(RenderGeometryPackAppendSelected(&pack, source, 64, retain, 128));
        CHECK(pack.residentCount == 32 && pack.vertexCount == 64);
        CHECK(pack.generation == generation);
        for (unsigned i = 0; i < 64; ++i) {
            CHECK(memcmp(&source[i], &pack.vertices[pack.indices[i]], sizeof(source[i])) == 0);
            if (retain[i]) CHECK(pack.indices[i] == i / 2);
        }
    }
    RenderGeometryPackRelease(&pack);
    RageNativeGeometryPack flat = {0};
    uint8_t mixed[64];
    for (unsigned i = 0; i < 64; ++i)
        mixed[i] = i < 16 ? 1 : i < 48 ? (uint8_t)(i % 2) : 0;
    for (unsigned frame = 0; frame < 20; ++frame) {
        for (unsigned i = 0; i < 64; ++i) {
            source[i].position[0] = (float)(mixed[i] ? i : frame * 100 + i);
            source[i].uv[0] = (float)i / 64;
        }
        const RageNativeGeometryRange ranges[] = {
            {source, 16, NULL, 1}, {NULL, 0, NULL, 0},
            {source + 16, 32, mixed + 16, 0}, {source + 48, 16, NULL, 0}};
        CHECK(RenderGeometryPackAppendRanges(&pack, ranges, 4, 128));
        CHECK(RenderGeometryPackAppendSelected(&flat, source, 64, mixed, 128));
        CHECK(pack.vertexCount == flat.vertexCount && pack.residentCount == flat.residentCount);
        CHECK(pack.generation == flat.generation && pack.indexCount == 64);
        CHECK(memcmp(pack.indices, flat.indices, 64 * sizeof(*pack.indices)) == 0);
        CHECK(memcmp(pack.vertices, flat.vertices, pack.vertexCount * sizeof(*pack.vertices)) == 0);
        for (unsigned i = 0; i < 64; ++i)
            CHECK(memcmp(&source[i], &pack.vertices[pack.indices[i]], sizeof(source[i])) == 0);
    }
    uint32_t resident = pack.residentCount;
    generation = pack.generation;
    RageNativeGeometryRange invalid[] = {{source, 16, NULL, 1}, {NULL, 1, NULL, 1}};
    CHECK(!RenderGeometryPackAppendRanges(&pack, invalid, 2, 128));
    CHECK(pack.indexCount == 0 && pack.residentCount == resident && pack.generation == generation);
    invalid[0].count = UINT32_MAX;
    invalid[1].vertices = source;
    CHECK(!RenderGeometryPackAppendRanges(&pack, invalid, 2, UINT32_MAX));
    CHECK(!RenderGeometryPackAppendRanges(&pack, NULL, 1, 128));
    CHECK(RenderGeometryPackAppendRanges(&pack, NULL, 0, 128));
    RenderGeometryPackRelease(&pack);
    RenderGeometryPackRelease(&flat);
    puts("Geometry pack: reconstruction, bounded residency, ranges and transient replacement passed");
    return 0;
}
