#include "native_mesh_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)

typedef struct MeshSource { int passes; int failPass; SVec vertices[4]; } MeshSource;
static int VisitFixture(void *context, RageImportedFaceVisitor visitor,
                        void *output, uint32_t *meshCount) {
    MeshSource *source = context;
    if (++source->passes == source->failPass) return 0;
    RageImportedFace face = {0};
    face.vertices = source->vertices;
    face.color[0] = 255;
    for (unsigned i = 0; i < 4; ++i) face.vertex[i] = (uint16_t)i;
    *meshCount = 1;
    return visitor(0, &face, output);
}

static void Put16(uint8_t *bytes, uint16_t value) {
    bytes[0] = (uint8_t)value; bytes[1] = (uint8_t)(value >> 8);
}
static void TestModelPrimitives(void) {
    for (unsigned prim = 0; prim < 4; ++prim) {
        uint8_t stream[40] = {0};
        SVec vertices[4] = {{.vx = 100, .vy = -20, .vz = 30},
            {.vx = 200}, {.vx = 300}, {.vx = 400}};
        SVec normals[4] = {{.vy = -7}, {.vy = -8}, {.vy = -9}, {.vy = -10}};
        Put16(stream, (uint16_t)prim); Put16(stream + 2, 1);
        for (unsigned i = 0; i < 4; ++i) {
            Put16(stream + 4 + i * 2, (uint16_t)i);
            if (prim >= 2) Put16(stream + 12 + i * 2, (uint16_t)i);
        }
        uint8_t *face = stream + 4;
        if (prim & 1) {
            unsigned uv = prim == 1 ? 8 : 16;
            face[uv] = 64; face[uv + 1] = 128;
            Put16(face + uv + 2, 123); Put16(face + uv + 6, 456);
        } else {
            unsigned color = prim == 0 ? 8 : 16;
            face[color] = 11; face[color + 1] = 22; face[color + 2] = 33;
        }
        NativeModelBank bank = {.modelCount = 1, .table = vertices,
            .normals = normals, .models = {stream}};
        RenderMeshInstance identity = {.assetKey = 12,
            .assetSet = RAGE_RENDER_ASSET_MODEL_BANK};
        RageImportedMeshEntry entry = {0};
        int built = ImportBuildBankMesh(&identity, &bank, &entry);
        CHECK(built);
        if (!built) continue;
        RageRuntimeVertex vertex;
        CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
        CHECK(vertex.position[0] == 100 && vertex.position[1] == 20 && vertex.position[2] == -30);
        CHECK(vertex.normal[1] == (prim >= 2 ? 7 : 1));
        CHECK(entry.materialCount == (prim & 1));
        if (prim & 1) {
            CHECK(entry.materials[0].clut == 123 && entry.materials[0].tpage == 456);
            CHECK(vertex.uv[0] == 64.5f / 256 && vertex.uv[1] == 128.5f / 256);
        } else CHECK(vertex.color[0] == 11 && vertex.color[1] == 22 && vertex.color[2] == 33);
RageImportedMeshEntry saved = entry;
CHECK(!ImportBuildBankMesh(&identity, &bank, &entry));
CHECK(memcmp(&entry, &saved, sizeof(entry)) == 0);
RuntimeCachedMeshRelease(&entry.cached); free(entry.materials);
memset(&entry, 0, sizeof(entry));
bank.models[0] = NULL;
CHECK(!ImportBuildBankMesh(&identity, &bank, &entry));
CHECK(entry.cached.mesh.bytes == NULL && entry.materials == NULL);
bank.modelCount = GAME_MODEL_PER_BANK_LIMIT + 1;
CHECK(!ImportBuildBankMesh(&identity, &bank, &entry));

    }
}

