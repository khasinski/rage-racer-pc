#include "game/race.h"
#include "game/track_internal.h"

static s16 NormalizePathRate(s16 rate, s16 *isNegative) {
    *isNegative = rate < 0;
    if (rate < 0) {
        return (s16)-rate;
    }
    return rate == 0 ? 1 : rate;
}

void InitPathScenery(void) {
    PathSceneryPositionKey *positionKeys =
        &g_PathSceneryPosData->keys[
            g_PathSceneryPosData->firstKey[g_RaceSeries]];
    PathSceneryRotationKey *rotationKeys =
        &g_PathSceneryRotData->keys[
            g_PathSceneryRotData->firstKey[g_RaceSeries]];
    int axis;

    g_PathSceneryPosKeys = positionKeys;
    g_PathSceneryRotKeys = rotationKeys;
    g_PathSceneryClock.posFrame = 0;
    g_PathSceneryClock.rotFrame = 0;
    g_PathSceneryTransform.position = positionKeys[0].position;
    g_PathSceneryTransform.rotation = rotationKeys[0].rotation;

    g_PathSceneryCursors.posPhase.value = 0;
    g_PathSceneryCursors.rotPhase.value = 0;
    g_PathSceneryCursors.posSpan = positionKeys[0].fields.span;
    g_PathSceneryCursors.rotSpan = rotationKeys[0].fields.span;
    g_PathSceneryCursors.posRate.value =
        NormalizePathRate(positionKeys[0].fields.rate,
                          &g_PathSceneryClock.posRateNeg);
    g_PathSceneryCursors.rotRate.value =
        NormalizePathRate(rotationKeys[0].fields.rate,
                          &g_PathSceneryClock.rotRateNeg);
    g_PathSceneryCursors.posIndex = 0;
    g_PathSceneryCursors.rotIndex = 0;
    g_PathSceneryVolume = 0;

    for (axis = 0; axis < 3; axis++) {
        g_PathSceneryHalfDelta[axis] =
            (s16)((positionKeys[1].position.w[axis] -
                   positionKeys[0].position.w[axis]) / 2);
    }
    g_PathSceneryRotHalfDelta[0] =
        (s16)((rotationKeys[1].fields.x - rotationKeys[0].fields.x) / 2);
    g_PathSceneryRotHalfDelta[1] =
        (s16)((rotationKeys[1].fields.y - rotationKeys[0].fields.y) / 2);
    g_PathSceneryRotHalfDelta[2] =
        (s16)((rotationKeys[1].fields.z - rotationKeys[0].fields.z) / 2);
}
