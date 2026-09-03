#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/cd.h"
#include "game/render.h"

enum { OPTION_SCREEN_LOAD_ASSET = 1 };

s32 RequestOptionScreenAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_OPTION_SCREEN,
                            OPTION_SCREEN_LOAD_ASSET, 1);
}

void LoadOptionScreenAssets(void) {
    const OptionScreenAsset *asset;
    s32 loadedSize;

    if (g_AssetLoadState == 0) return;
    if (g_AssetLoadState != OPTION_SCREEN_LOAD_ASSET) {
        FailAssetLoad();
        return;
    }

    loadedSize = LoadAsset(ASSET_OPTION_SCREEN, g_AssetBase);
    if (AssetLoadDidNotComplete(loadedSize)) return;
    asset = (const OptionScreenAsset *)(const void *)g_AssetBase;
    if (loadedSize < (s32)sizeof(asset->imageOffset)) {
        FailAssetLoad();
        return;
    }

    if (asset->imageOffset <= (s32)offsetof(OptionScreenAsset, modelBank) ||
        asset->imageOffset > loadedSize ||
        !RegisterModelBank(
            &asset->modelBank,
            (size_t)asset->imageOffset -
                offsetof(OptionScreenAsset, modelBank),
            0)) {
        FailAssetLoad();
        return;
    }
    SelectModelBank(0);
    g_ImageBlockBuffer = g_AssetBase + asset->imageOffset;
    g_ImageBlockSize = (size_t)(loadedSize - asset->imageOffset);
    g_AssetLoadState = 0;
}
