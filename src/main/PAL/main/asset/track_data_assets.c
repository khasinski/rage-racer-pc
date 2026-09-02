#include "game/asset.h"
#include "game/race.h"

static void LoadStandaloneTrackRuntimeAssets(void) {
    s32 assetIndex = TrackCourseAssetIndex(
        ASSET_TRACK_2ND_BASE, g_GrandPrixClass, g_CourseIndex);

    if (LoadAsset(assetIndex, g_AssetLoadCursor) == 0) return;
    InstallTrackRuntimeAssetPack(assetIndex, 0);
    g_AssetLoadState = 2;
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
