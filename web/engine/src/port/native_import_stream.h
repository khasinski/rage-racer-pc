#ifndef RAGE_NATIVE_IMPORT_STREAM_H
#define RAGE_NATIVE_IMPORT_STREAM_H

/* Palette selection shared by the importer and its disc-free fixture. */
#include <string.h>
#include "native_mesh_writer.h"
#include "game/model_stream.h"


static uint16_t ImportMaterialClut(const RageImportedTextureKey *texture,
                                   RenderAssetSet assetSet,
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
