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

/* Performance setup derives gear loads and repairs invalid limits in-place.
 * Keep those runtime values out of the serialized car pack. */
static GameCarSpec s_RuntimeCarSpec;

s32 RequestRaceAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_RACE,
                            RACE_LOAD_VOICE_HEADER, 0);
}

static void BeginRaceVoiceLoad(void) {
    AudioSlotAsset asset;
    size_t headerSize;

    if (g_RaceVoiceHeaderSize <= 0) {
        FailAssetLoad();
        return;
    }
    headerSize = (size_t)g_RaceVoiceHeaderSize;
    if (g_AssetBlockPtr == NULL || headerSize > g_AssetBlockSize ||
        PortAssetRoomAt(g_AssetLoadCursor) < headerSize) {
        FailAssetLoad();
        return;
    }
    memcpy(g_AssetLoadCursor, g_AssetBlockPtr, headerSize);
    asset = (AudioSlotAsset){
        .vabHeader = g_AssetLoadCursor,
        .vabHeaderSize = headerSize,
        .vabBody = g_AssetSubBlockPtr,
        .vabBodySize = g_AssetSubBlockSize,
    };
    if (StartAudioSlotLoad(AUDIO_SLOT_RACE_CUES, &asset) < 0) {
        FailAssetLoad();
        return;
    }
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
    s32 carAsset;
    const RaceCarAssetHeader *pack;
    const u8 *sourceSpec;
    u8 *audioHeader;
    u8 *audioTable;
    u8 *audioBody;
    u8 *carImage;
    AudioSlotAsset audioAsset;
    s32 loadedSize;

    if ((u32)carIndex >= GAME_CAR_COUNT || g_CarTable == NULL) {
        FailAssetLoad();
        return;
    }
    carAsset = GetCarAssetIndex(carIndex,
                                g_CarTable[carIndex].modelVariant);
    if (carAsset < 0) {
        FailAssetLoad();
        return;
    }

    loadedSize = LoadAsset(
        CarVariantAssetIndex(ASSET_CAR_2ND_BASE, carAsset),
        g_AssetLoadCursor);
    if (AssetLoadDidNotComplete(loadedSize)) {
        return;
    }

    pack = (const RaceCarAssetHeader *)(const void *)g_AssetLoadCursor;
    if (loadedSize < (s32)sizeof(*pack) ||
        pack->specificationOffset < (s32)sizeof(*pack) ||
        pack->audioHeaderOffset <= pack->specificationOffset ||
        pack->audioHeaderOffset - pack->specificationOffset <
            (s32)sizeof(GameCarSpec) ||
        pack->audioSequenceOffset <= pack->audioHeaderOffset ||
        pack->audioBodyOffset <= pack->audioSequenceOffset ||
        pack->audioBodyOffset - pack->audioSequenceOffset <
            ENGINE_SOUND_PARAMETER_TABLE_SIZE ||
        pack->imageOffset <= pack->audioBodyOffset ||
        pack->imageOffset >= loadedSize) {
        FailAssetLoad();
        return;
    }

    audioHeader = g_AssetLoadCursor + pack->audioHeaderOffset;
    audioTable = g_AssetLoadCursor + pack->audioSequenceOffset;
    audioBody = g_AssetLoadCursor + pack->audioBodyOffset;
    carImage = g_AssetLoadCursor + pack->imageOffset;
    sourceSpec = g_AssetLoadCursor + pack->specificationOffset;
    if (!IsValidImageAsset(
            GetImageAssetHeaderWords(carImage),
            (size_t)(loadedSize - pack->imageOffset))) {
        FailAssetLoad();
        return;
    }
    audioAsset = (AudioSlotAsset){
        .vabHeader = audioHeader,
        .vabHeaderSize =
            (size_t)(pack->audioSequenceOffset - pack->audioHeaderOffset),
        .vabBody = audioBody,
        .vabBodySize = (size_t)(pack->imageOffset - pack->audioBodyOffset),
        .auxiliaryData = audioTable,
        .auxiliarySize =
            (size_t)(pack->audioBodyOffset - pack->audioSequenceOffset),
    };
    if (StartAudioSlotLoad(AUDIO_SLOT_ENGINE, &audioAsset) < 0) {
        FailAssetLoad();
        return;
    }
    memcpy(&s_RuntimeCarSpec, sourceSpec, sizeof(s_RuntimeCarSpec));
    GameRenderWorldSetTrackCarAsset(carAsset);
    g_CarSpec = &s_RuntimeCarSpec;
    UploadImageAsset(GetImageAssetHeaderWords(carImage),
                     (size_t)(loadedSize - pack->imageOffset));
    g_AssetLoadCursor = audioBody;
    g_AssetLoadState = RACE_WAIT_FOR_ENGINE_AUDIO;
}

