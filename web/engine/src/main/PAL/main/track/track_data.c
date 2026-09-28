#include "game/track_data.h"
#include "game/scene_asset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ReadTrackData(const SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT], TrackData *track) {
    if (!blocks || !track) return 0;
    TrackData view;
    const SceneAssetBlock points = blocks[SCENE_POINTS];
    const SceneAssetBlock events = blocks[SCENE_EVENTS];
    if (!ReadTrackRoute(points.data, points.size, &view.route) ||
        !(view.events = ReadTrackEvents(events.data, events.size))) return 0;
    *track = view;
    return 1;
}

TrackData *CopyTrackData(const void *data, size_t size) {
    SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT];
    if (!ReadSceneAssetBlocks(data, size, blocks)) return NULL;
    const SceneAssetBlock points = blocks[SCENE_POINTS];
    const SceneAssetBlock events = blocks[SCENE_EVENTS];
    TrackData view;
    if (!ReadTrackData(blocks, &view)) return NULL;
    const size_t pointBytes = (points.size + 3u) & ~(size_t)3;
    TrackData *copy = malloc(sizeof(*copy) + pointBytes + events.size);
    if (copy == NULL) return NULL;
    u8 *storage = (u8 *)(copy + 1);
    memcpy(storage, points.data, points.size);
    memcpy(storage + pointBytes, events.data, events.size);
    ReadTrackRoute((const TrackPointTable *)storage, points.size, &copy->route);
    copy->events = (const TrackEventData *)(storage + pointBytes);
    return copy;
}

void FreeTrackData(TrackData *data) { free(data); }
