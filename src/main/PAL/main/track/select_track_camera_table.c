#include "game/race.h"
#include "game/track_camera_internal.h"

void SelectTrackCameraTable(TrackCameraTable *table, s32 useSeriesCamera) {
    s32 offset;

    if (table == NULL) {
        g_TrackCameras = NULL;
        return;
    }

    offset = table->defaultOffset;

    if (useSeriesCamera) {
        offset = table->seriesOffset[g_GrandPrixSeries != 0];
    }

    g_TrackCameras = offset >= (s32)sizeof(*table)
        ? ResolveTrackCameraOffset(table, offset)
        : NULL;
}