static void TestCoursePrimitives(void) {
    for (unsigned prim = 0; prim < 4; ++prim) {
        uint8_t stream[40] = {0};
        SVec vertices[4] = {{.vx = 101, .vy = -22, .vz = 33},
            {.vx = 201}, {.vx = 301}, {.vx = 401}};
        Put16(stream, (uint16_t)prim); Put16(stream + 2, 1);
        uint8_t *face = stream + 4;
        for (unsigned i = 0; i < 4; ++i) Put16(face + i * 2, (uint16_t)i);
        face[8] = 11; face[9] = 22; face[10] = 33;
        if (prim != 0) {
            face[12] = 64; face[13] = 128;
            Put16(face + 14, 123); Put16(face + 18, 456);
            face[25] = 7;
            if (prim >= 2) {
                const uint32_t window = UINT32_C(0xe2000001);
                memcpy(face + 28, &window, sizeof(window));
            }
        }
        CourseBank bank = {.modelCount = 1,
            .models = {{vertices, 4, stream}}};
        RenderMeshInstance identity = {.assetKey = 88,
            .assetSet = RAGE_RENDER_ASSET_COURSE};
        RageImportedMeshEntry entry = {0};
        CHECK(!ImportBuildCourseMesh(&identity, NULL, &entry));
        CHECK(entry.cached.mesh.bytes == NULL && entry.materials == NULL);
        int built = ImportBuildCourseMesh(&identity, &bank, &entry);
        CHECK(built);
        if (!built) continue;
        RageRuntimeVertex vertex;
        CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
        CHECK(vertex.position[0] == 101 && vertex.position[1] == 22 && vertex.position[2] == -33);
        CHECK(vertex.color[0] == 11 && vertex.color[1] == 22 && vertex.color[2] == 33);
        CHECK(entry.cached.mesh.meshCount == 1 && entry.cached.mesh.indexCount == 6);
        CHECK(entry.materialCount == (prim != 0));
        if (prim != 0) {
            CHECK(entry.materials[0].clut == 123 && entry.materials[0].tpage == 456);
            CHECK(entry.materials[0].emissive == (prim == 3));
            CHECK(entry.materials[0].hasWindow == (prim >= 2));
            if (prim >= 2) CHECK(entry.materials[0].windowWidthU == 248);
        }
        const void *bytes = entry.cached.mesh.bytes;
        CHECK(!ImportBuildCourseMesh(&identity, &bank, &entry));
        CHECK(entry.cached.mesh.bytes == bytes);
        memset(stream, 0xff, sizeof(stream));
        CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
        CHECK(vertex.position[0] == 101);
        RuntimeCachedMeshRelease(&entry.cached);
        free(entry.materials);
    }
}

int main(void) {
    TestCoursePrimitives();
    TestModelPrimitives();
CHECK(!ImportWriteFinish(NULL, 0));
for (int failPass = 0; failPass <= 2; ++failPass) {
    MeshSource source = {.failPass = failPass,
        .vertices = {{.vx = 12}, {.vx = 24}, {.vx = 36}, {.vx = 48}}};
    RenderMeshInstance identity = {.assetKey = 10,
        .assetSet = RAGE_RENDER_ASSET_MODEL_BANK};
    RageImportedMeshEntry entry = {0}, before = entry;
    int result = ImportBuildMeshEntry(&identity, VisitFixture, &source, &entry);
    CHECK(result == (failPass == 0));
    if (result) {
        RageRuntimeVertex vertex;
        CHECK(entry.cached.assetKey == 10);
        CHECK(entry.cached.assetSet == RAGE_RENDER_ASSET_MODEL_BANK);
        CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
        CHECK(vertex.position[0] == 12);
        memset(&source, 0, sizeof(source));
        CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
        CHECK(vertex.position[0] == 12);
        RuntimeCachedMeshRelease(&entry.cached);
        free(entry.materials);
    } else {
        CHECK(memcmp(&entry, &before, sizeof(entry)) == 0);
    }
}

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
        const uint32_t expectedMaterial[] = {UINT32_MAX, UINT32_C(0x80000000), UINT32_C(0x74FC0000)};
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
