#ifndef GAME_TRACK_CAMERA_INTERNAL_H
#define GAME_TRACK_CAMERA_INTERNAL_H

#include <stddef.h>

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

typedef enum TrackCameraNodeMode {
    TRACK_CAMERA_CAR = 0,
    TRACK_CAMERA_CHASE = 1,
    TRACK_CAMERA_BLENDED_NODE = 2,
    TRACK_CAMERA_PATH = 3,
    TRACK_CAMERA_SLIDING_NODE = 4,
    TRACK_CAMERA_ORBIT = 5,
} TrackCameraNodeMode;

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

extern const GameTrackCameraNode *g_TrackCameras;
extern u16 g_TrackSectionCount;
s32 IsValidTrackCameraTable(const TrackCameraTable *table, size_t size,
                            s32 useSeriesCamera);
s32 SelectTrackCameraTable(const TrackCameraTable *table, size_t size,
                           s32 useSeriesCamera);

#endif
