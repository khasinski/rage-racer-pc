#ifndef GAME_TRACK_CAMERA_INTERNAL_H
#define GAME_TRACK_CAMERA_INTERNAL_H

#include "common.h"
#include "game/vector.h"

typedef union GameTrackCameraData {
    struct {
        s32 x;
        s32 y;
        s32 z;
        s32 blend;
    } world;
    struct {
        s32 pitch;
        s32 yaw;
        s32 roll;
        s32 distance;
    } orientation;
    s32 value[4];
    Block16 block;
} GameTrackCameraData;

typedef union TrackCameraSection {
    s16 value;
    u16 raw;
} TrackCameraSection;

typedef struct GameTrackCameraNode {
    GameTrackCameraData data;
    s32 offset[3];
    s32 duration;
    s16 mode;
    TrackCameraSection trackSection;
} GameTrackCameraNode;

typedef struct TrackCameraTable {
    s32 seriesOffset[2];
    s32 defaultOffset;
} TrackCameraTable;

extern GameTrackCameraNode *g_TrackCameras;
extern u16 g_TrackSectionCount;
void SelectTrackCameraTable(TrackCameraTable *table, s32 useSeriesCamera);

#endif
