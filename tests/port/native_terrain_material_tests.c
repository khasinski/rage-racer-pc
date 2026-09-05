/* Exercise the real stream parser and material-key resolution together.
 * Unused importer entry points are discarded by the test link. */
#include "../../src/port/native_asset_importer.c"

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
    SVec vertices[1] = {{0}};
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
    if (!ImportVisitTerrainStream(0, stream, vertices, SaveMaterial, NULL))
        return 1;
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
    puts("terrain stream modes retain their fixed or environment palette");
    return 0;
}
