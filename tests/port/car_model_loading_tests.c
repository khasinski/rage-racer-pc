#include "common.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio.h"
#include "game/car.h"

#include <stdio.h>
#include <string.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
s32 g_AssetLoadFailed;
s32 g_PendingCarModelIndex;
u8 *g_AssetBlockPtr;
size_t g_AssetBlockSize;
u8 *g_AssetBlockPtr2;
size_t g_AssetBlock2Size;
u8 *g_AssetLoadCursor;
u8 *g_AssetSubBlockPtr;
size_t g_AssetSubBlockSize;
u8 *g_ImageBlockBuffer;
size_t g_ImageBlockSize;
u8 *g_CarModelBuffer;
u32 g_CarModelSlot;
s32 g_PlayerCarIndex;
CarEntry *g_CarTable;
CarModelAsset *g_CarModelSlots[CAR_ASSET_SLOT_COUNT];
CarImageData *g_CarImageSlots[CAR_ASSET_SLOT_COUNT];
CarModelAsset *g_CarModelAsset;
TeamLogoSample *g_TeamLogoSampleData;

static s32 s_loadResult;
static s32 s_loadAssetId;
static void *s_loadDestination;
static s32 s_registeredSlot;
static ModelBankHeader *s_registeredBank;
static s32 s_color1Calls;
static s32 s_color2Calls;
static u32 s_color1;
static u32 s_color2;
static s32 s_failures;
static s32 s_pollResult;
static s32 s_audioSlot;
static u8 *s_audioHeader;
static u8 *s_audioBody;
static u16 *s_audioTable;
static size_t s_audioHeaderSize;
static size_t s_audioBodySize;
static size_t s_audioAuxiliarySize;
static s32 s_startAudioResult = 1;
static s32 s_sequenceInitCalls;
static CourseModelAssetHeader *s_courseModels;
static GameImageAssetHeaderWord *s_uploadedImage;
static s32 s_installCarModelSlotCalls;
static s32 s_serializedModelValid = 1;
static s32 s_registerModelBankResult = 1;
static size_t s_validatedModelSize;

void ResetCdAudioState(void) {}

s32 GetCarAssetIndex(s32 model, s32 grade) { return model * 10 + grade; }
s32 LoadAsset(s32 assetId, void *destination) {
    s_loadAssetId = assetId;
    s_loadDestination = destination;
    return s_loadResult;
}
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, s32 slot) {
    s_installCarModelSlotCalls++;
    g_CarModelSlots[slot] = asset;
    return s_startAudioResult;
}
s32 IsValidSerializedCarModelAsset(const CarModelAsset *asset, size_t size) {
    (void)asset;
    s_validatedModelSize = size;
    return s_serializedModelValid;
}
s32 RegisterModelBank(ModelBankHeader *bank, size_t size, s32 slot) {
    (void)size;
    s_registeredBank = bank;
    s_registeredSlot = slot;
    return s_registerModelBankResult;
}
void SelectCarModelSlot(s32 slot) { g_CarModelAsset = g_CarModelSlots[slot]; }
s32 RegisterCourseModels(CourseModelAssetHeader *models, size_t size) {
    (void)size;
    s_courseModels = models;
    return 1;
}
s32 UploadImageAsset(GameImageAssetHeaderWord *image, size_t size) {
    (void)size;
    s_uploadedImage = image;
    return 1;
}
s32 StartAudioSlotLoad(s32 slot, const AudioSlotAsset *asset) {
    s_audioSlot = slot;
    s_audioHeader = asset->vabHeader;
    s_audioHeaderSize = asset->vabHeaderSize;
    s_audioBody = asset->vabBody;
    s_audioBodySize = asset->vabBodySize;
    s_audioTable = asset->auxiliaryData;
    s_audioAuxiliarySize = asset->auxiliarySize;
    return s_startAudioResult;
}
s32 PollAudioSlotLoad(void) { return s_pollResult; }
void InitSequenceAudio(void) { s_sequenceInitCalls++; }
void ApplyPrimaryBodyColor(u32 color, CarImageData *image) {
    (void)image;
    s_color1 = color;
    s_color1Calls++;
}
void ApplySecondaryBodyColor(u32 color, CarImageData *image) {
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

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    RequestCarModel(-1);
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE &&
              g_AssetLoadState == 0,
          "negative car model request is rejected");
    RequestUpgradedCarModel(GAME_CAR_COUNT);
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE &&
              g_AssetLoadState == 0,
          "out-of-range upgraded model request is rejected");
}

