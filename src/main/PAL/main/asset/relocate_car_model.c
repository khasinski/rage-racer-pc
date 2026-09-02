#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"

#include <string.h>

s32 RelocateCarModel(void) {
    CarModelAsset *source = FindSerializedCarModelAsset(g_CarModelAsset);
    ModelBankHeader *sourceBank;
    CarModelAsset *destination;
    size_t byteCount;

    if (!IsValidSerializedCarModelAsset(source, CAR_MODEL_SLOT_SIZE)) {
        return 0;
    }
    sourceBank = GetModelBankHeader(
        GetAssetBytes(source) + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    if (!IsValidModelBankAsset(sourceBank,
                               (size_t)source->serializedModelSize)) {
        return 0;
    }
    byteCount = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                (size_t)source->serializedModelSize;
    if (PortAssetRoomAt(g_AssetBase) < byteCount) return 0;

    memcpy(g_AssetBase, source, byteCount);
    destination = GetCarModelAsset(g_AssetBase);

    if (!RegisterModelBank(
            GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE),
            (size_t)source->serializedModelSize, 0)) {
        return 0;
    }
    if (!InstallSerializedCarModelSlot(destination, 0)) {
        return 0;
    }
    g_AssetLoadCursor = g_AssetBase + byteCount;
    SelectCarModelSlot(0);
    return 1;
}
