#ifndef GAME_CAMERA_TYPES_H
#define GAME_CAMERA_TYPES_H

#include "common.h"

typedef enum CameraViewMode {
    CAMERA_VIEW_INVALID = -1,
    CAMERA_VIEW_CAR,
    CAMERA_VIEW_CHASE,
    CAMERA_VIEW_TRACK
} CameraViewMode;

typedef struct CarModelRenderParams {
    s16 axis0;
    u16 axis1;
    u16 axis2;
    s16 horizon;
} CarModelRenderParams;

typedef struct TrackRenderTable {
    s32 textureSectionLo;
    s32 textureSectionHi;
    s32 environmentScriptOffset;
    CarModelRenderParams models[1];
} TrackRenderTable;

#endif
