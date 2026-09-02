#include "common.h"
#include "game/asset.h"
#include "game/car.h"

#include <stdio.h>
#include <string.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
s32 g_PendingCarModelIndex;
u8 *g_CarModelBuffer;
u32 g_CarModelSlot;
CarEntry *g_CarTable;
CarModelAsset *g_CarModelSlots[2];

static s32 s_loadResult;
static s32 s_loadAssetId;
static void *s_loadDestination;
static s32 s_registeredSlot;
static ModelBankHeader *s_registeredBank;
static s32 s_imageSlot;
static CarImageData *s_image;
static s32 s_color1Calls;
static s32 s_color2Calls;
static u32 s_color1;
static u32 s_color2;
static s32 s_failures;

s32 GetCarAssetIndex(s32 model, s32 grade) { return model * 10 + grade; }
s32 LoadAsset(s32 assetId, void *destination) {
    s_loadAssetId = assetId;
    s_loadDestination = destination;
    return s_loadResult;
}
void SetCarModelSlot(CarModelAsset *asset, s32 slot) {
    g_CarModelSlots[slot] = asset;
}
void RegisterModelBank(ModelBankHeader *bank, s32 slot) {
    s_registeredBank = bank;
    s_registeredSlot = slot;
}
void SetCarImageSlot(CarImageData *image, s32 slot) {
    s_image = image;
    s_imageSlot = slot;
}
void ApplyBodyColor1(u32 color, CarImageData *image) {
    (void)image;
    s_color1 = color;
    s_color1Calls++;
}
void ApplyBodyColor2(u32 color, CarImageData *image) {
    (void)image;
    s_color2 = color;
    s_color2Calls++;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestRequests(void) {
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    RequestCarModel(4);
    Check(g_AssetRequestType == ASSET_REQUEST_CAR_MODEL &&
              g_PendingCarModelIndex == 4 && g_AssetLoadState == 1,
          "car model request");

    RequestUpgradedCarModel(7);
    Check(g_AssetRequestType == ASSET_REQUEST_CAR_MODEL &&
              g_PendingCarModelIndex == 4,
          "busy loader preserves pending car request");

    g_AssetLoadState = 0;
    RequestUpgradedCarModel(7);
    Check(g_AssetRequestType == ASSET_REQUEST_UPGRADED_CAR_MODEL &&
              g_PendingCarModelIndex == 7 && g_AssetLoadState == 1,
          "upgraded car model request");
}

static void TestModelVariantLoads(void) {
    static u8 buffers[CAR_MODEL_BUFFER_SIZE];
    CarEntry cars[11];
    CarModelAsset *upper =
        (CarModelAsset *)(void *)(buffers + CAR_MODEL_SLOT_SIZE);
    CarModelAsset *lower = (CarModelAsset *)(void *)buffers;
    ModelBankHeader upperBank;
    ModelBankHeader lowerBank;
    CarImageData upperImage;
    CarImageData lowerImage;

    memset(cars, 0, sizeof(cars));
    memset(buffers, 0, sizeof(buffers));
    cars[2].modelVariant = 3;
    cars[2].paintColor1 = 4;
    cars[2].paintColor2 = 5;
    cars[10].modelVariant = 1;
    upper->modelData.modelBank = &upperBank;
    upper->imageData.carImage = &upperImage;
    lower->modelData.modelBank = &lowerBank;
    lower->imageData.carImage = &lowerImage;
    g_CarTable = cars;
    g_CarModelBuffer = buffers;

    g_CarModelSlot = 0;
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadCarModel(2);
    Check(s_loadAssetId == 0xA + (23 << 1) &&
              s_loadDestination == buffers + CAR_MODEL_SLOT_SIZE,
          "normal model asset and inactive slot");
    Check(g_AssetLoadState == 1, "pending normal model holds loader");

    s_loadResult = 1;
    s_color1Calls = 0;
    s_color2Calls = 0;
    LoadCarModel(2);
    Check(g_CarModelSlots[1] == upper && s_registeredBank == &upperBank &&
              s_registeredSlot == 1,
          "normal model installs inactive model slot");
    Check(s_image == &upperImage && s_imageSlot == 1,
          "normal model installs inactive image slot");
    Check(s_color1Calls == 1 && s_color2Calls == 1 &&
              s_color1 == 4 && s_color2 == 5,
          "normal model applies player paint");
    Check(g_AssetLoadState == 0, "normal model completes loader");

    g_CarModelSlot = 1;
    g_AssetLoadState = 1;
    s_color1Calls = 0;
    s_color2Calls = 0;
    LoadUpgradedCarModel(10);
    Check(s_loadAssetId == 0xA + (102 << 1) &&
              s_loadDestination == buffers,
          "upgraded model asset and inactive slot");
    Check(g_CarModelSlots[0] == lower && s_registeredBank == &lowerBank &&
              s_registeredSlot == 0 && s_image == &lowerImage &&
              s_imageSlot == 0,
          "upgraded model installs inactive slots");
    Check(s_color1Calls == 0 && s_color2Calls == 0,
          "non-player car entry skips custom paint");
    Check(g_AssetLoadState == 0, "upgraded model completes loader");
}

int main(void) {
    TestRequests();
    TestModelVariantLoads();

    if (s_failures != 0) return 1;
    puts("car model requests load the selected grade into the inactive slot");
    return 0;
}
