#include "game/asset.h"
#include "game/car.h"

enum { CAR_MODEL_LOAD_ASSET = 1 };

static void RequestPendingCarModel(AssetRequestType request, s32 carIndex) {
    if (g_AssetLoadState != 0) {
        return;
    }

    g_AssetRequestType = request;
    g_PendingCarModelIndex = carIndex;
    g_AssetLoadState = CAR_MODEL_LOAD_ASSET;
}

void InstallCarModelAsset(CarModelAsset *asset, s32 slot, s32 carIndex) {
    SetCarModelSlot(asset, slot);
    asset = g_CarModelSlots[slot];
    RegisterModelBank(asset->modelData.modelBank, slot);
    SetCarImageSlot(asset->imageData.carImage, slot);
    if (carIndex < 10) {
        ApplyBodyColor1(g_CarTable[carIndex].paintColor1,
                        asset->imageData.carImage);
        ApplyBodyColor2(g_CarTable[carIndex].paintColor2,
                        asset->imageData.carImage);
    }
}

void RequestCarModel(s32 carIndex) {
    RequestPendingCarModel(ASSET_REQUEST_CAR_MODEL, carIndex);
}

void RequestUpgradedCarModel(s32 carIndex) {
    RequestPendingCarModel(ASSET_REQUEST_UPGRADED_CAR_MODEL, carIndex);
}

static void LoadCarModelVariant(s32 carIndex, s32 gradeOffset) {
    s32 targetSlot;
    s32 variantIndex;
    u8 *destination;
    CarModelAsset *asset;
    s32 assetId;

    if (g_AssetLoadState != CAR_MODEL_LOAD_ASSET) {
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

void LoadCarModel(s32 carIndex) {
    LoadCarModelVariant(carIndex, 0);
}

void LoadUpgradedCarModel(s32 carIndex) {
    LoadCarModelVariant(carIndex, 1);
}
