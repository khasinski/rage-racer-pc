#include "modern_texture_index.h"

#include <assert.h>
#include <string.h>

static RageNativeDrawSpan Span(uint32_t key, uint32_t material,
                               uint8_t variant, uint8_t paint) {
    RageNativeDrawSpan span = {0};
    span.assetKey = key;
    span.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    span.material = material;
    span.materialVariant = variant;
    span.hasCarPaint = paint != 0;
    span.carPaintColor1 = paint;
    span.carPaintColor2 = (uint8_t)(paint + 1);
    return span;
}

int main(void) {
    ModernTextureIndex index = {0};
    RageNativeDrawSpan firstSpan = Span(17, 4, 2, 5);
    RageNativeDrawSpan sameSpan = Span(17, 4, 2, 5);
    RageNativeDrawSpan variantSpan = Span(17, 4, 3, 5);
    RageNativeDrawSpan paintSpan = Span(17, 4, 2, 6);
    ModernTextureKey first = ModernTextureKeyFromSpan(&firstSpan);
    ModernTextureKey same = ModernTextureKeyFromSpan(&sameSpan);
    ModernTextureKey variant = ModernTextureKeyFromSpan(&variantSpan);
    ModernTextureKey paint = ModernTextureKeyFromSpan(&paintSpan);
    ModernTextureKey collision = {0};
    uint32_t key;

    assert(ModernTextureKeyEqual(&first, &same));
    assert(ModernTextureKeyHash(&first) == ModernTextureKeyHash(&same));
    assert(!ModernTextureKeyEqual(&first, &variant));
    assert(!ModernTextureKeyEqual(&first, &paint));
    assert(ModernTextureIndexFind(&index, &first) == -1);
    assert(ModernTextureIndexInsert(&index, &first));
    assert(index.count == 1 && ModernTextureIndexFind(&index, &first) == 0);
    assert(ModernTextureIndexInsert(&index, &same) && index.count == 1);
    assert(ModernTextureIndexInsert(&index, &variant));
    assert(ModernTextureIndexInsert(&index, &paint));
    assert(index.count == 3 && ModernTextureIndexFind(&index, &variant) == 1 &&
           ModernTextureIndexFind(&index, &paint) == 2);

    for (key = 1; key < 100000; ++key) {
        RageNativeDrawSpan candidate = Span(key, 9, 0, 0);
        collision = ModernTextureKeyFromSpan(&candidate);
        if ((ModernTextureKeyHash(&collision) &
             (MODERN_TEXTURE_INDEX_HASH_SIZE - 1u)) ==
            (ModernTextureKeyHash(&first) &
             (MODERN_TEXTURE_INDEX_HASH_SIZE - 1u))) break;
    }
    assert(key < 100000 && !ModernTextureKeyEqual(&first, &collision));
    assert(ModernTextureIndexInsert(&index, &collision));
    assert(ModernTextureIndexFind(&index, &collision) == 3 &&
           ModernTextureIndexFind(&index, &first) == 0);

    memset(&index, 0, sizeof(index));
    index.count = MODERN_TEXTURE_INDEX_CAPACITY;
    assert(!ModernTextureIndexInsert(&index, &first));
    ModernTextureIndexClear(&index);
    assert(index.count == 0 && ModernTextureIndexFind(&index, &first) == -1);
    return 0;
}
