#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"

enum { CAR_MODEL_LOAD_ASSET = 1 };

static void RequestPendingCarModel(AssetRequestType request, s32 carIndex) {
    if ((u32)carIndex >= GAME_CAR_COUNT || g_AssetLoadState != 0) {
        return;
    }

    g_AssetRequestType = request;
    g_AssetLoadFailed = 0;
    g_PendingCarModelIndex = carIndex;
    g_AssetLoadState = CAR_MODEL_LOAD_ASSET;
}

s32 InstallCarModelAsset(CarModelAsset *asset, size_t size, s32 slot,
                         s32 carIndex) {
    ModelBankHeader *modelBank;

    if ((u32)slot >= CAR_ASSET_SLOT_COUNT ||
        !IsValidSerializedCarModelAsset(asset, size)) {
        return 0;
    }

    modelBank = GetModelBankHeader(
        GetAssetBytes(asset) + SERIALIZED_CAR_MODEL_HEADER_SIZE);
    if (!RegisterModelBank(modelBank,
                           (size_t)asset->serializedModelSize, slot)) {
        return 0;
    }
    if (!InstallSerializedCarModelSlot(asset, slot)) {
        return 0;
    }
    asset = g_CarModelSlots[slot];
    g_CarImageSlots[slot] = asset->imageData.carImage;
    if ((u32)carIndex < CUSTOM_PAINT_CAR_COUNT) {
        ApplyPrimaryBodyColor(g_CarTable[carIndex].paintColor1,
                              asset->imageData.carImage);
        ApplySecondaryBodyColor(g_CarTable[carIndex].paintColor2,
                                asset->imageData.carImage);
    }
    return 1;
}

void RequestCarModel(s32 carIndex) {
    RequestPendingCarModel(ASSET_REQUEST_CAR_MODEL, carIndex);
}

void RequestUpgradedCarModel(s32 carIndex) {
    RequestPendingCarModel(ASSET_REQUEST_UPGRADED_CAR_MODEL, carIndex);
}

void LoadPendingCarModelAsset(void) {
    s32 carIndex = g_PendingCarModelIndex;
    s32 gradeOffset =
        g_AssetRequestType == ASSET_REQUEST_UPGRADED_CAR_MODEL ? 1 : 0;
    s32 targetSlot;
    s32 variantIndex;
    u8 *destination;
    CarModelAsset *asset;
    s32 assetId;
    s32 loadedSize;

    if (g_AssetLoadState != CAR_MODEL_LOAD_ASSET) {
        return;
    }
    if ((u32)carIndex >= GAME_CAR_COUNT) {
        FailAssetLoad();
        return;
    }

    targetSlot = g_CarModelSlot == 0 ? 1 : 0;
    destination = g_CarModelBuffer + targetSlot * CAR_MODEL_SLOT_SIZE;
    variantIndex = GetCarAssetIndex(
        carIndex, g_CarTable[carIndex].modelVariant + gradeOffset);
    assetId = CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variantIndex);
    loadedSize = LoadAsset(assetId, destination);
    if (loadedSize == 0) return;

    asset = GetCarModelAsset(destination);
    if (!InstallCarModelAsset(asset, (size_t)loadedSize, targetSlot,
                              carIndex)) {
        FailAssetLoad();
        return;
    }
    g_AssetLoadState = 0;
}
