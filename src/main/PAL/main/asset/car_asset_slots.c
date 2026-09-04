#include <string.h>

#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/render.h"

static CarModelAsset s_NativeCarModelAssets[CAR_ASSET_SLOT_COUNT];
static const CarModelAsset *s_SerializedCarModelAssets[CAR_ASSET_SLOT_COUNT];

s32 IsValidSerializedCarModelAsset(const CarModelAsset *asset, size_t size) {
    const SerializedCarModelAssetHeader *serialized;
    size_t imageOffset;

    if (asset == NULL || size < SERIALIZED_CAR_MODEL_HEADER_SIZE ||
        size > CAR_MODEL_SLOT_SIZE ||
        asset->serializedModelSize < 0) {
        return 0;
    }

    serialized = (const SerializedCarModelAssetHeader *)(const void *)asset;
    imageOffset = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                  (size_t)asset->serializedModelSize;
    return imageOffset <= size && sizeof(CarImageData) <= size - imageOffset &&
           serialized->modelOffset == SERIALIZED_CAR_MODEL_HEADER_SIZE &&
           serialized->imageOffset == (s32)imageOffset;
}

void UploadCarImage(s32 index) {
    if ((u32)index >= CAR_ASSET_SLOT_COUNT || g_CarImageSlots[index] == NULL) {
        return;
    }
    LoadImage(&g_CarImageRect, g_CarImageSlots[index]);
}

s32 InstallSerializedCarModelSlot(CarModelAsset *asset, size_t size,
                                  s32 index) {
    SerializedCarModelAssetHeader *serialized;
    u8 *bytes;

    if ((u32)index >= CAR_ASSET_SLOT_COUNT ||
        !IsValidSerializedCarModelAsset(asset, size)) {
        return 0;
    }

    serialized = GetSerializedCarModelAssetHeader(asset);
    bytes = GetAssetBytes(serialized);
    return PublishCarModelSlot(
        asset, serialized->metadata, bytes + serialized->modelOffset,
        (CarImageData *)(void *)(bytes + serialized->imageOffset), index);
}

s32 PublishCarModelSlot(const CarModelAsset *serializedAsset,
                        const void *metadata, void *modelData,
                        CarImageData *imageData, s32 index) {
    CarModelAsset *nativeAsset;

    if ((u32)index >= CAR_ASSET_SLOT_COUNT || serializedAsset == NULL ||
        metadata == NULL || modelData == NULL || imageData == NULL) {
        return 0;
    }

    nativeAsset = &s_NativeCarModelAssets[index];
    memcpy(nativeAsset, metadata,
           sizeof(((SerializedCarModelAssetHeader *)0)->metadata));
    nativeAsset->modelData.pointer = modelData;
    nativeAsset->imageData.carImage = imageData;
    s_SerializedCarModelAssets[index] = serializedAsset;
    g_CarModelSlots[index] = nativeAsset;
    return 1;
}

const CarModelAsset *FindSerializedCarModelAsset(
    const CarModelAsset *nativeAsset) {
    u32 i;

    if (nativeAsset == NULL) return NULL;

    for (i = 0; i < CAR_ASSET_SLOT_COUNT; i++) {
        if (nativeAsset == g_CarModelSlots[i]) {
            return s_SerializedCarModelAssets[i];
        }
    }
    return NULL;
}

void SelectCarModelSlot(s32 index) {
    if ((u32)index >= CAR_ASSET_SLOT_COUNT || g_CarModelSlots[index] == NULL) {
        return;
    }
    g_CarModelAsset = g_CarModelSlots[index];
}
