/* Exercise the production stream parser and material-key resolution together. */
#include <stdio.h>
#include <stdlib.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#include "../../src/port/native_import_stream.h"

static RageImportedTextureKey s_keys[6];

static int SaveMaterial(uint32_t mesh, const RageImportedFace *face, void *ctx) {
    (void)mesh;
    (void)ctx;
    s_keys[face->prim] = face->texture;
    return 1;
}

static void Put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

int main(void) {
    uint8_t stream[6 * 40 + 4] = {0};
    uint8_t *cursor = stream;
    SVec vertices[1] = {{.vx = 11, .vy = -22, .vz = 33}};
    unsigned mode, variant;
    /* Retail modes 0/1 follow environment lighting; 2..5 select a fixed
     * palette, including their encoded odd-mode offset. Page selection in
     * variants 2/3 must not change any palette. */
    static const uint16_t expected[6][4] = {
        {0x7800, 0x7801, 0x7800, 0x7801},
        {0x7800, 0x7801, 0x7800, 0x7801},
        {0x7800, 0x7800, 0x7800, 0x7800},
        {0x7801, 0x7801, 0x7801, 0x7801},
        {0x7800, 0x7800, 0x7800, 0x7800},
        {0x7801, 0x7801, 0x7801, 0x7801},
    };
    for (mode = 0; mode < 6; mode++) {
        int stride = TerrainPrimitiveStride(mode);
        Put16(cursor, (uint16_t)mode);
        Put16(cursor + 2, 1);
        cursor += 4;
        Put16(cursor + 10, 0x7800);
        Put16(cursor + 14, 0x19);
        if (stride == 0x24) {
            cursor[32] = 0xff; cursor[33] = 3; cursor[35] = 0xe2;
        }
        cursor += stride;
    }
    const void *cells[] = {stream};
    uint32_t meshCount;
    CHECK(ImportVisitTerrainCells(cells, 1, vertices, SaveMaterial, NULL, &meshCount));
    CHECK(meshCount == 1);
    for (mode = 0; mode < 6; mode++) {
        for (variant = 0; variant < 4; variant++) {
            uint16_t clut = ImportMaterialClut(&s_keys[mode],
                RAGE_RENDER_ASSET_TERRAIN, (uint8_t)variant);
            if (clut != expected[mode][variant]) {
                fprintf(stderr, "mode %u variant %u: CLUT %04x, expected %04x\n",
                        mode, variant, clut, expected[mode][variant]);
                return 1;
            }
        }
    }
    /* These pairs share their base atlas/CLUT/window but need different
     * decoded images when the environment changes. */
    if (ImportTextureEqual(&s_keys[0], &s_keys[2]) ||
        ImportTextureEqual(&s_keys[1], &s_keys[4])) {
        fputs("fixed and environment-controlled terrain materials merged\n", stderr);
        return 1;
    }
    TerrainBank bank = {.cellCount = 1, .vertices = vertices, .cells = {stream}};
    RenderMeshInstance identity = {.assetKey = 88, .assetSet = RAGE_RENDER_ASSET_TERRAIN};
    RageImportedMeshEntry entry = {0};
    CHECK(!ImportBuildTerrainMesh(&identity, NULL, &entry));
    CHECK(entry.cached.mesh.bytes == NULL && entry.materials == NULL);
    bank.cellCount = 2; /* Reject a missing late cell after visiting the first. */
    CHECK(!ImportBuildTerrainMesh(&identity, &bank, &entry));
    CHECK(entry.cached.mesh.bytes == NULL && entry.materials == NULL);
    bank.cellCount = 1;
    CHECK(ImportBuildTerrainMesh(&identity, &bank, &entry));
    CHECK(entry.cached.mesh.meshCount == 1 && entry.cached.mesh.vertexCount == 24);
    CHECK(entry.cached.mesh.indexCount == 36 && entry.materialCount == 6);
    const void *bytes = entry.cached.mesh.bytes;
    CHECK(!ImportBuildTerrainMesh(&identity, &bank, &entry));
    CHECK(entry.cached.mesh.bytes == bytes);
    memset(stream, 0xff, sizeof(stream));
    RageRuntimeVertex vertex;
    CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
    CHECK(vertex.position[0] == 11 && vertex.position[1] == 22 && vertex.position[2] == -33);
    RuntimeCachedMeshRelease(&entry.cached);
    free(entry.materials);
    puts("terrain stream modes retain their fixed or environment palette");
    return 0;
}
