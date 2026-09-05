#include "native_mesh_writer.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)

int main(void) {
    CHECK(!ImportWriteFinish(NULL, 0));
    {
        uint8_t offsets[20] = {0};
        RageImportedWrite write = {.offsets = offsets, .meshLimit = 4,
            .vertexLimit = 4, .vertexCount = 4, .indexLimit = 6, .indexCount = 6};
        CHECK(ImportWriteFinish(&write, 4));
        CHECK(write.currentMesh == 4);
        for (unsigned i = 0; i < sizeof(offsets); ++i)
            CHECK(offsets[i] == (i > 0 && i % 4 == 0 ? 6 : 0));
    }
    for (unsigned test = 0; test < 5; ++test) {
        uint8_t bytes[20], saved[20];
        memset(bytes, 0xA5, sizeof(bytes));
        memcpy(saved, bytes, sizeof(bytes));
        RageImportedWrite write = {.offsets = bytes, .meshLimit = 4,
            .vertexLimit = 4, .vertexCount = 4, .indexLimit = 6, .indexCount = 6};
        uint32_t meshes = 4;
        if (test == 0) meshes = 5;
        if (test == 1) meshes = 3;
        if (test == 2) write.vertexCount = 0;
        if (test == 3) write.indexCount = 0;
        if (test == 4) write.currentMesh = 5;
        uint32_t before = write.currentMesh;
        CHECK(!ImportWriteFinish(&write, meshes));
        CHECK(write.currentMesh == before);
        CHECK(memcmp(bytes, saved, sizeof(bytes)) == 0);
    }
    SVec positions[4] = {0};
    RageImportedMeshEntry entry = {0};
    RageImportedTextureKey material = {0};
    entry.materials = &material;
    entry.materialCount = 1;
    for (unsigned test = 0; test < 8; ++test) {
        unsigned char bytes[256], saved[256];
        memset(bytes, 0xA5, sizeof(bytes));
        RageImportedFace face = {0};
        face.vertices = positions;
        for (unsigned i = 0; i < 4; ++i) face.vertex[i] = (uint16_t)i;
        RageImportedWrite write = {0};
        write.entry = &entry;
        write.offsets = bytes + 8;
        write.vertexCursor = bytes + 32;
        write.indexCursor = bytes + 192;
        write.meshLimit = 1;
        write.vertexLimit = 4;
        write.indexLimit = 6;
        uint32_t mesh = 0;
        if (test == 0) mesh = 1; /* More meshes than sizing pass. */
        if (test == 1) write.currentMesh = 1; /* Reversed traversal. */
        if (test == 2) write.vertexLimit = 3;
        if (test == 3) write.indexLimit = 5;
        if (test == 4) write.vertexCount = UINT32_MAX;
        if (test == 5) write.indexCount = UINT32_MAX;
        if (test == 6) { face.textured = 1; face.texture.tpage = 1; }
        if (test == 7) {
            /* First face fits exactly; second face must not write anything. */
            CHECK(ImportWriteFace(0, &face, &write));
            CHECK(write.vertexCount == 4 && write.indexCount == 6);
            for (unsigned i = 0; i < 8; ++i) CHECK(bytes[i] == 0xA5);
            for (unsigned i = 216; i < sizeof(bytes); ++i) CHECK(bytes[i] == 0xA5);
        }
        memcpy(saved, bytes, sizeof(bytes));
        RageImportedWrite before = write;
        CHECK(!ImportWriteFace(mesh, &face, &write));
        CHECK(memcmp(bytes, saved, sizeof(bytes)) == 0);
        CHECK(write.currentMesh == before.currentMesh &&
              write.vertexCount == before.vertexCount && write.indexCount == before.indexCount);
        CHECK(write.vertexCursor == before.vertexCursor && write.indexCursor == before.indexCursor);
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        uint8_t bytes[216] = {0};
        RageImportedFace face = {0};
        SVec normal = {.vx = 10, .vy = 20, .vz = -30};
        positions[0] = (SVec){.vx = 11, .vy = -22, .vz = 33};
        face.vertices = positions;
        face.normals = &normal;
        face.hasNormals = mode != 0;
        face.textured = mode != 0;
        face.prim = mode == 1 ? 3 : 0;
        face.flags = mode == 2 ? 2 : 0;
        face.depthBias = mode == 2 ? -4 : 0;
        face.color[0] = 17; face.color[1] = 34; face.color[2] = 51;
        face.uv[0][0] = 127; face.uv[0][1] = 255;
        entry.cached.assetSet = mode == 2 ? RAGE_RENDER_ASSET_TERRAIN : RAGE_RENDER_ASSET_COURSE;
        RageImportedWrite write = {.entry = &entry, .offsets = bytes + 24,
            .vertexCursor = bytes + 32, .indexCursor = bytes + 192,
            .meshLimit = 1, .vertexLimit = 4, .indexLimit = 6};
        CHECK(RuntimeMeshEncodeHeader(bytes, sizeof(bytes), 1, 4, 6));
        CHECK(ImportWriteFace(0, &face, &write));
        CHECK(ImportWriteFinish(&write, 1));
        CHECK(ImportWriteFinish(&write, 1)); /* Repeated finalization is harmless. */
        RageRuntimeMesh mesh;
        RageRuntimeVertex vertex;
        CHECK(RuntimeMeshOpen(&mesh, bytes, sizeof(bytes)));
        CHECK(RuntimeMeshVertex(&mesh, 0, &vertex));
        CHECK(vertex.position[0] == 11 && vertex.position[1] == 22 && vertex.position[2] == -33);
        CHECK(vertex.normal[0] == (mode ? 10 : 0) &&
              vertex.normal[1] == (mode ? -20 : 1) && vertex.normal[2] == (mode ? 30 : 0));
        CHECK(vertex.color[0] == 17 && vertex.color[1] == 34 && vertex.color[2] == 51 && vertex.color[3] == 255);
        CHECK(vertex.uv[0] == (mode ? 127.5f / 256 : 0) && vertex.uv[1] == (mode ? 255.5f / 256 : 0));
        const uint32_t expectedMaterial[] = {UINT32_MAX, UINT32_C(0x80000000), UINT32_C(0x70FC0000)};
        CHECK(vertex.material == expectedMaterial[mode]);
        const uint32_t expectedIndices[] = {0, 2, 1, 1, 2, 3};
        for (unsigned i = 0; i < 6; ++i) {
            uint32_t index;
            CHECK(RuntimeMeshIndex(&mesh, i, &index));
            CHECK(index == expectedIndices[i]);
        }
    }
    return failures ? 1 : 0;
}
