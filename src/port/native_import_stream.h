#ifndef RAGE_NATIVE_IMPORT_STREAM_H
#define RAGE_NATIVE_IMPORT_STREAM_H

/* Shared stream parsing used by the importer and its disc-free fixture. */
#include <string.h>
#include "native_mesh_writer.h"
#include "game/terrain_internal.h"

enum { RAGE_IMPORT_BATCH_GUARD = 65536 };

typedef int (*RageImportedFaceVisitor)(uint32_t mesh,
                                       const RageImportedFace *face,
                                       void *context);

static uint16_t ImportRead16(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ImportRead32(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


static void ImportTextureWindow(uint32_t word,
                                    RageImportedTextureKey *texture) {
    uint32_t value, maskU, maskV, offsetU, offsetV;
    if ((word >> 24) != 0xE2) return;
    value = word & 0xFFFFFu;
    maskU = value & 0x1Fu;
    maskV = (value >> 5) & 0x1Fu;
    if (maskU == 0 && maskV == 0) return;
    offsetU = (value >> 10) & 0x1Fu;
    offsetV = (value >> 15) & 0x1Fu;
    texture->hasWindow = 1;
    texture->windowWidthU = (uint16_t)(256u - maskU * 8u);
    texture->windowWidthV = (uint16_t)(256u - maskV * 8u);
    texture->windowOffsetU = (uint16_t)((offsetU & maskU) * 8u);
    texture->windowOffsetV = (uint16_t)((offsetV & maskV) * 8u);
}

static int ImportVisitTerrainStream(
    uint32_t mesh, const uint8_t *stream, const SVec *vertices,
    RageImportedFaceVisitor visitor, void *context) {
    uint32_t batches = 0;
    while (batches++ < RAGE_IMPORT_BATCH_GUARD) {
        uint16_t prim = ImportRead16(stream);
        uint16_t count = ImportRead16(stream + 2);
        uint16_t face;
        stream += 4;
        if (count == 0) return 1;
        const s32 stride = TerrainPrimitiveStride(prim);
        if (stride == 0) return 0;
        for (face = 0; face < count; face++, stream += stride) {
            RageImportedFace value;
            uint32_t corner;
            static const uint8_t offsets[] = {8, 0x0C, 0x10, 0x12};
            memset(&value, 0, sizeof(value));
            value.vertices = vertices;
            value.prim = (uint8_t)prim;
            value.textured = 1;
            value.depthBias = (int8_t)stream[0x15];
            value.flags = stream[0x14];
            memcpy(value.color, stream + 0x1C, 3);
            for (corner = 0; corner < 4; corner++) {
                value.vertex[corner] = ImportRead16(stream + corner * 2);
                value.uv[corner][0] = stream[offsets[corner]];
                value.uv[corner][1] = stream[offsets[corner] + 1];
            }
            value.texture.clut = ImportRead16(stream + 0x0A);
            /* Modes 2..5 encode their palette directly. Only 0/1 follow
             * envMode4, as in the retail terrain dispatch. Keep that choice
             * in the material key even when the base atlas and CLUT match. */
            value.texture.terrainEnvironmentClut = prim < 2;
            if (prim >= 2)
                value.texture.clut = (uint16_t)(value.texture.clut +
                                                 ((prim - 2) & 1));
            value.texture.tpage = ImportRead16(stream + 0x0E);
            if (stride == 0x24)
                ImportTextureWindow(ImportRead32(stream + 0x20),
                                        &value.texture);
            if (!visitor(mesh, &value, context)) return 0;
        }
    }
    return 0;
}

static uint16_t ImportMaterialClut(const RageImportedTextureKey *texture,
                                   RageRenderAssetSet assetSet,
                                   uint8_t variant) {
    uint32_t offset = 0;
    if (assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)
        offset = variant % 3u;
    else if (assetSet == RAGE_RENDER_ASSET_COURSE)
        offset = variant % 4u;
    else if (assetSet == RAGE_RENDER_ASSET_TERRAIN &&
             texture->terrainEnvironmentClut)
        offset = variant % 2u;
    return (uint16_t)(texture->clut + offset);
}

#endif
