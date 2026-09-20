#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"

#include <string.h>

static s32 ActiveCarModelSlotAssetIndex(void) {
    u32 slot;

    for (slot = 0; slot < CAR_ASSET_SLOT_COUNT; slot++) {
        if (g_CarModelSlots[slot] == g_CarModelAsset) {
            return g_CarModelSlotAssetIndex[slot];
        }
    }
    return -1;
}

s32 RelocateCarModel(void) {
    const CarModelAsset *source =
        FindSerializedCarModelAsset(g_CarModelAsset);
    s32 assetIndex = ActiveCarModelSlotAssetIndex();
    const SerializedCarModelAssetHeader *serialized;
    const ModelBankHeader *sourceBank;
    CarModelAsset *destination;
    CarImageData *sourceImage;
    u8 metadata[sizeof(((SerializedCarModelAssetHeader *)0)->metadata)];
    size_t modelDataSize;
    size_t byteCount;

    if (!IsValidSerializedCarModelAsset(source, CAR_MODEL_SLOT_SIZE)) {
        return 0;
    }
    serialized = (const SerializedCarModelAssetHeader *)(const void *)source;
    modelDataSize = (size_t)source->serializedModelSize;
    sourceBank = GetModelBankHeader(
        (const u8 *)source + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    if (!IsValidModelBankAsset(sourceBank, modelDataSize)) {
        return 0;
    }
    /* Retail moves the header and the model only, and the next loads start
     * right behind them: the image stays with the source, which is where the
     * slot keeps pointing for it. Carrying the image along pushed every race
     * load 29 KB down the boot buffer, and the largest track pack (class 4
     * course 1) no longer fit. */
    byteCount = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                modelDataSize;
    if (PortAssetRoomAt(g_AssetBase) < byteCount) return 0;

    memcpy(metadata, serialized->metadata, sizeof(metadata));
    sourceImage = (CarImageData *)(void *)(
        (u8 *)(void *)source + serialized->imageOffset);
    memmove(g_AssetBase, source, byteCount);
    destination = GetCarModelAsset(g_AssetBase);

    if (!RegisterModelBank(
            GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE),
            modelDataSize, 0)) {
        return 0;
    }
    if (!PublishCarModelSlot(source, metadata,
                            (u8 *)destination +
                                SERIALIZED_CAR_MODEL_HEADER_SIZE,
                            sourceImage, 0)) {
        return 0;
    }
    g_AssetLoadCursor = g_AssetBase + byteCount;
    SelectCarModelSlot(0);
    /* The race draws from bank 0 whichever showroom slot the menu left
     * active; keep the slot index and the car identity in step with it. */
    g_CarModelSlot = 0;
    g_CarModelSlotAssetIndex[0] = assetIndex;
    return 1;
}
