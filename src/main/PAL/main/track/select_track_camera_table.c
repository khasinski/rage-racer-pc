#include "game/race.h"
#include "game/track_camera_internal.h"

static s32 CameraListHasTerminator(const TrackCameraTable *table, size_t size,
                                   s32 offset) {
    const GameTrackCameraNode *node;
    size_t remaining;

    if (offset < (s32)sizeof(*table) ||
        offset % (s32)_Alignof(GameTrackCameraNode) != 0 ||
        (size_t)offset > size) {
        return 0;
    }
    remaining = size - (size_t)offset;
    node = (const GameTrackCameraNode *)((const u8 *)table + offset);
    while (remaining >= sizeof(*node)) {
        if (node->trackSection.value == -1) return 1;
        node++;
        remaining -= sizeof(*node);
    }
    return 0;
}

s32 IsValidTrackCameraTable(const TrackCameraTable *table, size_t size,
                            s32 useSeriesCamera) {
    s32 offset;

    if (table == NULL || size < sizeof(*table)) return 0;

    offset = table->defaultOffset;

    if (useSeriesCamera) {
        offset = table->seriesOffset[g_GrandPrixSeries != 0];
    }

    return CameraListHasTerminator(table, size, offset);
}

s32 SelectTrackCameraTable(TrackCameraTable *table, size_t size,
                           s32 useSeriesCamera) {
    s32 offset;

    if (!IsValidTrackCameraTable(table, size, useSeriesCamera)) {
        g_TrackCameras = NULL;
        return 0;
    }

    offset = useSeriesCamera
                 ? table->seriesOffset[g_GrandPrixSeries != 0]
                 : table->defaultOffset;

    g_TrackCameras = (GameTrackCameraNode *)((u8 *)table + offset);
    return 1;
}
