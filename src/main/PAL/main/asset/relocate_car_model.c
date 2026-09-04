#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"

#include <string.h>

s32 RelocateCarModel(void) {
    const CarModelAsset *source =
        FindSerializedCarModelAsset(g_CarModelAsset);
    const ModelBankHeader *sourceBank;
    CarModelAsset *destination;
    size_t byteCount;

    if (!IsValidSerializedCarModelAsset(source, CAR_MODEL_SLOT_SIZE)) {
        return 0;
    }
    sourceBank = GetModelBankHeader(
        (const u8 *)source + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    if (!IsValidModelBankAsset(sourceBank,
                               (size_t)source->serializedModelSize)) {
        return 0;
    }
    /* Retail moves the header and the model only, and the next loads start
     * right behind them: the image stays with the source, which is where the
     * slot keeps pointing for it. Carrying the image along pushed every race
     * load 29 KB down the boot buffer, and the largest track pack (class 4
     * course 1) no longer fit. */
    byteCount = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                (size_t)source->serializedModelSize;
    if (PortAssetRoomAt(g_AssetBase) < byteCount) return 0;

    memmove(g_AssetBase, source, byteCount);
    destination = GetCarModelAsset(g_AssetBase);

    if (!RegisterModelBank(
            GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE),
            (size_t)source->serializedModelSize, 0)) {
        return 0;
    }
    if (!InstallSerializedCarModelSlot(
            (CarModelAsset *)source,
            byteCount + sizeof(CarImageData), 0)) {
        return 0;
    }
    g_AssetLoadCursor = g_AssetBase + byteCount;
    SelectCarModelSlot(0);
    g_CarModelAsset->modelData.pointer =
        (u8 *)destination + SERIALIZED_CAR_MODEL_HEADER_SIZE;
    return 1;
}
