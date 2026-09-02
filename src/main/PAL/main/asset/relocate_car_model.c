#include "game/asset.h"
#include "game/car.h"

#include <string.h>

void RelocateCarModel(void) {
    CarModelAsset *source = FindSerializedCarModelAsset(g_CarModelAsset);
    size_t byteCount;

    if (source == NULL || source->serializedModelSize < 0 ||
        source->serializedModelSize >
            CAR_MODEL_SLOT_SIZE - SERIALIZED_CAR_MODEL_HEADER_SIZE) {
        return;
    }
    byteCount = SERIALIZED_CAR_MODEL_HEADER_SIZE +
                (size_t)source->serializedModelSize;

    g_AssetLoadCursor = g_AssetBase + byteCount;
    memcpy(g_AssetBase, source, byteCount);

    if (!InstallSerializedCarModelSlot(GetCarModelAsset(g_AssetBase), 0)) {
        return;
    }
    SelectCarModelSlot(0);
    g_CarModelAsset->modelData.pointer =
        g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE;
    RegisterModelBank(
        GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE), 0);
}
