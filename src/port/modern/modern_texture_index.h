#ifndef RAGE_MODERN_TEXTURE_INDEX_H
#define RAGE_MODERN_TEXTURE_INDEX_H

#include "render/render_local_geometry.h"

#include <stdint.h>

enum {
    MODERN_TEXTURE_INDEX_CAPACITY = 2048,
    MODERN_TEXTURE_INDEX_HASH_SIZE = 4096,
};

typedef struct ModernTextureKey {
    uint32_t assetKey;
    uint32_t material;
    RageRenderAssetSet assetSet;
    uint8_t variant;
    uint8_t hasCarPaint;
    uint8_t carPaintColor1;
    uint8_t carPaintColor2;
} ModernTextureKey;

typedef struct ModernTextureIndex {
    ModernTextureKey keys[MODERN_TEXTURE_INDEX_CAPACITY];
    uint16_t slots[MODERN_TEXTURE_INDEX_HASH_SIZE];
    uint32_t count;
} ModernTextureIndex;

ModernTextureKey ModernTextureKeyFromSpan(const RageNativeDrawSpan *span);
uint32_t ModernTextureKeyHash(const ModernTextureKey *key);
int ModernTextureKeyEqual(const ModernTextureKey *left,
                          const ModernTextureKey *right);
int ModernTextureIndexFind(const ModernTextureIndex *index,
                           const ModernTextureKey *key);
int ModernTextureIndexInsert(ModernTextureIndex *index,
                             const ModernTextureKey *key);
void ModernTextureIndexClear(ModernTextureIndex *index);

#endif
