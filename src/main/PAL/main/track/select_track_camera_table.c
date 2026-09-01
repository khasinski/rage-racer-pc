#include "game/race.h"
#include "game/track_camera_internal.h"

void SelectTrackCameraTable(void *block, s32 useSeriesCamera) {
    TrackCameraTable *table = block;
    s32 offset = table->defaultOffset;

    if (useSeriesCamera) {
        offset = table->seriesOffset[g_GrandPrixSeries != 0];
    }

    g_TrackCameras = ResolveTrackCameraOffset(table, offset);
}
