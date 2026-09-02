#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"

#include <string.h>

void RelocateCarModel(void) {
    CarModelAsset *source = FindSerializedCarModelAsset(g_CarModelAsset);
    ModelBankHeader *sourceBank;
    CarModelAsset *destination;
    size_t byteCount;

    if (!IsValidSerializedCarModelAsset(source, CAR_MODEL_SLOT_SIZE)) {
        return;
    }
    sourceBank = GetModelBankHeader(
        GetAssetBytes(source) + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    if (!IsValidModelBankAsset(sourceBank,
                               (size_t)source->serializedModelSize)) {
        return;
    }
    byteCount = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                (size_t)source->serializedModelSize;

    memcpy(g_AssetBase, source, byteCount);
    destination = GetCarModelAsset(g_AssetBase);

    if (!RegisterModelBank(
            GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE),
            (size_t)source->serializedModelSize, 0)) {
        return;
    }
    if (!InstallSerializedCarModelSlot(destination, 0)) {
        return;
    }
    g_AssetLoadCursor = g_AssetBase + byteCount;
    SelectCarModelSlot(0);
}
