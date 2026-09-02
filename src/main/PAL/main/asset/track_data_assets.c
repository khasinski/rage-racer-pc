#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/race.h"

enum {
    TRACK_DATA_LOAD_ASSET = 1,
    TRACK_DATA_ENABLE_CD_AUDIO = 2,
};

s32 RequestTrackDataAssets(void) {
    return RequestAssetLoad(ASSET_REQUEST_TRACK_DATA,
                            TRACK_DATA_LOAD_ASSET, 0);
}

static void LoadStandaloneTrackRuntimeAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_2ND_BASE, g_GrandPrixClass, g_CourseIndex);

    if (LoadAsset(assetIndex, g_AssetLoadCursor) == 0) return;
    InstallTrackRuntimeAssetPack(assetIndex, 0);
    g_AssetLoadState = TRACK_DATA_ENABLE_CD_AUDIO;
}

void LoadTrackDataAssets(void) {
    switch (g_AssetLoadState) {
    case TRACK_DATA_LOAD_ASSET:
        LoadStandaloneTrackRuntimeAssets();
        break;
    case TRACK_DATA_ENABLE_CD_AUDIO:
        if (EnableCdAudioMode() != 0) {
            g_AssetLoadState = 0;
        }
        break;
    }
}
