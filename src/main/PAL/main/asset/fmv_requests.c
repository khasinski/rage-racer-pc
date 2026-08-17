#include "common.h"
#include "game/asset.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"


void LoadTrackDataAssets(void) {
    GameSceneAssetHeader *header;
    void *dst;
    s32 offset;

    switch (g_AssetLoadState) {
    case 1:
        dst = g_AssetLoadCursor;
        offset = g_CourseIndex * 2;
        if (LoadAsset((g_GrandPrixClass * 8) + offset + ASSET_TRACK_2ND_BASE, dst) != 0) {
            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[0];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            SetTrackCameraTable(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[1];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            SetEnvPaletteTable(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[2];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            SetEnvironmentScript(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[3];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            RegisterModelBank(GetModelBankHeader(g_AssetBlockPtr), 1);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[4];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            InstallTrackPoints(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[5];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            RegisterCourseModels(GetCourseModelAssetHeader(g_AssetBlockPtr));

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[6];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            RegisterModelBank(GetModelBankHeader(g_AssetBlockPtr), 2);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[7];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            InstallTerrainCellData(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[8];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            SetCourseObjects(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[9];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            InstallTrackEventData(g_AssetBlockPtr);

            header = GetSceneAssetHeader(g_AssetLoadCursor);
            offset = header->offsets[10];
            g_AssetBlockPtr = GetSceneAssetAddress(header, offset);
            SelectTrackCameraTable(g_AssetBlockPtr, 0);

            g_AssetLoadState = 2;
        }
        break;
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
