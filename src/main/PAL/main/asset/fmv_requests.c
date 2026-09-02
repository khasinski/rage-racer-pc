#include "game/asset.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"
static void LoadStandaloneTrackRuntimeAssets(void) {
    s32 assetIndex = ASSET_TRACK_2ND_BASE + (g_GrandPrixClass * 8) +
                     (g_CourseIndex * 2);

    if (LoadAsset(assetIndex, g_AssetLoadCursor) != 0) {
        InstallTrackRuntimeAssetPack(assetIndex, 0);
        g_AssetLoadState = 2;
    }
}

void LoadTrackDataAssets(void) {
    switch (g_AssetLoadState) {
    case 1:
        LoadStandaloneTrackRuntimeAssets();
        break;
    case 2:
        if (EnableCdAudioMode() != 0) {
            g_AssetLoadState = 0;
        }
        break;
    }
}

static void SetFmvStream(s32 index, u32 sectorLimitMultiplier) {
    u32 sectors;

    sectors = g_StreamCdEntries[index].size;
    g_StreamLoc = &g_StreamCdEntries[index];
    g_StreamSectorCount = sectors;
    g_StreamSectorLimit = sectors * sectorLimitMultiplier;
}

void BeginIntroFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SetFmvStream(0, 2);
}

void BeginClassFmv(s32 returnScene) {
    s32 index;

    BeginFmv(returnScene);

    if (g_SeriesSelection != 0) {
        index = 5;
    } else {
        index = 1;
    }

    index += g_GrandPrixClass;
    SetFmvStream(index, 2);
}

void BeginEndingFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SetFmvStream(10, 4);
}

void ServiceAssetLoad(void) {
    if (g_AssetLoadState == 0) return;

    switch (g_AssetRequestType) {
    case ASSET_REQUEST_INVALID:
    case ASSET_REQUEST_IDLE:
        break;
    case ASSET_REQUEST_BOOT:
        LoadBootAssets();
        break;
    case ASSET_REQUEST_SAVE_SCREEN:
        LoadSaveScreenAssets();
        break;
    case ASSET_REQUEST_SELECT_BGM:
        LoadSelectBgmAssets();
        break;
    case ASSET_REQUEST_CAR_SELECT:
        LoadCarSelectAssets();
        break;
    case ASSET_REQUEST_CAR_MODEL:
        LoadCarModel(g_PendingCarModelIndex);
        break;
    case ASSET_REQUEST_UPGRADED_CAR_MODEL:
        LoadUpgradedCarModel(g_PendingCarModelIndex);
        break;
    case ASSET_REQUEST_OPTION_SCREEN:
        LoadOptionScreenAssets();
        break;
    case ASSET_REQUEST_ROUND_SCREEN:
        LoadRoundAssets();
        break;
    case ASSET_REQUEST_RACE:
        LoadRaceAssets();
        break;
    case ASSET_REQUEST_GRAND_PRIX_SCREEN:
        LoadGrandPrixScreen();
        break;
    case ASSET_REQUEST_COURSE:
        LoadCourseAssets();
        break;
    case ASSET_REQUEST_TRACK_DATA:
        LoadTrackDataAssets();
        break;
    }
}
