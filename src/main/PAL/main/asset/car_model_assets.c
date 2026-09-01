#include "game/asset.h"
#include "game/car.h"
#include "game/render.h"

void LoadUpgradedCarModel(s32 carIndex) {
    u8 *destination;
    CarModelAsset *asset;
    s32 targetSlot;
    s32 assetId;

    if (g_AssetLoadState != 1) {
        return;
    }

    targetSlot = g_CarModelSlot < 1;
    destination = g_CarModelBuffer;
    if (g_CarModelSlot == 0) {
        destination += CAR_MODEL_SLOT_SIZE;
    }
    assetId = 0xA +
              (GetCarAssetIndex(carIndex,
                                g_CarTable[carIndex].modelVariant + 1) << 1);
    if (LoadAsset(assetId, destination) == 0) {
        return;
    }

    SetCarModelSlot(GetCarModelAsset(destination), targetSlot);
    asset = g_CarModelSlots[targetSlot];
    RegisterModelBank(asset->modelData.modelBank, targetSlot);
    SetCarImageSlot(asset->imageData.carImage, targetSlot);
    if (g_PlayerCarIndex < 10) {
        ApplyBodyColor1(g_CarTable[carIndex].paintColor1,
                        asset->imageData.carImage);
        ApplyBodyColor2(g_CarTable[carIndex].paintColor2,
                        asset->imageData.carImage);
    }
    g_AssetLoadState = 0;
}

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
