#include "game/asset.h"
#include "game/car.h"

void RelocateCarModel(void) {
    u32 *destination;
    u32 *source;
    u32 byteCount;
    u32 wordCount;

    source = (u32 *)(void *)GetSerializedCarModelAsset(g_CarModelAsset);
    destination = (u32 *)(void *)g_AssetBase;
    byteCount = source[6] + 0x28;
    wordCount = byteCount >> 2;
    g_AssetLoadCursor = g_AssetBase + byteCount;

    while (wordCount-- != 0) {
        *destination++ = *source++;
    }

    SetCarModelSlot(GetCarModelAsset(g_AssetBase), 0);
    SelectCarModelSlot(0);
    g_CarModelAsset->modelData.pointer = g_AssetBase + 0x28;
    RegisterModelBank(GetModelBankHeader(g_AssetBase + 0x28), 0);
}
