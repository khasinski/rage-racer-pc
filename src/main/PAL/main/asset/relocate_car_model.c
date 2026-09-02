#include "game/asset.h"
#include "game/car.h"

#include <string.h>

void RelocateCarModel(void) {
    CarModelAsset *source = GetSerializedCarModelAsset(g_CarModelAsset);
    u32 byteCount = (u32)source->serializedModelSize +
                    SERIALIZED_CAR_MODEL_HEADER_SIZE;

    g_AssetLoadCursor = g_AssetBase + byteCount;
    memcpy(g_AssetBase, source, byteCount);

    SetCarModelSlot(GetCarModelAsset(g_AssetBase), 0);
    SelectCarModelSlot(0);
    g_CarModelAsset->modelData.pointer =
        g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE;
    RegisterModelBank(
        GetModelBankHeader(g_AssetBase + SERIALIZED_CAR_MODEL_HEADER_SIZE), 0);
}
