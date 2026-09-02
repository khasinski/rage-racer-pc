#include "game/asset.h"
#include "game/car.h"

enum { CAR_MODEL_LOAD_ASSET = 1 };

static void RequestPendingCarModel(AssetRequestType request, s32 carIndex) {
    if ((u32)carIndex >= GAME_CAR_COUNT || g_AssetLoadState != 0) {
        return;
    }

    g_AssetRequestType = request;
    g_PendingCarModelIndex = carIndex;
    g_AssetLoadState = CAR_MODEL_LOAD_ASSET;
}

void InstallCarModelAsset(CarModelAsset *asset, s32 slot, s32 carIndex) {
    if ((u32)slot >= CAR_ASSET_SLOT_COUNT) {
        return;
    }

    if (!InstallSerializedCarModelSlot(asset, slot)) {
        return;
    }
    asset = g_CarModelSlots[slot];
    if (!RegisterModelBank(asset->modelData.modelBank,
                           (size_t)asset->serializedModelSize, slot)) {
        return;
    }
    g_CarImageSlots[slot] = asset->imageData.carImage;
    if ((u32)carIndex < CUSTOM_PAINT_CAR_COUNT) {
        ApplyPrimaryBodyColor(g_CarTable[carIndex].paintColor1,
                              asset->imageData.carImage);
        ApplySecondaryBodyColor(g_CarTable[carIndex].paintColor2,
                                asset->imageData.carImage);
    }
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

    if (g_AssetLoadState != CAR_MODEL_LOAD_ASSET) {
        return;
    }
    if ((u32)carIndex >= GAME_CAR_COUNT) {
        g_AssetLoadState = 0;
        return;
    }

    targetSlot = g_CarModelSlot == 0 ? 1 : 0;
    destination = g_CarModelBuffer + targetSlot * CAR_MODEL_SLOT_SIZE;
    variantIndex = GetCarAssetIndex(
        carIndex, g_CarTable[carIndex].modelVariant + gradeOffset);
    assetId = CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variantIndex);
    if (LoadAsset(assetId, destination) == 0) {
        return;
    }

    asset = GetCarModelAsset(destination);
    InstallCarModelAsset(asset, targetSlot, carIndex);
    g_AssetLoadState = 0;
}
