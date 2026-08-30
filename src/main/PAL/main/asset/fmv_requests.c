#include "game/asset.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"
#include "rage/track_asset_identity.h"


void LoadTrackDataAssets(void) {
    void *dst;
    s32 offset;

    switch (g_AssetLoadState) {
    case 1: {
        s32 assetIndex;
        dst = g_AssetLoadCursor;
        offset = g_CourseIndex * 2;
        assetIndex = (g_GrandPrixClass * 8) + offset + ASSET_TRACK_2ND_BASE;
        if (LoadAsset(assetIndex, dst) != 0) {
            TrackAssetIdentitySet(assetIndex);
            SetTrackCameraTable(SceneAssetBlock(0));
            SetEnvPaletteTable(SceneAssetBlock(1));
            SetEnvironmentScript(SceneAssetBlock(2));
            RegisterModelBank(GetModelBankHeader(SceneAssetBlock(3)), 1);
            InstallTrackPoints(SceneAssetBlock(4));
            RegisterCourseModels(GetCourseModelAssetHeader(SceneAssetBlock(5)));
            RegisterModelBank(GetModelBankHeader(SceneAssetBlock(6)), 2);
            InstallTerrainCellData(SceneAssetBlock(7));
            SetCourseObjects(SceneAssetBlock(8));
            InstallTrackEventData(SceneAssetBlock(9));
            SelectTrackCameraTable(SceneAssetBlock(10), 0);

            g_AssetLoadState = 2;
        }
        break;
    }
    case 2:
        if (EnableCdAudioMode() != 0) {
            g_AssetLoadState = 0;
        }
        break;
    }
}

void BeginIntroFmv(s32 returnScene) {
    u32 sectors;

    BeginFmv(returnScene);

    sectors = g_StreamCdEntries[0].size;
    g_StreamLoc = &g_StreamCdEntries[0];
    g_StreamSectorCount = sectors;
    g_StreamSectorLimit = sectors * 2;
}

void BeginClassFmv(s32 returnScene) {
    s32 index;
    u32 sectors;

    BeginFmv(returnScene);

    if (g_SeriesSelection != 0) {
        index = 5;
    } else {
        index = 1;
    }

    index += g_GrandPrixClass;
    sectors = g_StreamCdEntries[index].size;
    g_StreamLoc = &g_StreamCdEntries[index];
    g_StreamSectorCount = sectors;
    g_StreamSectorLimit = sectors * 2;
}

void BeginEndingFmv(s32 returnScene) {
    u32 sectors;

    BeginFmv(returnScene);

    sectors = g_StreamCdEntries[10].size;
    g_StreamLoc = &g_StreamCdEntries[10];
    g_StreamSectorCount = sectors;
    g_StreamSectorLimit = sectors * 4;
}

void ServiceAssetLoad(void) {
    if (g_AssetLoadState != 0) {
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
}
