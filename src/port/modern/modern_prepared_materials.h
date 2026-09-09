#ifndef RAGE_MODERN_PREPARED_MATERIALS_H
#define RAGE_MODERN_PREPARED_MATERIALS_H

#include "modern_asset_image.h"
#include "render/render_material.h"
#include "render/render_material_storage.h"
#include "render/render_world.h"

#include <stddef.h>
#include <stdint.h>

enum {
    MODERN_PREPARED_MATERIAL_LIMIT = 128,
    MODERN_PREPARED_MATERIAL_BYTE_LIMIT = 32 * 1024 * 1024,
};

typedef struct ModernPreparedMaterialEntry {
    uint32_t assetKey;
    uint32_t material;
    RageRenderAssetSet assetSet;
    uint8_t variant;
    uint8_t hasCarPaint;
    uint8_t color1;
    uint8_t color2;
    RageRenderMaterial definition;
    RageRenderMaterialStorage storage;
    ModernAssetImage image;
} ModernPreparedMaterialEntry;

typedef struct ModernPreparedMaterials {
    ModernPreparedMaterialEntry entries[MODERN_PREPARED_MATERIAL_LIMIT];
    uint32_t count;
    size_t bytes;
    int budgetReached;
} ModernPreparedMaterials;

void ModernPreparedMaterialsClear(ModernPreparedMaterials *cache);
int ModernPreparedMaterialsContains(const ModernPreparedMaterials *cache,
                                    const RageRenderMeshInstance *instance,
                                    uint32_t material, uint8_t variant);
int ModernPreparedMaterialsCopy(const ModernPreparedMaterials *cache,
                                const RageRenderMeshInstance *instance,
                                uint32_t material, uint8_t variant,
                                RageRenderMaterial *definition,
                                ModernAssetImage *image,
                                RageRenderMaterialStorage *storage);
/* On a full cache or a failed private copy, caller ownership remains with
 * image. A successful cached copy replaces image with independent pixels. */
int ModernPreparedMaterialsStore(ModernPreparedMaterials *cache,
                                 const RageRenderMeshInstance *instance,
                                 uint32_t material, uint8_t variant,
                                 RageRenderMaterial *definition,
                                 ModernAssetImage *image,
                                 RageRenderMaterialStorage *storage);
int ModernPreparedMaterialsBudgetReached(const ModernPreparedMaterials *cache);

#endif
