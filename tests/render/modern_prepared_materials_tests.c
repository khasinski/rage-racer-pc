#include "modern_prepared_materials.h"

#include <SDL3/SDL.h>

#include <assert.h>
#include <string.h>

static ModernAssetImage Image(void) {
    ModernAssetImage image = {0};
    image.pixels = SDL_malloc(4);
    assert(image.pixels != NULL);
    memset(image.pixels, 0x5A, 4);
    image.size = 4;
    image.width = 1;
    image.height = 1;
    return image;
}

int main(void) {
    ModernPreparedMaterials cache = {0};
    RageRenderMeshInstance instance = {
        .assetKey = 17,
        .assetSet = RAGE_RENDER_ASSET_MODEL_BANK,
        .hasCarPaint = 1,
        .carPaintColor1 = 2,
        .carPaintColor2 = 5,
    };
    RageRenderMaterial definition = {0};
    RageRenderMaterialStorage storage = {0};
    ModernAssetImage image = Image();
    void *cachedPixels = image.pixels;
    const char texture[] = "textures/body.png";

    definition.baseColorTexture.text = texture;
    definition.baseColorTexture.length = sizeof(texture) - 1;
    assert(ModernPreparedMaterialsStore(&cache, &instance, 9, 3,
                                        &definition, &image, &storage));
    assert(cache.count == 1);
    assert(cache.entries[0].image.pixels == cachedPixels);
    assert(image.pixels != cachedPixels);
    assert(definition.baseColorTexture.text == storage.baseColorTexture);
    assert(strcmp(definition.baseColorTexture.text, texture) == 0);
    assert(ModernPreparedMaterialsContains(&cache, &instance, 9, 3));
    instance.carPaintColor2 = 6;
    assert(!ModernPreparedMaterialsContains(&cache, &instance, 9, 3));
    instance.carPaintColor2 = 5;

    SDL_free(image.pixels);
    image = (ModernAssetImage){0};
    memset(&definition, 0, sizeof(definition));
    memset(&storage, 0, sizeof(storage));
    assert(ModernPreparedMaterialsCopy(&cache, &instance, 9, 3,
                                       &definition, &image, &storage));
    assert(image.pixels != cachedPixels);
    assert(strcmp(definition.baseColorTexture.text, texture) == 0);
    SDL_free(image.pixels);
    ModernPreparedMaterialsClear(&cache);
    assert(cache.count == 0 && cache.bytes == 0 && !cache.budgetReached);

    image = Image();
    cache.bytes = MODERN_PREPARED_MATERIAL_BYTE_LIMIT;
    assert(ModernPreparedMaterialsStore(&cache, &instance, 9, 3,
                                        &definition, &image, &storage));
    assert(cache.budgetReached && cache.count == 0);
    SDL_free(image.pixels);
    ModernPreparedMaterialsClear(&cache);
    return 0;
}
