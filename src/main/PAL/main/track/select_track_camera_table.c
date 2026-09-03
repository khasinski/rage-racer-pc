#include "game/race.h"
#include "game/track_camera_internal.h"

static const GameTrackCameraNode *FindValidCameraList(
    const TrackCameraTable *table, size_t size, s32 useSeriesCamera) {
    s32 offset;
    const GameTrackCameraNode *node;
    const GameTrackCameraNode *first;
    size_t remaining;

    if (table == NULL || size < sizeof(*table)) return NULL;
    offset = useSeriesCamera
                 ? table->seriesOffset[g_GrandPrixSeries != 0]
                 : table->defaultOffset;
    if (offset < (s32)sizeof(*table) ||
        offset % (s32)_Alignof(GameTrackCameraNode) != 0 ||
        (size_t)offset > size) {
        return NULL;
    }
    remaining = size - (size_t)offset;
    first = (const GameTrackCameraNode *)((const u8 *)table + offset);
    node = first;
    while (remaining >= sizeof(*node)) {
        if (node->trackSection.value == -1) return first;
        if ((u16)node->mode > TRACK_CAMERA_ORBIT) return NULL;
        node++;
        remaining -= sizeof(*node);
    }
    return NULL;
}

s32 IsValidTrackCameraTable(const TrackCameraTable *table, size_t size,
                            s32 useSeriesCamera) {
    return FindValidCameraList(table, size, useSeriesCamera) != NULL;
}

s32 SelectTrackCameraTable(const TrackCameraTable *table, size_t size,
                           s32 useSeriesCamera) {
    const GameTrackCameraNode *cameras =
        FindValidCameraList(table, size, useSeriesCamera);

    if (cameras == NULL) {
        g_TrackCameras = NULL;
        return 0;
    }

    g_TrackCameras = cameras;
    return 1;
}
