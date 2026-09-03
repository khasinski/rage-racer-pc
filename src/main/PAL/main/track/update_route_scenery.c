#include "game/render.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

void SeedRouteScenery(void) {
    const s32 series = g_RaceSeries;
    const s16 firstKeyframe =
        g_RouteSceneryData->firstKeyframe[series][0];
    const SceneryMotionKeyframe *keyframe =
        &g_RouteSceneryData->keyframes[firstKeyframe];

    g_RouteSceneryKeyIndex = 0;
    g_RouteSceneryKeyframe = keyframe;
    g_RouteSceneryRotX = keyframe->rotationX;
    g_RouteSceneryRotY = keyframe->rotationY;
    g_RouteSceneryRotZ = keyframe->rotationZ;
    g_RouteSceneryPosition = g_RouteSceneryData->start[series].position;
    g_RouteSceneryActive = 1;
    g_RouteSceneryFrame = 0;
}

void UpdateRouteScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    SVec vin;
    LVec vout;
    const SceneryMotionKeyframe *keyframe;
    s32 elapsed;

    if (g_RouteSceneryActive <= 0) {
        return;
    }
    g_RouteSceneryFrame = WrapSigned32(
        (int64_t)g_RouteSceneryFrame + 1);

    keyframe = &g_RouteSceneryKeyframe[g_RouteSceneryKeyIndex];
    if (keyframe->duration == g_RouteSceneryFrame) {
        g_RouteSceneryKeyIndex++;
        g_RouteSceneryFrame = 0;
        keyframe++;
    }

    if (keyframe->duration == SCENERY_MOTION_END) {
        SeedRouteScenery();
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
    vin.vz = WrapSigned16(-(int64_t)keyframe->speed * 4);
    BuildRotMatrixY(
        &mtx0,
        WrapSigned32((int64_t)0x800 - g_RouteSceneryRotY));

    BuildRotMatrixX(&mtx1, g_RouteSceneryRotX);
    MulMatrix2(&mtx0, &mtx1);
    BuildRotMatrixZ(&mtx0, g_RouteSceneryRotZ);
    MulMatrix(&mtx1, &mtx0);
    ApplyMatrix(&mtx1, &vin, &vout);

    g_RouteSceneryX = WrapSigned32(
        (int64_t)g_RouteSceneryX + vout.x / 4);
    g_RouteSceneryY = WrapSigned32(
        (int64_t)g_RouteSceneryY + vout.y / 4);
    g_RouteSceneryZ = WrapSigned32(
        (int64_t)g_RouteSceneryZ + vout.z / 4);
}
