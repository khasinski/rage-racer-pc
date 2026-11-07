#ifndef GAME_TRACK_DATA_H
#define GAME_TRACK_DATA_H
#include "game/track.h"
#include "game/scene_asset.h"

typedef struct TrackData {
    TrackRoute route;
    const TrackEventData *events;
} TrackData;

/* Borrows blocks returned by ReadSceneAssetBlocks. Failed validation leaves
 * output unchanged; its source storage must outlive the view. */
int ReadTrackData(const SceneAssetBlock blocks[SCENE_ASSET_BLOCK_COUNT], TrackData *track);

/* Copies only physics blocks from a retail runtime pack. Caller owns the
 * returned object and frees it after all races borrowing its views finish. */
TrackData *CopyTrackData(const void *data, size_t size);
/* Only for CopyTrackData/LoadTrackData results, never borrowed stack views. */
void FreeTrackData(TrackData *data);
#endif
