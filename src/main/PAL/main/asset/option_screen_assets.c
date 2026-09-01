#include "game/asset.h"
#include "game/cd.h"
#include "game/render.h"

void RequestOptionScreenAssets(void) {
    if (g_AssetLoadState != 0) {
        return;
    }
    if (g_AssetRequestType == ASSET_REQUEST_OPTION_SCREEN) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return;
    }

    ResetCdAudioState();
    g_AssetRequestType = ASSET_REQUEST_OPTION_SCREEN;
    g_AssetLoadState = 1;
}

void LoadOptionScreenAssets(void) {
    OptionScreenAsset *asset;

    if (g_AssetLoadState != 1 || LoadAsset(9, g_AssetBase) == 0) {
        return;
    }

    asset = GetOptionScreenAsset(g_AssetBase);
    RegisterModelBank(&asset->modelBank, 0);
    SelectModelBank(0);
    g_ImageBlockBuffer = g_AssetBase + asset->imageOffset;
    g_AssetLoadState = 0;
}
