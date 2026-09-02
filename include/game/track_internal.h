#ifndef GAME_TRACK_INTERNAL_H
#define GAME_TRACK_INTERNAL_H

#include "common.h"
#include "game/track.h"
#include "game/track_camera_internal.h"

static inline s32 LerpColorChannel(s32 from, s32 to, s32 blend) {
    return from + (((to - from) * blend) >> 12);
}

typedef struct CourseObject {
    s16 modelId;
    s16 field2;
    s32 x;
    s32 y;
    s32 z;
    s32 flags;
} CourseObject;

typedef struct CourseObjectTable {
    u32 count;
    CourseObject objects[1];
} CourseObjectTable;

typedef struct StartGridSceneryStep {
    s16 x;
    s16 y;
} StartGridSceneryStep;

typedef struct PathSceneryClock {
    s16 posFrame;
    s16 rotFrame;
} PathSceneryClock;

static inline s16 NormalizePathSceneryRate(s16 rate) {
    if (rate < 0) {
        return (s16)-rate;
    }
    return rate == 0 ? 1 : rate;
}

typedef struct PathSceneryTransform {
    Block16 position;
    SVec rotation;
} PathSceneryTransform;

extern GameTrackArcCenter *g_TrackArcCenters;
extern s32 g_EnvScriptClock;
extern CourseObject *g_CourseObjects;
extern s32 g_CourseObjectCount;
extern StartGridSceneryStep g_StartGridSceneryStep[];
extern PathSceneryClock g_PathSceneryClock;
extern PathSceneryTransform g_PathSceneryTransform;
extern s32 g_TrackLength;
extern TrackEventData *g_TrackEventData;

extern SceneryMotionData *g_RouteSceneryData;
extern PathSceneryRotationData *g_PathSceneryRotData;
extern SceneryMotionData *g_FlybySceneryData;
extern PathSceneryPositionData *g_PathSceneryPosData;

extern s32 g_RouteSceneryClock;
extern s16 g_RouteSceneryKeyIndex;
extern s32 g_RouteSceneryRotY;
extern SceneryMotionKeyframe *g_RouteSceneryKeyframe;

extern s32 g_EnvScriptLength;
extern GameEnvironmentCue *g_EnvScriptCursor;

extern s32 g_SkyRowBase;

extern FlybySceneryState g_FlybyScenery;

void GetVisibleCellScanOffset(s32 direction, s32 cellIndex, s32 rearView,
                              s32 offset[2]);

/* Where a car and the two track points around it sit on a curve's arc: the
 * offsets from the arc centre, the angle each stands at, and the radius each
 * is out by. Callers decide for themselves what to do with the span between
 * the two points, which is where they stop agreeing. */
struct CarTrackWork;
void CarTrackMeasureArc(struct CarTrackWork *work, s32 arcIndex, s32 carX,
                        s32 carZ, const GameTrackPoint *point,
                        const GameTrackPoint *nextPoint);

#endif
