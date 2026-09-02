#include "game/render.h"
#include "game/race.h"

#include "game/track_internal.h"

static void RestartRouteScenery(void) {
    const s32 series = g_RaceSeries;
    const s16 firstKeyframe =
        g_RouteSceneryData->firstKeyframe[series][0];
    SceneryMotionKeyframe *keyframe =
        &g_RouteSceneryData->keyframes[firstKeyframe];

    g_RouteSceneryKeyIndex = 0;
    g_RouteSceneryKeyframe = keyframe;
    g_RouteSceneryRotX = keyframe->rotationX;
    g_RouteSceneryRotY = keyframe->rotationY;
    g_RouteSceneryRotZ = keyframe->rotationZ;
    g_RouteSceneryPosition = g_RouteSceneryData->start[series].position;
    g_RouteSceneryClock = 1;
    g_RouteSceneryFrame = 0;
}

void SeedRouteScenery(void) {
    RestartRouteScenery();
}

void UpdateRouteScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    SVec vin;
    LVec vout;
    SceneryMotionKeyframe *keyframe;
    s32 elapsed;

    if (g_RouteSceneryClock <= 0) {
        return;
    }
    g_RouteSceneryClock++;
    g_RouteSceneryFrame++;

    keyframe = &g_RouteSceneryKeyframe[g_RouteSceneryKeyIndex];
    if (keyframe->duration == g_RouteSceneryFrame) {
        g_RouteSceneryKeyIndex++;
        g_RouteSceneryFrame = 0;
        keyframe++;
    }

    if (keyframe->duration == SCENERY_MOTION_END) {
        RestartRouteScenery();
        keyframe = g_RouteSceneryKeyframe;
    }

    elapsed = g_RouteSceneryFrame;
    g_RouteSceneryRotX = InterpolateSceneryMotionValue(
        keyframe->rotationX, keyframe[1].rotationX, elapsed,
        keyframe->duration);
    g_RouteSceneryRotY = InterpolateSceneryMotionValue(
        keyframe->rotationY, keyframe[1].rotationY, elapsed,
        keyframe->duration);
    g_RouteSceneryRotZ = InterpolateSceneryMotionValue(
        keyframe->rotationZ, keyframe[1].rotationZ, elapsed,
        keyframe->duration);

    vin.vx = 0;
    vin.vy = 0;
    vin.vz = -keyframe->speed * 4;
    BuildRotMatrixY(&mtx0, 0x800 - g_RouteSceneryRotY);

    BuildRotMatrixX(&mtx1, g_RouteSceneryRotX);
    MulMatrix2(&mtx0, &mtx1);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix(&mtx1, &mtx0);
    ApplyMatrix(&mtx1, &vin, &vout);

    g_RouteSceneryX += vout.x / 4;
    g_RouteSceneryY += vout.y / 4;
    g_RouteSceneryZ += vout.z / 4;
}
