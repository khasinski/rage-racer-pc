#include "client_race.h"
#include "render/car_paint.h"
#include <stdlib.h>

const RageImportedMeshEntry *FindClientMesh(const ClientRace *race,
                                           const RenderMeshInstance *instance) {
    if (!race || !instance || instance->assetSource != RENDER_ASSET_OWNED) return NULL;
    const RageImportedMeshEntry *entry = NULL;
    switch (instance->assetSet) {
    case RAGE_RENDER_ASSET_MODEL_BANK:
        if (race->carMeshCount > CAR_MODEL_VARIANT_COUNT) return NULL;
        for (u32 car = 0; car < race->carMeshCount; ++car)
            if (race->carMeshes[car].cached.assetKey == instance->assetKey) {
                entry = &race->carMeshes[car]; break;
            }
        break;
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1: entry = &race->primaryMesh; break;
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2: entry = &race->secondaryMesh; break;
    case RAGE_RENDER_ASSET_COURSE: entry = &race->courseMesh; break;
    case RAGE_RENDER_ASSET_TERRAIN: entry = &race->terrainMesh; break;
    default: return NULL;
    }
    if (!entry || entry->source != RENDER_ASSET_OWNED || !entry->cached.mesh.bytes ||
        entry->cached.assetKey != instance->assetKey ||
        entry->cached.assetSet != instance->assetSet ||
        instance->mesh >= entry->cached.mesh.meshCount) return NULL;
    return entry;
}

int DecodeClientMaterial(const ClientRace *race, const RenderMeshInstance *instance,
                          u32 material, int page, const u16 *palette,
                          u8 *rgba, size_t size) {
    const RageImportedMeshEntry *entry = FindClientMesh(race, instance);
    if (!entry || !entry->materials || material >= entry->materialCount ||
        page < 0 || page > 1) return 0;
    const RageImportedTextureKey *texture = &entry->materials[material];
    if (instance->assetSet != RAGE_RENDER_ASSET_MODEL_BANK)
        return DecodeTrackMaterial(race->pixels, texture, instance->assetSet,
                                    instance->materialVariant, page, palette, rgba, size);
    if (!rgba || size < 256u * 256u * 4u || !entry->carSource ||
        (instance->hasCarPaint && (instance->carPaintColor1 >= RAGE_CAR_PAINT_COLOR_COUNT ||
                                  instance->carPaintColor2 >= RAGE_CAR_PAINT_COLOR_COUNT))) return 0;
    TextureImage images[3];
    if (!CarTextureImages(entry->carSource, images)) return 0;
    u8 *mask = instance->hasCarPaint ? malloc(256u * 256u) : NULL;
    if (instance->hasCarPaint && !mask) return 0;
    int decoded = DecodeTexture(texture, texture->clut, images, rgba, size,
                                 mask, mask ? 256u * 256u : 0);
    if (decoded && mask)
        decoded = CarPaintApply(rgba, mask, 256u * 256u,
                                instance->carPaintColor1, instance->carPaintColor2);
    free(mask);
    return decoded;
}