static void LoadTrackTextureAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_1ST_BASE, g_GrandPrixClass, g_CourseIndex);
    s32 loadedSize;

    if (assetIndex < 0) {
        FailAssetLoad();
        return;
    }
    loadedSize = LoadAsset(assetIndex, g_AssetLoadCursor);
    if (AssetLoadDidNotComplete(loadedSize)) return;

    if (!InstallTrackTextureAssetPack(g_AssetLoadCursor,
                                      (size_t)loadedSize)) {
        FailAssetLoad();
        return;
    }
    g_AssetLoadState = RACE_LOAD_TRACK_RUNTIME;
}

static void LoadTrackRuntimeAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_2ND_BASE, g_GrandPrixClass, g_CourseIndex);
    s32 loadedSize;

    if (assetIndex < 0) {
        FailAssetLoad();
        return;
    }
    loadedSize = LoadAsset(assetIndex, g_AssetLoadCursor);
    if (AssetLoadDidNotComplete(loadedSize)) return;

    if (!InstallTrackRuntimeAssetPack(g_AssetLoadCursor, (size_t)loadedSize,
                                      assetIndex, 1)) {
        FailAssetLoad();
        return;
    }
    g_AssetLoadState = RACE_ENABLE_CD_AUDIO;
}

void LoadRaceAssets(void) {
    if (g_AssetLoadState == 0) {
        return;
    }

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
    default:
        FailAssetLoad();
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

    if (g_AssetLoadState == 0) return;
    if (g_AssetLoadState != GRAND_PRIX_SCREEN_LOAD_ASSET) {
        FailAssetLoad();
        return;
    }

    if ((u32)g_GrandPrixSeries >= GRAND_PRIX_SERIES_COUNT ||
        (u32)g_GrandPrixClass >= GRAND_PRIX_PRIZE_CLASS_COUNT) {
        FailAssetLoad();
        return;
    }
    assetIndex = ASSET_ROUND_SCREEN_BASE +
                 g_GrandPrixSeries * GRAND_PRIX_PRIZE_CLASS_COUNT +
                 g_GrandPrixClass;
    loadedSize = LoadAsset(assetIndex, g_ImageBlockBuffer);
    if (AssetLoadDidNotComplete(loadedSize)) return;
    g_ImageBlockSize = (size_t)loadedSize;
    g_AssetLoadState = 0;
}

s32 RequestCourseTextureAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_COURSE_TEXTURES,
                            COURSE_LOAD_TEXTURE_ASSET, 0);
}

void LoadCourseTextureAssets(void) {
    s32 loadedSize;
    s32 assetIndex;

    if (g_AssetLoadState == 0) return;
    if (g_AssetLoadState != COURSE_LOAD_TEXTURE_ASSET) {
        FailAssetLoad();
        return;
    }

    assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_1ST_BASE, g_GrandPrixClass, g_CourseIndex);
    if (assetIndex < 0) {
        FailAssetLoad();
        return;
    }
    loadedSize = LoadAsset(assetIndex, g_AssetBase);
    if (AssetLoadDidNotComplete(loadedSize)) return;
    g_AssetLoadState = 0;
    g_ImageBlockBuffer = g_AssetBase + loadedSize;
    g_ImageBlockSize = 0;
}
