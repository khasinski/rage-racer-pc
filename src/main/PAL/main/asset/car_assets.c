#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/audio.h"

/* g_CarModelAsset really is per-screen typed; see game/asset.h.
 * g_TeamLogoSampleData's other reader, menu/team_logo.c, walks it through a
 * TeamLogoSample struct private to that file. */

enum {
    CAR_SELECT_BEGIN_AUDIO = 1,
    CAR_SELECT_WAIT_FOR_AUDIO,
    CAR_SELECT_LOAD_SHARED_ASSETS,
    CAR_SELECT_LOAD_INITIAL_MODEL,
};

s32 RequestCarSelectAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_CAR_SELECT,
                            CAR_SELECT_BEGIN_AUDIO, 0);
}

static void BeginCarSelectAudioLoad(void) {
    StartAudioSlotLoad(AUDIO_SLOT_SEQUENCE, g_AssetBlockPtr, g_AssetSubBlockPtr,
                       GetAssetHalfwords(g_AssetBlockPtr2));
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
    GameSceneAssetHeader *header;

    if (LoadAsset(ASSET_CAR_SELECT_SCREEN, g_AssetLoadCursor) == 0) {
        return;
    }

    RegisterModelBank(GetModelBankHeader(g_AssetLoadCursor + 0xC), 0xE);
    header = GetSceneAssetHeader(g_AssetLoadCursor);
    g_TeamLogoSampleData =
        GetTeamLogoSample(GetSceneAssetAddress(header, header->offsets[0]));
    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[1]);
    RegisterCourseModels(GetCourseModelAssetHeader(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(header, header->offsets[2]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_CarModelBuffer = g_AssetBlockPtr;
    g_ImageBlockBuffer = g_CarModelBuffer + CAR_MODEL_BUFFER_SIZE;
    g_AssetLoadState = CAR_SELECT_LOAD_INITIAL_MODEL;
}

static void LoadInitialCarSelectModel(void) {
    s32 carIndex = g_PlayerCarIndex;
    s32 variantIndex = GetCarAssetIndex(
        carIndex, g_CarTable[carIndex].modelVariant);
    s32 assetIndex = CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variantIndex);

    if (LoadAsset(assetIndex, g_CarModelBuffer) == 0) {
        return;
    }

    InstallCarModelAsset(GetCarModelAsset(g_CarModelBuffer), 0, carIndex);
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
