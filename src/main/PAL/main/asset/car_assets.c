#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/audio.h"

enum {
    CAR_SELECT_BEGIN_AUDIO = 1,
    CAR_SELECT_WAIT_FOR_AUDIO,
    CAR_SELECT_LOAD_SHARED_ASSETS,
    CAR_SELECT_LOAD_INITIAL_MODEL,
    CAR_SELECT_SCENE_MODEL_BANK = 14,
};

typedef struct CarSelectAssetHeader {
    s32 teamLogoSamplesOffset;
    s32 courseModelsOffset;
    s32 imageOffset;
    ModelBankHeader sceneModelBank;
} CarSelectAssetHeader;

_Static_assert(offsetof(CarSelectAssetHeader, sceneModelBank) == 0xC,
               "car-select model bank must remain at +0xC");

s32 RequestCarSelectAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_CAR_SELECT,
                            CAR_SELECT_BEGIN_AUDIO, 0);
}

static void BeginCarSelectAudioLoad(void) {
    AudioSlotAsset asset = {
        .vabHeader = g_AssetBlockPtr,
        .vabHeaderSize = g_AssetBlockSize,
        .vabBody = g_AssetSubBlockPtr,
        .vabBodySize = g_AssetSubBlockSize,
        .auxiliaryData = g_AssetBlockPtr2,
        .auxiliarySize = g_AssetBlock2Size,
    };

    if (StartAudioSlotLoad(AUDIO_SLOT_SEQUENCE, &asset) < 0) {
        g_AssetLoadState = 0;
        return;
    }
    g_AssetLoadState = CAR_SELECT_WAIT_FOR_AUDIO;
}

static void FinishCarSelectAudioLoad(void) {
    if (PollAudioSlotLoad() == 0) {
        return;
    }

    InitSequenceAudio();
    g_AssetLoadCursor = g_AssetSubBlockPtr;
    g_AssetLoadState = CAR_SELECT_LOAD_SHARED_ASSETS;
}

static void LoadCarSelectSharedAssets(void) {
    CarSelectAssetHeader *header;
    CourseModelAssetHeader *courseModels;
    u8 *image;
    s32 loadedSize;

    loadedSize = LoadAsset(ASSET_CAR_SELECT_SCREEN, g_AssetLoadCursor);
    if (loadedSize == 0) return;

    header = (CarSelectAssetHeader *)g_AssetLoadCursor;
    if (header->teamLogoSamplesOffset <=
            (s32)offsetof(CarSelectAssetHeader, sceneModelBank) ||
        header->courseModelsOffset <= header->teamLogoSamplesOffset ||
        header->imageOffset <= header->courseModelsOffset ||
        header->imageOffset > loadedSize ||
        !RegisterModelBank(
            &header->sceneModelBank,
            (size_t)header->teamLogoSamplesOffset -
                offsetof(CarSelectAssetHeader, sceneModelBank),
            CAR_SELECT_SCENE_MODEL_BANK)) {
        g_AssetLoadState = 0;
        return;
    }
    g_TeamLogoSampleData =
        GetTeamLogoSample(ResolveAssetAddress(
            header, header->teamLogoSamplesOffset));
    courseModels = GetCourseModelAssetHeader(
        ResolveAssetAddress(header, header->courseModelsOffset));
    if (!RegisterCourseModels(
            courseModels,
            (size_t)(header->imageOffset - header->courseModelsOffset))) {
        g_AssetLoadState = 0;
        return;
    }
    image = ResolveAssetAddress(header, header->imageOffset);
    if (!UploadImageAsset(GetImageAssetHeaderWords(image),
                          (size_t)(loadedSize - header->imageOffset))) {
        g_AssetLoadState = 0;
        return;
    }
    g_CarModelBuffer = image;
    g_ImageBlockBuffer = image + CAR_MODEL_BUFFER_SIZE;
    g_ImageBlockSize = 0;
    g_AssetLoadState = CAR_SELECT_LOAD_INITIAL_MODEL;
}

static void LoadInitialCarSelectModel(void) {
    s32 carIndex = g_PlayerCarIndex;
    s32 variantIndex = GetCarAssetIndex(
        carIndex, g_CarTable[carIndex].modelVariant);
    s32 assetIndex = CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variantIndex);
    s32 loadedSize;

    loadedSize = LoadAsset(assetIndex, g_CarModelBuffer);
    if (loadedSize == 0) return;

    if (!InstallCarModelAsset(GetCarModelAsset(g_CarModelBuffer),
                              (size_t)loadedSize, 0, carIndex)) {
        g_AssetLoadState = 0;
        return;
    }
    SelectCarModelSlot(0);
    g_CarModelSlot = 0;
    g_AssetLoadState = 0;
}

void LoadCarSelectAssets(void) {
    switch (g_AssetLoadState) {
    case CAR_SELECT_BEGIN_AUDIO:
        BeginCarSelectAudioLoad();
        break;
    case CAR_SELECT_WAIT_FOR_AUDIO:
        FinishCarSelectAudioLoad();
        break;
    case CAR_SELECT_LOAD_SHARED_ASSETS:
        LoadCarSelectSharedAssets();
        break;
    case CAR_SELECT_LOAD_INITIAL_MODEL:
        LoadInitialCarSelectModel();
        break;
    }
}
