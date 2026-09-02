#include "game/asset.h"

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
    case ASSET_REQUEST_UPGRADED_CAR_MODEL:
        LoadPendingCarModelAsset();
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
    case ASSET_REQUEST_COURSE_TEXTURES:
        LoadCourseTextureAssets();
        break;
    case ASSET_REQUEST_TRACK_DATA:
        LoadTrackDataAssets();
        break;
    }
}
