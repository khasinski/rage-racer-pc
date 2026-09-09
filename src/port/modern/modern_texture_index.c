#include "modern_texture_index.h"

#include <string.h>

ModernTextureKey ModernTextureKeyFromSpan(const RageNativeDrawSpan *span) {
    ModernTextureKey key = {0};
    if (!span) return key;
    key.assetKey = span->assetKey;
    key.assetSet = span->assetSet;
    key.material = span->material;
    key.variant = span->materialVariant;
    key.hasCarPaint = span->hasCarPaint;
    key.carPaintColor1 = span->carPaintColor1;
    key.carPaintColor2 = span->carPaintColor2;
    return key;
}

uint32_t ModernTextureKeyHash(const ModernTextureKey *key) {
    uint32_t hash;
    if (!key) return 0;
    hash = key->assetKey * 0x9E3779B1u;
    hash ^= (uint32_t)key->assetSet * 0x85EBCA77u;
    hash ^= key->material * 0xC2B2AE3Du;
    hash ^= (uint32_t)key->variant << 24;
    hash ^= (uint32_t)key->hasCarPaint << 23;
    hash ^= (uint32_t)key->carPaintColor1 << 8;
    hash ^= (uint32_t)key->carPaintColor2 << 16;
    return hash ^ (hash >> 16);
}

int ModernTextureKeyEqual(const ModernTextureKey *left,
                          const ModernTextureKey *right) {
    return left && right && left->assetKey == right->assetKey &&
           left->assetSet == right->assetSet && left->material == right->material &&
           left->variant == right->variant &&
           left->hasCarPaint == right->hasCarPaint &&
           left->carPaintColor1 == right->carPaintColor1 &&
           left->carPaintColor2 == right->carPaintColor2;
}

int ModernTextureIndexFind(const ModernTextureIndex *index,
                           const ModernTextureKey *key) {
    uint32_t hash, probe;
    if (!index || !key) return -1;
    hash = ModernTextureKeyHash(key);
    for (probe = 0; probe < MODERN_TEXTURE_INDEX_HASH_SIZE; ++probe) {
        uint16_t stored = index->slots[(hash + probe) &
                                        (MODERN_TEXTURE_INDEX_HASH_SIZE - 1u)];
        if (stored == 0) return -1;
        if (ModernTextureKeyEqual(&index->keys[stored - 1u], key))
            return (int)(stored - 1u);
    }
    return -1;
}

int ModernTextureIndexInsert(ModernTextureIndex *index,
                             const ModernTextureKey *key) {
    uint32_t hash, probe, slot;
    if (!index || !key || index->count >= MODERN_TEXTURE_INDEX_CAPACITY)
        return 0;
    if (ModernTextureIndexFind(index, key) >= 0) return 1;
    hash = ModernTextureKeyHash(key);
    for (probe = 0; probe < MODERN_TEXTURE_INDEX_HASH_SIZE; ++probe) {
        slot = (hash + probe) & (MODERN_TEXTURE_INDEX_HASH_SIZE - 1u);
        if (index->slots[slot] == 0) {
            index->keys[index->count] = *key;
            index->slots[slot] = (uint16_t)(index->count + 1u);
            ++index->count;
            return 1;
        }
    }
    return 0;
}

void ModernTextureIndexClear(ModernTextureIndex *index) {
    if (index) memset(index, 0, sizeof(*index));
}
