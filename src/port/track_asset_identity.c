#include "rage/track_asset_identity.h"

static int s_trackAsset = -1;
static uint64_t s_trackAssetRevision;

void TrackAssetIdentitySet(int asset) {
    s_trackAsset = asset;
    s_trackAssetRevision++;
}

void TrackAssetIdentityInvalidate(void) {
    s_trackAssetRevision++;
}

uint32_t TrackAssetIdentityResolve(uint32_t fallback) {
    return s_trackAsset >= 0 ? (uint32_t)s_trackAsset : fallback;
}

uint64_t TrackAssetIdentityRevision(void) {
    return s_trackAssetRevision;
}
