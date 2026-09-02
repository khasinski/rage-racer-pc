#include "game/asset_internal.h"
#include "game/cd.h"

s32 RequestAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == request) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    if (resetCdAudio) {
        ResetCdAudioState();
    }
    g_AssetRequestType = request;
    g_AssetLoadState = firstLoadState;
    return 1;
}