static void TestModelVariantLoads(void) {
    static u8 buffers[CAR_MODEL_BUFFER_SIZE];
    CarEntry cars[11];
    CarModelAsset *upper =
        (CarModelAsset *)(void *)(buffers + CAR_MODEL_SLOT_SIZE);
    CarModelAsset *lower = (CarModelAsset *)(void *)buffers;
    CarImageData upperImage;
    CarImageData lowerImage;

    memset(cars, 0, sizeof(cars));
    memset(buffers, 0, sizeof(buffers));
    cars[2].modelVariant = 3;
    cars[2].paintColor1 = 4;
    cars[2].paintColor2 = 5;
    cars[10].modelVariant = 1;
    upper->imageData.carImage = &upperImage;
    lower->imageData.carImage = &lowerImage;
    g_CarTable = cars;
    g_CarModelBuffer = buffers;

    g_CarModelSlot = 0;
    g_AssetRequestType = ASSET_REQUEST_CAR_MODEL;
    g_PendingCarModelIndex = 2;
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadPendingCarModelAsset();
    Check(s_loadAssetId == 0xA + (23 << 1) &&
              s_loadDestination == buffers + CAR_MODEL_SLOT_SIZE,
          "normal model asset and inactive slot");
    Check(g_AssetLoadState == 1, "pending normal model holds loader");

    s_loadResult = 1;
    s_color1Calls = 0;
    s_color2Calls = 0;
    LoadPendingCarModelAsset();
    Check(g_CarModelSlots[1] == upper &&
              s_registeredBank == GetModelBankHeader(
                                      (u8 *)upper +
                                      SERIALIZED_CAR_MODEL_HEADER_SIZE) &&
              s_registeredSlot == 1,
          "normal model installs inactive model slot");
    Check(s_validatedModelSize == (size_t)s_loadResult,
          "normal model validates exactly the loaded bytes");
    Check(g_CarImageSlots[1] == &upperImage,
          "normal model installs inactive image slot");
    Check(s_color1Calls == 1 && s_color2Calls == 1 &&
              s_color1 == 4 && s_color2 == 5,
          "normal model applies player paint");
    Check(g_AssetLoadState == 0, "normal model completes loader");

    g_CarModelSlot = 1;
    g_AssetRequestType = ASSET_REQUEST_UPGRADED_CAR_MODEL;
    g_PendingCarModelIndex = 10;
    g_AssetLoadState = 1;
    s_color1Calls = 0;
    s_color2Calls = 0;
    LoadPendingCarModelAsset();
    Check(s_loadAssetId == 0xA + (102 << 1) &&
              s_loadDestination == buffers,
          "upgraded model asset and inactive slot");
    Check(g_CarModelSlots[0] == lower &&
              s_registeredBank == GetModelBankHeader(
                                      (u8 *)lower +
                                      SERIALIZED_CAR_MODEL_HEADER_SIZE) &&
              s_registeredSlot == 0 && g_CarImageSlots[0] == &lowerImage,
          "upgraded model installs inactive slots");
    Check(s_color1Calls == 0 && s_color2Calls == 0,
          "non-player car entry skips custom paint");
    Check(g_AssetLoadState == 0, "upgraded model completes loader");

    g_AssetLoadState = 1;
    g_AssetRequestType = ASSET_REQUEST_CAR_MODEL;
    g_PendingCarModelIndex = -1;
    s_loadAssetId = -123;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && s_loadAssetId == -123,
          "invalid pending model is cancelled before asset lookup");

    g_CarModelSlot = 0;
    g_PendingCarModelIndex = 2;
    g_AssetLoadState = 1;
    s_loadResult = 1;
    s_serializedModelValid = 0;
    s_registeredBank = NULL;
    g_CarModelSlots[1] = upper;
    g_CarImageSlots[1] = &upperImage;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL &&
              g_CarModelSlots[1] == upper &&
              g_CarImageSlots[1] == &upperImage,
          "invalid replacement model preserves the inactive slot");
    s_serializedModelValid = 1;
}

