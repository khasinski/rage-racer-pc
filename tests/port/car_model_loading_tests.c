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
const TeamLogoSample *g_TeamLogoSampleData;

static s32 s_loadResult;
static s32 s_loadAssetId;
static void *s_loadDestination;
static s32 s_registeredSlot;
static const ModelBankHeader *s_registeredBank;
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
static const CourseModelAssetHeader *s_courseModels;
static const GameImageAssetHeaderWord *s_uploadedImage;
static s32 s_installCarModelSlotCalls;
static s32 s_serializedModelValid = 1;
static s32 s_registerModelBankResult = 1;
static s32 s_forceInvalidAssetIndex;
static size_t s_validatedModelSize;
static size_t s_assetRoom = SIZE_MAX;

size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return s_assetRoom;
}

void ResetCdAudioState(void) {}

s32 GetCarAssetIndex(s32 model, s32 grade) {
    return s_forceInvalidAssetIndex ? -1 : model * 10 + grade;
}
s32 LoadAsset(s32 assetId, void *destination) {
    s_loadAssetId = assetId;
    s_loadDestination = destination;
    return s_loadResult;
}
s32 InstallSerializedCarModelSlot(CarModelAsset *asset, size_t size,
                                  s32 slot) {
    (void)size;
    s_installCarModelSlotCalls++;
    g_CarModelSlots[slot] = asset;
    return s_startAudioResult;
}
s32 IsValidSerializedCarModelAsset(const CarModelAsset *asset, size_t size) {
    (void)asset;
    s_validatedModelSize = size;
    return s_serializedModelValid;
}
s32 RegisterModelBank(const ModelBankHeader *bank, size_t size, s32 slot) {
    (void)size;
    s_registeredBank = bank;
    s_registeredSlot = slot;
    return s_registerModelBankResult;
}
s32 IsValidModelBankAsset(const ModelBankHeader *bank, size_t size) {
    (void)bank;
    (void)size;
    return s_registerModelBankResult;
}
s32 IsValidCourseModelAsset(const CourseModelAssetHeader *models,
                            size_t size) {
    (void)models;
    (void)size;
    return 1;
}
s32 IsValidImageAsset(const GameImageAssetHeaderWord *image, size_t size) {
    (void)image;
    (void)size;
    return 1;
}
void SelectCarModelSlot(s32 slot) { g_CarModelAsset = g_CarModelSlots[slot]; }
s32 RegisterCourseModels(const CourseModelAssetHeader *models, size_t size) {
    (void)size;
    s_courseModels = models;
    return 1;
}
s32 UploadImageAsset(const GameImageAssetHeaderWord *image, size_t size) {
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
    g_AssetLoadFailed = 1;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestCarModel(4), "car model request is accepted");
    Check(g_AssetRequestType == ASSET_REQUEST_CAR_MODEL &&
              g_PendingCarModelIndex == 4 && g_AssetLoadState == 1 &&
              !AssetLoadHasFailed(),
          "car model request");

    Check(!RequestUpgradedCarModel(7), "busy model request is rejected");
    Check(g_AssetRequestType == ASSET_REQUEST_CAR_MODEL &&
              g_PendingCarModelIndex == 4,
          "busy loader preserves pending car request");

    g_AssetLoadState = 0;
    Check(RequestUpgradedCarModel(7), "upgraded model request is accepted");
    Check(g_AssetRequestType == ASSET_REQUEST_UPGRADED_CAR_MODEL &&
              g_PendingCarModelIndex == 7 && g_AssetLoadState == 1,
          "upgraded car model request");

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(!RequestCarModel(-1), "negative model request reports rejection");
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE &&
              g_AssetLoadState == 0,
          "negative car model request is rejected");
    Check(!RequestUpgradedCarModel(GAME_CAR_COUNT),
          "out-of-range upgraded model request reports rejection");
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

    g_AssetLoadState = 1;
    g_PendingCarModelIndex = 2;
    g_CarTable = NULL;
    s_loadAssetId = -123;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "missing car table is rejected before asset lookup");

    g_CarTable = cars;
    g_CarModelBuffer = NULL;
    g_AssetLoadState = 1;
    g_AssetLoadFailed = 0;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "missing car model buffer is rejected before pointer arithmetic");

    g_CarModelBuffer = buffers;
    g_CarModelSlot = CAR_ASSET_SLOT_COUNT;
    g_AssetLoadState = 1;
    g_AssetLoadFailed = 0;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "invalid active model slot is rejected before asset lookup");

    g_CarModelSlot = 0;
    g_AssetLoadState = 1;
    g_AssetLoadFailed = 0;
    s_forceInvalidAssetIndex = 1;
    LoadPendingCarModelAsset();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "invalid variant index is rejected before asset lookup");
    s_forceInvalidAssetIndex = 0;

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

    g_CarTable = NULL;
    Check(InstallCarModelAsset(&model, sizeof(model), 0, 0) == 1,
          "model installation does not require save data");
    Check(s_color1Calls == 0 && s_color2Calls == 0,
          "missing save data skips custom paint");
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
    enum {
        TEAM_LOGO_SAMPLES_OFFSET = 64,
        COURSE_MODELS_OFFSET =
            TEAM_LOGO_SAMPLES_OFFSET +
            TEAM_LOGO_SAMPLE_RECORD_COUNT * sizeof(TeamLogoSample),
        IMAGE_OFFSET = COURSE_MODELS_OFFSET + 64,
        SHARED_ASSET_SIZE = IMAGE_OFFSET + 320,
    };
    static u8 storage[CAR_MODEL_BUFFER_SIZE + SHARED_ASSET_SIZE];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)storage;
    CarEntry cars[2];
    CarModelAsset *model;
    CarImageData carImage;
    u8 originalSharedAssets[512];

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
    pack->offsets[0] = TEAM_LOGO_SAMPLES_OFFSET;
    pack->offsets[1] = COURSE_MODELS_OFFSET;
    pack->offsets[2] = IMAGE_OFFSET;
    s_loadResult = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 3 && s_loadAssetId == 8,
          "pending shared car assets hold phase");
    s_loadResult = SHARED_ASSET_SIZE;
    g_AssetLoadState = 3;
    s_loadResult = 3 * (s32)sizeof(s32) - 1;
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL,
          "truncated showroom header cancels installation");

    g_AssetLoadState = 3;
    s_loadResult = SHARED_ASSET_SIZE;
    pack->offsets[1] = COURSE_MODELS_OFFSET - 1;
    s_registeredBank = NULL;
    s_courseModels = NULL;
    s_uploadedImage = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL &&
              s_courseModels == NULL && s_uploadedImage == NULL,
          "truncated team-logo samples cancel installation");

    g_AssetLoadState = 3;
    pack->offsets[1] = COURSE_MODELS_OFFSET;
    pack->offsets[2] = pack->offsets[1];
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && s_registeredBank == NULL,
          "overlapping showroom blocks cancel installation");
    g_AssetLoadState = 3;
    pack->offsets[2] = IMAGE_OFFSET;
    s_assetRoom = CAR_MODEL_BUFFER_SIZE - 1;
    s_registeredBank = NULL;
    s_courseModels = NULL;
    s_uploadedImage = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_registeredBank == NULL && s_courseModels == NULL &&
              s_uploadedImage == NULL,
          "undersized showroom arena publishes no shared assets");

    g_AssetLoadState = 3;
    s_assetRoom = SIZE_MAX;
    memcpy(originalSharedAssets, storage, sizeof(originalSharedAssets));
    LoadCarSelectAssets();
    Check(memcmp(originalSharedAssets, storage,
                 sizeof(originalSharedAssets)) == 0,
          "shared showroom assets remain unchanged during installation");
    Check(s_registeredBank ==
              (ModelBankHeader *)(void *)(storage + 0xC) &&
              s_registeredSlot == 14,
          "showroom scene model bank follows its three offsets");
    Check(g_TeamLogoSampleData ==
              GetTeamLogoSample(storage + TEAM_LOGO_SAMPLES_OFFSET),
          "team logo samples installed");
    Check(s_courseModels == (CourseModelAssetHeader *)(void *)(
                                storage + COURSE_MODELS_OFFSET),
          "showroom course models installed");
    Check(s_uploadedImage == (GameImageAssetHeaderWord *)(void *)(
                                 storage + IMAGE_OFFSET),
          "showroom image uploaded");
    Check(g_CarModelBuffer == storage + IMAGE_OFFSET &&
              g_ImageBlockBuffer == storage + IMAGE_OFFSET +
                                        CAR_MODEL_BUFFER_SIZE &&
              g_AssetLoadState == 4,
          "shared assets publish car buffers");

    model = (CarModelAsset *)(void *)(storage + IMAGE_OFFSET);
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
    g_PlayerCarIndex = GAME_CAR_COUNT;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed(),
          "out-of-range showroom car is rejected before table access");

    g_AssetLoadState = 4;
    g_AssetLoadFailed = 0;
    g_PlayerCarIndex = 1;
    g_CarTable = NULL;
    s_loadAssetId = -123;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "missing showroom car table is rejected before asset lookup");

    g_CarTable = cars;
    g_CarModelBuffer = NULL;
    g_AssetLoadState = 4;
    g_AssetLoadFailed = 0;
    s_loadAssetId = -123;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "missing showroom model buffer is rejected before asset lookup");

    g_CarModelBuffer = storage + IMAGE_OFFSET;
    g_AssetLoadState = 4;
    g_AssetLoadFailed = 0;
    s_forceInvalidAssetIndex = 1;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetId == -123,
          "invalid showroom variant is rejected before asset lookup");
    s_forceInvalidAssetIndex = 0;

    g_AssetLoadState = 4;
    g_PlayerCarIndex = 1;
    g_CarModelSlot = 1;
    g_CarModelAsset = NULL;
    s_serializedModelValid = 0;
    s_registeredBank = NULL;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && g_CarModelSlot == 1 &&
              g_CarModelAsset == NULL && s_registeredBank == NULL,
          "invalid initial showroom model is not selected");
    s_serializedModelValid = 1;

    g_AssetLoadState = 99;
    g_AssetLoadFailed = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed(),
          "unknown car-select load phase is rejected");

    g_AssetLoadFailed = 0;
    LoadCarSelectAssets();
    Check(g_AssetLoadState == 0 && !AssetLoadHasFailed(),
          "idle car-select loader is a no-op");
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
