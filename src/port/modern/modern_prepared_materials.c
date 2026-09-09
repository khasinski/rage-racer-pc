#include "modern_prepared_materials.h"

#include <SDL3/SDL.h>

#include <string.h>

static const ModernPreparedMaterialEntry *Find(
    const ModernPreparedMaterials *cache,
    const RageRenderMeshInstance *instance, uint32_t material, uint8_t variant) {
    uint32_t i;
    if (!cache || !instance) return NULL;
    for (i = 0; i < cache->count; ++i) {
        const ModernPreparedMaterialEntry *entry = &cache->entries[i];
        if (entry->assetKey == instance->assetKey &&
            entry->assetSet == instance->assetSet &&
            entry->material == material && entry->variant == variant &&
            entry->hasCarPaint == instance->hasCarPaint &&
            entry->color1 == instance->carPaintColor1 &&
            entry->color2 == instance->carPaintColor2)
            return entry;
    }
    return NULL;
}

static int CopyEntry(const ModernPreparedMaterialEntry *entry,
                     RageRenderMaterial *definition, ModernAssetImage *image,
                     RageRenderMaterialStorage *storage) {
    void *pixels;
    if (!entry || !definition || !image || !storage ||
        !ModernAssetImageValidRGBA(&entry->image)) return 0;
    pixels = SDL_malloc(entry->image.size);
    if (!pixels) return 0;
    memcpy(pixels, entry->image.pixels, entry->image.size);
    *definition = entry->definition;
    *storage = entry->storage;
    if (!RenderMaterialStorePaths(definition, storage)) {
        SDL_free(pixels);
        return 0;
    }
    *image = entry->image;
    image->pixels = pixels;
    return 1;
}

void ModernPreparedMaterialsClear(ModernPreparedMaterials *cache) {
    uint32_t i;
    if (!cache) return;
    for (i = 0; i < cache->count; ++i) SDL_free(cache->entries[i].image.pixels);
    memset(cache, 0, sizeof(*cache));
}

int ModernPreparedMaterialsContains(const ModernPreparedMaterials *cache,
                                    const RageRenderMeshInstance *instance,
                                    uint32_t material, uint8_t variant) {
    return Find(cache, instance, material, variant) != NULL;
}

int ModernPreparedMaterialsCopy(const ModernPreparedMaterials *cache,
                                const RageRenderMeshInstance *instance,
                                uint32_t material, uint8_t variant,
                                RageRenderMaterial *definition,
                                ModernAssetImage *image,
                                RageRenderMaterialStorage *storage) {
    return CopyEntry(Find(cache, instance, material, variant), definition,
                     image, storage);
}

int ModernPreparedMaterialsStore(ModernPreparedMaterials *cache,
                                 const RageRenderMeshInstance *instance,
                                 uint32_t material, uint8_t variant,
                                 RageRenderMaterial *definition,
                                 ModernAssetImage *image,
                                 RageRenderMaterialStorage *storage) {
    ModernPreparedMaterialEntry *entry;
    if (!cache || !instance || !definition || !image || !storage) return 0;
    if (cache->count == MODERN_PREPARED_MATERIAL_LIMIT ||
        image->size > MODERN_PREPARED_MATERIAL_BYTE_LIMIT - cache->bytes) {
        cache->budgetReached = 1;
        return 1;
    }
    entry = &cache->entries[cache->count++];
    entry->assetKey = instance->assetKey;
    entry->assetSet = instance->assetSet;
    entry->material = material;
    entry->variant = variant;
    entry->hasCarPaint = instance->hasCarPaint;
    entry->color1 = instance->carPaintColor1;
    entry->color2 = instance->carPaintColor2;
    entry->definition = *definition;
    entry->storage = *storage;
    if (!RenderMaterialStorePaths(&entry->definition, &entry->storage)) {
        memset(entry, 0, sizeof(*entry));
        --cache->count;
        return 1;
    }
    entry->image = *image;
    if (CopyEntry(entry, definition, image, storage)) {
        cache->bytes += entry->image.size;
        return 1;
    }
    memset(entry, 0, sizeof(*entry));
    --cache->count;
    return 1;
}

int ModernPreparedMaterialsBudgetReached(const ModernPreparedMaterials *cache) {
    return cache && cache->budgetReached;
}
