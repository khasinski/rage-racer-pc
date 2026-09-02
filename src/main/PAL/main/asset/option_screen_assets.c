#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/cd.h"
#include "game/render.h"

void RequestOptionScreenAssets(void) {
    RequestAssetLoad(ASSET_REQUEST_OPTION_SCREEN, 1, 1);
}

void LoadOptionScreenAssets(void) {
    OptionScreenAsset *asset;

    if (g_AssetLoadState != 1 ||
        LoadAsset(ASSET_OPTION_SCREEN, g_AssetBase) == 0) {
        return;
    }

    asset = GetOptionScreenAsset(g_AssetBase);
    RegisterModelBank(&asset->modelBank, 0);
    SelectModelBank(0);
    g_ImageBlockBuffer = g_AssetBase + asset->imageOffset;
    g_AssetLoadState = 0;
}