static void TestInvalidCarSkipsCustomPaint(void) {
    CarModelAsset model;
    ModelBankHeader bank;
    CarImageData image;

    memset(&model, 0, sizeof(model));
    model.modelData.modelBank = &bank;
    model.imageData.carImage = &image;
    s_color1Calls = 0;
    s_color2Calls = 0;

    InstallCarModelAsset(&model, sizeof(model), 0, -1);

    Check(s_color1Calls == 0 && s_color2Calls == 0,
          "invalid car index skips custom paint");
}

static void TestInvalidSlotSkipsInstallation(void) {
    CarModelAsset model;

    memset(&model, 0, sizeof(model));
    s_registeredBank = NULL;
    g_CarImageSlots[0] = NULL;
    s_color1Calls = 0;
    s_color2Calls = 0;

    InstallCarModelAsset(&model, sizeof(model), CAR_ASSET_SLOT_COUNT, 0);

    Check(s_registeredBank == NULL && g_CarImageSlots[0] == NULL &&
              s_color1Calls == 0 && s_color2Calls == 0,
          "invalid model slot skips installation");
}

static void TestInvalidSerializedModelSkipsInstallation(void) {
    CarModelAsset model;

    memset(&model, 0, sizeof(model));
    s_serializedModelValid = 0;
    s_installCarModelSlotCalls = 0;
    s_registeredBank = NULL;
    g_CarImageSlots[0] = NULL;
    s_color1Calls = 0;
    s_color2Calls = 0;

    InstallCarModelAsset(&model, sizeof(model), 0, 0);

    Check(s_registeredBank == NULL && s_installCarModelSlotCalls == 0 &&
              g_CarImageSlots[0] == NULL &&
              s_color1Calls == 0 && s_color2Calls == 0,
          "invalid serialized model skips dependent installation");
    s_serializedModelValid = 1;
}

static void TestInvalidModelBankPreservesSlot(void) {
    CarModelAsset model;
    CarModelAsset previousModel;
    CarImageData previousImage;

    memset(&model, 0, sizeof(model));
    g_CarModelSlots[0] = &previousModel;
    g_CarImageSlots[0] = &previousImage;
    s_installCarModelSlotCalls = 0;
    s_registerModelBankResult = 0;

    Check(InstallCarModelAsset(&model, sizeof(model), 0, 0) == 0,
          "invalid model bank rejects car asset");
    Check(s_installCarModelSlotCalls == 0 &&
              g_CarModelSlots[0] == &previousModel &&
              g_CarImageSlots[0] == &previousImage,
          "invalid model bank preserves the published slot");

    s_registerModelBankResult = 1;
}

