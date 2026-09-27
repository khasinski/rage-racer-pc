#include "native_mesh_writer.h"
#include "game/car_model_data.h"
#include "game/asset_index.h"
#include <stdlib.h>
#include <string.h>

void ImportReleaseEntry(RageImportedMeshEntry *entry) {
    if (!entry) return;
    RuntimeCachedMeshRelease(&entry->cached);
    free(entry->materials);
    FreeCarModelData(entry->carSource);
    memset(entry, 0, sizeof(*entry));
}

RageImportedMeshEntry *ImportFindMesh(RageImportedMeshEntry *entries, uint32_t count,
                                    uint32_t key, RenderAssetSet set, RenderAssetSource source) {
    if (!entries || (unsigned)source >= RENDER_ASSET_SOURCE_COUNT) return NULL;
    for (uint32_t i = 0; i < count; ++i)
        if (entries[i].cached.assetKey == key && entries[i].cached.assetSet == set &&
            entries[i].source == source) return &entries[i];
    return NULL;
}

int ImportPrepareCars(RageImportedMeshEntry *entries, uint32_t *count,
                      uint32_t capacity, CarModelData *const *models) {
    if (!entries || !count || !models || *count > capacity) return 0;
    const uint32_t initialCount = *count;
    for (uint32_t variant = 0; variant < CAR_MODEL_VARIANT_COUNT; ++variant) {
        const CarModelData *model = models[variant];
        if (!model) continue;
        RenderMeshInstance instance = {
            .assetSource = RENDER_ASSET_OWNED,
            .assetSet = RAGE_RENDER_ASSET_MODEL_BANK,
            .assetKey = (uint32_t)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, (s32)variant)};
        uint32_t index;
        for (index = 0; index < *count; ++index) {
            RageImportedMeshEntry *entry = &entries[index];
            if (!entry->carSource || entry->cached.assetKey != instance.assetKey ||
                entry->cached.assetSet != instance.assetSet) continue;
            if (entry->carSource->size != model->size ||
memcmp(entry->carSource->bytes, model->bytes, model->size) != 0 ||
entry->carSource->hasSharedImage != model->hasSharedImage ||
memcmp(entry->carSource->sharedImage, model->sharedImage, sizeof(model->sharedImage)) != 0 ||
memcmp(entry->carSource->logoPalette, model->logoPalette, sizeof(model->logoPalette)) != 0)
                goto fail;
            break;
        }
        if (index < *count) continue;
        if (*count == capacity) goto fail;
        CarModelData *owned = CopyCarModelData(model->bytes, model->size);
        if (!owned) goto fail;
        memcpy(owned->sharedImage, model->sharedImage, sizeof(model->sharedImage));
        memcpy(owned->logoPalette, model->logoPalette, sizeof(model->logoPalette));
        owned->hasSharedImage = model->hasSharedImage;
        if (!ImportBuildBankMesh(&instance, &owned->bank, &entries[*count])) {
            FreeCarModelData(owned);
            goto fail;
        }
        entries[*count].carSource = owned;
        ++*count;
    }
    return 1;
fail:
    while (*count > initialCount) ImportReleaseEntry(&entries[--*count]);
    return 0;
}
