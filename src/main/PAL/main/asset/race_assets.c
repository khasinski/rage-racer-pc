#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/race.h"
#include "game/cd.h"
#include "rage/render_world_game.h"

#include <string.h>

enum {
    RACE_LOAD_VOICE_HEADER = 1,
    RACE_WAIT_FOR_VOICE_AUDIO,
    RACE_LOAD_PLAYER_CAR,
    RACE_WAIT_FOR_ENGINE_AUDIO,
    RACE_LOAD_TRACK_TEXTURES,
    RACE_LOAD_TRACK_RUNTIME,
    RACE_ENABLE_CD_AUDIO,
};

enum {
    GRAND_PRIX_SCREEN_LOAD_ASSET = 1,
    COURSE_LOAD_TEXTURE_ASSET = 1,
};

typedef struct RaceCarAssetHeader {
    s32 specificationOffset;
    s32 audioHeaderOffset;
    s32 audioSequenceOffset;
    s32 audioBodyOffset;
    s32 imageOffset;
} RaceCarAssetHeader;

s32 RequestRaceAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_RACE,
                            RACE_LOAD_VOICE_HEADER, 0);
}

static void BeginRaceVoiceLoad(void) {
    memcpy(g_AssetLoadCursor, g_AssetBlockPtr,
           (size_t)g_RaceVoiceHeaderSize);
    StartAudioSlotLoad(AUDIO_SLOT_RACE_CUES, g_AssetLoadCursor,
                       g_AssetSubBlockPtr, 0);
    g_AssetLoadCursor += g_RaceVoiceHeaderSize;
    g_AssetLoadState = RACE_WAIT_FOR_VOICE_AUDIO;
}

static void AdvanceAfterAudioLoad(s32 nextState) {
    if (PollAudioSlotLoad() != 0) {
        g_AssetLoadState = nextState;
    }
}

static void LoadPlayerCarRaceAssets(void) {
    s32 carIndex = g_PlayerCarIndex;
    s32 carAsset =
        GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
    RaceCarAssetHeader *pack;
    u8 *audioHeader;
    u8 *audioTable;
    u8 *audioBody;
    u8 *carImage;
    s32 loadedSize;

    loadedSize = LoadAsset(
        CarVariantAssetIndex(ASSET_CAR_2ND_BASE, carAsset),
        g_AssetLoadCursor);
    if (loadedSize == 0) {
        return;
    }

    pack = (RaceCarAssetHeader *)g_AssetLoadCursor;
    if (loadedSize < (s32)sizeof(*pack) ||
        pack->specificationOffset < (s32)sizeof(*pack) ||
        pack->audioHeaderOffset <= pack->specificationOffset ||
        pack->audioSequenceOffset <= pack->audioHeaderOffset ||
        pack->audioBodyOffset <= pack->audioSequenceOffset ||
        pack->imageOffset <= pack->audioBodyOffset ||
        pack->imageOffset >= loadedSize) {
        g_AssetLoadState = 0;
        return;
    }

    GameRenderWorldSetTrackCarAsset(carAsset);
    g_CarSpec = GetGameCarSpec(
        g_AssetLoadCursor + pack->specificationOffset);
    audioHeader = g_AssetLoadCursor + pack->audioHeaderOffset;
    audioTable = g_AssetLoadCursor + pack->audioSequenceOffset;
    audioBody = g_AssetLoadCursor + pack->audioBodyOffset;
    StartAudioSlotLoad(AUDIO_SLOT_ENGINE, audioHeader, audioBody,
                       GetAssetHalfwords(audioTable));
    carImage = g_AssetLoadCursor + pack->imageOffset;
    if (!UploadImageAsset(GetImageAssetHeaderWords(carImage),
                          (size_t)(loadedSize - pack->imageOffset))) {
        g_AssetLoadState = 0;
        return;
    }
    g_AssetLoadCursor = audioBody;
    g_AssetLoadState = RACE_WAIT_FOR_ENGINE_AUDIO;
}

static void LoadTrackTextureAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_1ST_BASE, g_GrandPrixClass, g_CourseIndex);
    s32 loadedSize;

    loadedSize = LoadAsset(assetIndex, g_AssetLoadCursor);
    if (loadedSize == 0) return;

    if (!InstallTrackTextureAssetPack(g_AssetLoadCursor,
                                      (size_t)loadedSize)) {
        g_AssetLoadState = 0;
        return;
    }
    g_AssetLoadState = RACE_LOAD_TRACK_RUNTIME;
}

static void LoadTrackRuntimeAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_2ND_BASE, g_GrandPrixClass, g_CourseIndex);
    s32 loadedSize;

    loadedSize = LoadAsset(assetIndex, g_AssetLoadCursor);
    if (loadedSize == 0) return;

    if (!InstallTrackRuntimeAssetPack(g_AssetLoadCursor, (size_t)loadedSize,
                                      assetIndex, 1)) {
        g_AssetLoadState = 0;
        return;
    }
    g_AssetLoadState = RACE_ENABLE_CD_AUDIO;
}

void LoadRaceAssets(void) {
    switch (g_AssetLoadState) {
    case RACE_LOAD_VOICE_HEADER:
        BeginRaceVoiceLoad();
        break;
    case RACE_WAIT_FOR_VOICE_AUDIO:
        AdvanceAfterAudioLoad(RACE_LOAD_PLAYER_CAR);
        break;
    case RACE_LOAD_PLAYER_CAR:
        LoadPlayerCarRaceAssets();
        break;
    case RACE_WAIT_FOR_ENGINE_AUDIO:
        AdvanceAfterAudioLoad(RACE_LOAD_TRACK_TEXTURES);
        break;
    case RACE_LOAD_TRACK_TEXTURES:
        LoadTrackTextureAssets();
        break;
    case RACE_LOAD_TRACK_RUNTIME:
        LoadTrackRuntimeAssets();
        break;
    case RACE_ENABLE_CD_AUDIO:
        if (EnableCdAudioMode() != 0) {
            g_AssetLoadState = 0;
        }
        break;
    }
}

s32 RequestRaceStart(void) {
    return RequestAssetLoad(ASSET_REQUEST_GRAND_PRIX_SCREEN,
                            GRAND_PRIX_SCREEN_LOAD_ASSET, 1);
}

void LoadGrandPrixScreen(void) {
    s32 assetIndex;
    s32 loadedSize;

    if (g_AssetLoadState != GRAND_PRIX_SCREEN_LOAD_ASSET) return;

    assetIndex = ASSET_ROUND_SCREEN_BASE + g_GrandPrixSeries * 6 +
                 g_GrandPrixClass;
    loadedSize = LoadAsset(assetIndex, g_ImageBlockBuffer);
    if (loadedSize != 0) {
        g_ImageBlockSize = (size_t)loadedSize;
        g_AssetLoadState = 0;
    }
}

s32 RequestCourseTextureAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_COURSE_TEXTURES,
                            COURSE_LOAD_TEXTURE_ASSET, 0);
}

void LoadCourseTextureAssets(void) {
    s32 loadedSize;
    s32 assetIndex;

    if (g_AssetLoadState != COURSE_LOAD_TEXTURE_ASSET) return;

    assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_1ST_BASE, g_GrandPrixClass, g_CourseIndex);
    loadedSize = LoadAsset(assetIndex, g_AssetBase);
    if (loadedSize != 0) {
        g_AssetLoadState = 0;
        g_ImageBlockBuffer = g_AssetBase + loadedSize;
        g_ImageBlockSize = 0;
    }
}