static void TestCarSelectAssetPhases(void) {
    static u8 storage[CAR_MODEL_BUFFER_SIZE + 512];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)storage;
    CarEntry cars[2];
    CarModelAsset *model;
    CarImageData carImage;

    memset(storage, 0, sizeof(storage));
    memset(cars, 0, sizeof(cars));
    g_CarTable = cars;

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestCarSelectAssets() == 1, "new car select request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_CAR_SELECT &&
              g_AssetLoadState == 1,
          "car select request initializes loader");
    Check(RequestCarSelectAssets() == 1, "busy car select remains pending");
    g_AssetLoadState = 0;
    Check(RequestCarSelectAssets() == 0, "car select request acknowledged");

    g_AssetLoadState = 1;
    g_AssetBlockPtr = storage + 16;
    g_AssetBlockSize = 16;
    g_AssetBlockPtr2 = storage + 32;
    g_AssetBlock2Size = 16;
    g_AssetSubBlockPtr = storage + 48;
    g_AssetSubBlockSize = 32;
    s_startAudioResult = -1;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0,
          "failed sequence audio transfer cancels car-select loading");

    g_AssetLoadState = 1;
    s_startAudioResult = 1;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 2 && s_audioSlot == 1 &&
              s_audioHeader == storage + 16 && s_audioBody == storage + 48 &&
              s_audioTable == (u16 *)(void *)(storage + 32) &&
              s_audioHeaderSize == 16 && s_audioBodySize == 32 &&
              s_audioAuxiliarySize == 16,
          "car select audio phase");

    s_pollResult = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 2, "pending car select audio holds phase");
    s_pollResult = 1;
    s_sequenceInitCalls = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 3 && g_AssetLoadCursor == storage + 48 &&
              s_sequenceInitCalls == 1,
          "car select audio completion");

    g_AssetLoadCursor = storage;
    pack->offsets[0] = 64;
    pack->offsets[1] = 128;
    pack->offsets[2] = 192;
    s_loadResult = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 3 && s_loadAssetId == 8,
          "pending shared car assets hold phase");
    s_loadResult = 512;
    g_AssetLoadState = 3;
    s_loadResult = 3 * (s32)sizeof(s32) - 1;
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL,
          "truncated showroom header cancels installation");

    g_AssetLoadState = 3;
    s_loadResult = 512;
    pack->offsets[2] = pack->offsets[1];
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL,
          "overlapping showroom blocks cancel installation");
    g_AssetLoadState = 3;
    pack->offsets[2] = 192;
    LoadCarSelectAssets();
    Check(s_registeredBank ==
              (ModelBankHeader *)(void *)(storage + 0xC) &&
              s_registeredSlot == 14,
          "showroom scene model bank follows its three offsets");
    Check(g_TeamLogoSampleData == GetTeamLogoSample(storage + 64),
          "team logo samples installed");
    Check(s_courseModels ==
              (CourseModelAssetHeader *)(void *)(storage + 128),
          "showroom course models installed");
    Check(s_uploadedImage ==
              (GameImageAssetHeaderWord *)(void *)(storage + 192),
          "showroom image uploaded");
    Check(g_CarModelBuffer == storage + 192 &&
              g_ImageBlockBuffer == storage + 192 + CAR_MODEL_BUFFER_SIZE &&
              g_AssetLoadState == 4,
          "shared assets publish car buffers");

    model = (CarModelAsset *)(void *)(storage + 192);
    model->imageData.carImage = &carImage;
    cars[1].modelVariant = 2;
    cars[1].paintColor1 = 6;
    cars[1].paintColor2 = 7;
    g_PlayerCarIndex = 1;
    s_color1Calls = 0;
    s_color2Calls = 0;
    LoadCarSelectAssets();
    Check(s_loadAssetId == 0xA + (12 << 1),
          "initial showroom car asset index");
    Check(g_CarModelAsset == model && g_CarModelSlot == 0,
          "initial showroom model selected");
    Check(s_registeredBank == GetModelBankHeader(
                                  (u8 *)model +
                                  SERIALIZED_CAR_MODEL_HEADER_SIZE) &&
              s_registeredSlot == 0 &&
              g_CarImageSlots[0] == &carImage,
          "initial showroom model installed");
    Check(s_validatedModelSize == (size_t)s_loadResult,
          "initial showroom model validates exactly the loaded bytes");
    Check(s_color1Calls == 1 && s_color2Calls == 1 &&
              s_color1 == 6 && s_color2 == 7,
          "initial showroom paint applied");
    Check(g_AssetLoadState == 0, "car select asset load completes");

    g_AssetLoadState = 4;
    g_CarModelSlot = 1;
    g_CarModelAsset = NULL;
    s_serializedModelValid = 0;
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && g_CarModelSlot == 1 &&
              g_CarModelAsset == NULL && s_registeredBank == NULL,
          "invalid initial showroom model is not selected");
    s_serializedModelValid = 1;
}

int main(void) {
    TestRequests();
    TestModelVariantLoads();
    TestInvalidCarSkipsCustomPaint();
    TestInvalidSlotSkipsInstallation();
    TestInvalidSerializedModelSkipsInstallation();
    TestInvalidModelBankPreservesSlot();
    TestCarSelectAssetPhases();

    if (s_failures != 0) return 1;
    puts("car model requests load the selected grade into the inactive slot");
    return 0;
}
