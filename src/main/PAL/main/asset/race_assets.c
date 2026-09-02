#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/race.h"
#include "game/cd.h"
#include "rage/render_world_game.h"

s32 RequestRaceAssets(void) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == ASSET_REQUEST_RACE) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    g_AssetRequestType = ASSET_REQUEST_RACE;
    g_AssetLoadState = 1;
    return 1;
}

static void BeginRaceVoiceLoad(void) {
    s32 *source = GetAssetWords(g_AssetBlockPtr);
    s32 *destination = GetAssetWords(g_AssetLoadCursor);
    s32 wordsRemaining = g_SharedAssetWord0 / sizeof(*source);

    while (wordsRemaining != 0) {
        *destination++ = *source++;
        wordsRemaining--;
    }
    StartAudioSlotLoad(2, g_AssetLoadCursor, g_AssetSubBlockPtr, 0);
    g_AssetLoadCursor += g_SharedAssetWord0;
    g_AssetLoadState = 2;
}

static void AdvanceAfterAudioLoad(s32 nextState) {
    if ((s16)PollAudioSlotLoad() != 0) {
        g_AssetLoadState = nextState;
    }
}

static void LoadPlayerCarRaceAssets(void) {
    s32 carIndex = g_PlayerCarIndex;
    s32 carAsset =
        GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
    GameSceneAssetHeader *pack;
    u8 *audioHeader;
    u8 *audioTable;
    u8 *audioBody;

    GameRenderWorldSetTrackCarAsset(carAsset);
    if (LoadAsset((carAsset * 2) + 11, g_AssetLoadCursor) == 0) {
        return;
    }

    pack = GetSceneAssetHeader(g_AssetLoadCursor);
    g_CarSpec = GetGameCarSpec(GetSceneAssetAddress(pack, pack->offsets[0]));
    audioHeader = GetSceneAssetAddress(pack, pack->offsets[1]);
    audioTable = GetSceneAssetAddress(pack, pack->offsets[2]);
    audioBody = GetSceneAssetAddress(pack, pack->offsets[3]);
    g_AssetBlockPtr = audioHeader;
    g_AssetBlockPtr2 = audioTable;
    g_AssetSubBlockPtr = audioBody;
    StartAudioSlotLoad(3, audioHeader, audioBody,
                       GetAssetHalfwords(audioTable));
    g_AssetBlockPtr = GetSceneAssetAddress(pack, pack->offsets[4]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetLoadCursor = audioBody;
    g_AssetLoadState = 4;
}

static s32 RaceCourseAssetIndex(s32 base) {
    return base + (g_GrandPrixClass * 8) + (g_CourseIndex * 2);
}

static void LoadTrackTextureAssets(void) {
    GameSceneAssetHeader *pack;

    if (LoadAsset(RaceCourseAssetIndex(ASSET_TRACK_1ST_BASE),
                  g_AssetLoadCursor) == 0) {
        return;
    }

    pack = GetSceneAssetHeader(g_AssetLoadCursor);
    g_AssetBlockPtr = GetSceneAssetAddress(pack, pack->offsets[0]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(pack, pack->offsets[1]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(pack, pack->offsets[2]);
    UploadImageBlock(GetImageAssetHeaderWords(g_AssetBlockPtr));
    g_AssetBlockPtr = GetSceneAssetAddress(pack, pack->offsets[3]);
    g_AssetSubBlockPtr = GetSceneAssetAddress(pack, pack->offsets[4]);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetBlockPtr));
    StoreTeamLogoImage(g_AssetLoadCursor);
    g_TrackTextureShadow = GetTrackTextureShadowRows(g_AssetLoadCursor);
    UploadImageAsset(GetImageAssetHeaderWords(g_AssetSubBlockPtr));
    ResetTrackTextureSwap();
    g_AssetLoadCursor += TRACK_TEXTURE_SHADOW_SIZE;
    g_AssetLoadState = 6;
}

static void LoadTrackRuntimeAssets(void) {
    s32 assetIndex = RaceCourseAssetIndex(ASSET_TRACK_2ND_BASE);

    if (LoadAsset(assetIndex, g_AssetLoadCursor) == 0) {
        return;
    }

    InstallTrackRuntimeAssetPack(assetIndex, 1);
    g_AssetLoadState = 7;
}

void LoadRaceAssets(void) {
    switch (g_AssetLoadState) {
    case 1:
        BeginRaceVoiceLoad();
        break;
    case 2:
        AdvanceAfterAudioLoad(3);
        break;
    case 3:
        LoadPlayerCarRaceAssets();
        break;
    case 4:
        AdvanceAfterAudioLoad(5);
        break;
    case 5:
        LoadTrackTextureAssets();
        break;
    case 6:
        LoadTrackRuntimeAssets();
        break;
    case 7:
        if (EnableCdAudioMode() != 0) {
            g_AssetLoadState = 0;
        }
        break;
    }
}

s32 RequestRaceStart(void) {
    s32 state;

    if (g_AssetLoadState != 0) {
        return 1;
    }

    state = ASSET_REQUEST_GRAND_PRIX_SCREEN;
    if (g_AssetRequestType == state) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    ResetCdAudioState();
    g_AssetRequestType = state;
    g_AssetLoadState = 1;
    return 1;
}

void LoadGrandPrixScreen(void) {
    s32 base;
    s32 offset;
    s32 loaded;

    if (g_AssetLoadState == 1) {
        offset = g_GrandPrixSeries * 6;
        base = g_GrandPrixClass + ASSET_ROUND_SCREEN_BASE;
        loaded = LoadAsset(offset + base, g_ImageBlockBuffer);
        if (loaded != 0) {
            g_AssetLoadState = 0;
        }
    }
}

s32 RequestTrackLoad(void) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == ASSET_REQUEST_COURSE) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    g_AssetRequestType = ASSET_REQUEST_COURSE;
    g_AssetLoadState = 1;
    return 1;
}

void LoadCourseAssets(void) {
    s32 loaded;

    if (g_AssetLoadState == 1) {
        s32 courseOffset = g_CourseIndex * 2;
        s32 classBase = (g_GrandPrixClass * 8) + ASSET_TRACK_1ST_BASE;

        loaded = LoadAsset(courseOffset + classBase, g_AssetBase);
        if (loaded != 0) {
            g_AssetLoadState = 0;
            g_ImageBlockBuffer = g_AssetBase + loaded;
        }
    }
}
