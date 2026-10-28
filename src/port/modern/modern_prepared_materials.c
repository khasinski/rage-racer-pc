#include "modern_prepared_materials.h"

#include <SDL3/SDL.h>

#include <string.h>

static uint32_t HashKey(const RenderMeshInstance *instance,
                        uint32_t material, uint8_t variant) {
    uint32_t h = instance->assetKey * 2654435761u;
    h ^= (uint32_t)instance->assetSet * 2246822519u;
    h ^= material * 3266489917u;
    h ^= (uint32_t)variant * 668265263u;
    h ^= ((uint32_t)instance->hasCarPaint << 16) |
         ((uint32_t)instance->carPaintColor1 << 8) |
         (uint32_t)instance->carPaintColor2;
    return h ^ (h >> 16);
}

static const ModernPreparedMaterialEntry *Find(
    const ModernPreparedMaterials *cache,
    const RenderMeshInstance *instance, uint32_t material, uint8_t variant) {
    uint32_t mask = MODERN_PREPARED_MATERIAL_BUCKETS - 1u;
    uint32_t hash, step;
    if (!cache || !instance || cache->count == 0) return NULL;
    hash = HashKey(instance, material, variant);
    for (step = 0; step < MODERN_PREPARED_MATERIAL_BUCKETS; ++step) {
        uint16_t slot = cache->buckets[(hash + step) & mask];
        const ModernPreparedMaterialEntry *entry;
        if (slot == 0) return NULL;
        entry = &cache->entries[slot - 1u];
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

static void InsertBucket(ModernPreparedMaterials *cache,
                         const RenderMeshInstance *instance,
                         uint32_t material, uint8_t variant,
                         uint16_t oneBasedIndex) {
    uint32_t mask = MODERN_PREPARED_MATERIAL_BUCKETS - 1u;
    uint32_t hash = HashKey(instance, material, variant);
    uint32_t step;
    for (step = 0; step < MODERN_PREPARED_MATERIAL_BUCKETS; ++step) {
        uint32_t idx = (hash + step) & mask;
        if (cache->buckets[idx] == 0) {
            cache->buckets[idx] = oneBasedIndex;
            return;
        }
    }
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
                                    const RenderMeshInstance *instance,
                                    uint32_t material, uint8_t variant) {
    return Find(cache, instance, material, variant) != NULL;
}

int ModernPreparedMaterialsCopy(const ModernPreparedMaterials *cache,
                                const RenderMeshInstance *instance,
                                uint32_t material, uint8_t variant,
                                RageRenderMaterial *definition,
                                ModernAssetImage *image,
                                RageRenderMaterialStorage *storage) {
    return CopyEntry(Find(cache, instance, material, variant), definition,
                     image, storage);
}

int ModernPreparedMaterialsStore(ModernPreparedMaterials *cache,
                                 const RenderMeshInstance *instance,
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
        InsertBucket(cache, instance, material, variant, (uint16_t)cache->count);
        return 1;
    }
    memset(entry, 0, sizeof(*entry));
    --cache->count;
    return 1;
}

int ModernPreparedMaterialsBudgetReached(const ModernPreparedMaterials *cache) {
    return cache && cache->budgetReached;
}
