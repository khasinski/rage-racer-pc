#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/cd.h"
#include "game/render.h"

enum { OPTION_SCREEN_LOAD_ASSET = 1 };

void RequestOptionScreenAssets(void) {
    RequestAssetLoad(ASSET_REQUEST_OPTION_SCREEN, OPTION_SCREEN_LOAD_ASSET, 1);
}

void LoadOptionScreenAssets(void) {
    OptionScreenAsset *asset;

    if (g_AssetLoadState != OPTION_SCREEN_LOAD_ASSET ||
        LoadAsset(ASSET_OPTION_SCREEN, g_AssetBase) == 0) {
        return;
    }

    asset = GetOptionScreenAsset(g_AssetBase);
    RegisterModelBank(&asset->modelBank, 0);
    SelectModelBank(0);
    g_ImageBlockBuffer = g_AssetBase + asset->imageOffset;
    g_AssetLoadState = 0;
}
