#include "game/race.h"
#include "game/track_internal.h"

void InitPathScenery(void) {
    const PathSceneryPositionKey *positionKeys =
        &g_PathSceneryPosData->keys[
            g_PathSceneryPosData->firstKey[g_RaceSeries]];
    const PathSceneryRotationKey *rotationKeys =
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
        NormalizePathSceneryRate(positionKeys[0].fields.rate);
    g_PathSceneryCursors.rotRate.value =
        NormalizePathSceneryRate(rotationKeys[0].fields.rate);
    g_PathSceneryCursors.posIndex = 0;
    g_PathSceneryCursors.rotIndex = 0;
    g_PathSceneryVolume = 0;

    for (axis = 0; axis < 3; axis++) {
        g_PathSceneryHalfDelta[axis] =
            PathSceneryHalfDelta(positionKeys[0].position.w[axis],
                                 positionKeys[1].position.w[axis]);
    }
    g_PathSceneryRotHalfDelta[0] =
        (s16)((rotationKeys[1].fields.x - rotationKeys[0].fields.x) / 2);
    g_PathSceneryRotHalfDelta[1] =
        (s16)((rotationKeys[1].fields.y - rotationKeys[0].fields.y) / 2);
    g_PathSceneryRotHalfDelta[2] =
        (s16)((rotationKeys[1].fields.z - rotationKeys[0].fields.z) / 2);
}
