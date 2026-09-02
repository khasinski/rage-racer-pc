#include <string.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/render.h"

static CarModelAsset s_NativeCarModelAssets[CAR_ASSET_SLOT_COUNT];
static CarModelAsset *s_SerializedCarModelAssets[CAR_ASSET_SLOT_COUNT];

void UploadCarImage(s32 index) {
    if ((u32)index >= CAR_ASSET_SLOT_COUNT) return;
    LoadImage(&g_CarImageRect, g_CarImageSlots[index]);
}

s32 InstallSerializedCarModelSlot(CarModelAsset *asset, s32 index) {
    SerializedCarModelAssetHeader *serialized;
    u8 *bytes;

    if (asset == NULL || (u32)index >= CAR_ASSET_SLOT_COUNT) return 0;

    serialized = GetSerializedCarModelAssetHeader(asset);
    if (asset->serializedModelSize < 0 ||
        (u32)asset->serializedModelSize >
            CAR_MODEL_SLOT_SIZE - SERIALIZED_CAR_MODEL_HEADER_SIZE -
                sizeof(CarImageData) ||
        serialized->modelOffset != SERIALIZED_CAR_MODEL_HEADER_SIZE ||
        serialized->imageOffset != SERIALIZED_CAR_MODEL_HEADER_SIZE +
                                       asset->serializedModelSize) {
        return 0;
    }

    bytes = GetAssetBytes(serialized);
    memcpy(&s_NativeCarModelAssets[index], serialized->metadata,
           sizeof(serialized->metadata));
    s_NativeCarModelAssets[index].modelData.pointer =
        bytes + serialized->modelOffset;
    s_NativeCarModelAssets[index].imageData.pointer =
        bytes + serialized->imageOffset;
    s_SerializedCarModelAssets[index] = asset;
    g_CarModelSlots[index] = &s_NativeCarModelAssets[index];
    return 1;
}

CarModelAsset *FindSerializedCarModelAsset(CarModelAsset *nativeAsset) {
    u32 i;

    for (i = 0; i < CAR_ASSET_SLOT_COUNT; i++) {
        if (nativeAsset == g_CarModelSlots[i]) {
            return s_SerializedCarModelAssets[i];
        }
    }
    return NULL;
}

void SelectCarModelSlot(s32 index) {
    if ((u32)index >= CAR_ASSET_SLOT_COUNT) return;
    g_CarModelAsset = g_CarModelSlots[index];
}
