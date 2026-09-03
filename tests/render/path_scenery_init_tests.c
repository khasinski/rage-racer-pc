#include "common.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
const PathSceneryPositionData *g_PathSceneryPosData;
const PathSceneryRotationData *g_PathSceneryRotData;
const PathSceneryPositionKey *g_PathSceneryPosKeys;
const PathSceneryRotationKey *g_PathSceneryRotKeys;
PathSceneryClock g_PathSceneryClock;
PathSceneryTransform g_PathSceneryTransform;
PathSceneryCursors g_PathSceneryCursors;
s16 g_PathSceneryHalfDelta[3];
s16 g_PathSceneryRotHalfDelta[3];
s32 g_PathSceneryVolume;

typedef struct PositionFixture {
    s16 firstKey[2];
    PathSceneryPositionKey keys[3];
} PositionFixture;

typedef struct RotationFixture {
    s16 firstKey[2];
    PathSceneryRotationKey keys[3];
} RotationFixture;

static int RunCase(s16 positionRate, s16 rotationRate,
                   s16 expectedPositionRate, s16 expectedRotationRate) {
    PositionFixture positions;
    RotationFixture rotations;
    static const s16 expectedDelta[3] = {2, -4, 4};
    int axis;

    memset(&positions, 0, sizeof(positions));
    memset(&rotations, 0, sizeof(rotations));
    memset(&g_PathSceneryClock, 0x7F, sizeof(g_PathSceneryClock));
    memset(&g_PathSceneryCursors, 0x7F, sizeof(g_PathSceneryCursors));

    positions.firstKey[1] = 1;
    positions.keys[1].fields.x = 10;
    positions.keys[1].fields.y = -20;
    positions.keys[1].fields.z = 31;
    positions.keys[1].fields.span = 12;
    positions.keys[1].fields.rate = positionRate;
    positions.keys[2].fields.x = 15;
    positions.keys[2].fields.y = -29;
    positions.keys[2].fields.z = 40;

    rotations.firstKey[1] = 1;
    rotations.keys[1].fields.x = 100;
    rotations.keys[1].fields.y = -200;
    rotations.keys[1].fields.z = 301;
    rotations.keys[1].fields.span = 9;
    rotations.keys[1].fields.rate = rotationRate;
    rotations.keys[2].fields.x = 105;
    rotations.keys[2].fields.y = -209;
    rotations.keys[2].fields.z = 310;

    g_RaceSeries = 7;
    g_PathSceneryPosData = (PathSceneryPositionData *)&positions;
    g_PathSceneryRotData = (PathSceneryRotationData *)&rotations;
    g_PathSceneryVolume = 123;
    InitPathScenery();

    if (g_PathSceneryPosKeys != &positions.keys[1] ||
        g_PathSceneryRotKeys != &rotations.keys[1] ||
        g_PathSceneryClock.posFrame != 0 ||
        g_PathSceneryClock.rotFrame != 0 ||
        g_PathSceneryCursors.posPhase.value != 0 ||
        g_PathSceneryCursors.rotPhase.value != 0 ||
        g_PathSceneryCursors.posSpan != 12 ||
        g_PathSceneryCursors.rotSpan != 9 ||
        g_PathSceneryCursors.posRate.value != expectedPositionRate ||
        g_PathSceneryCursors.rotRate.value != expectedRotationRate ||
        g_PathSceneryCursors.posIndex != 0 ||
        g_PathSceneryCursors.rotIndex != 0 ||
        g_PathSceneryVolume != 0) {
        puts("FAIL: path scenery cursor initialization");
        return 0;
    }

    if (g_PathSceneryTransform.position.w[0] != 10 ||
        g_PathSceneryTransform.position.w[1] != -20 ||
        g_PathSceneryTransform.position.w[2] != 31 ||
        g_PathSceneryTransform.rotation.vx != 100 ||
        g_PathSceneryTransform.rotation.vy != -200 ||
        g_PathSceneryTransform.rotation.vz != 301) {
        puts("FAIL: initial path scenery transform");
        return 0;
    }

    for (axis = 0; axis < 3; axis++) {
        if (g_PathSceneryHalfDelta[axis] != expectedDelta[axis] ||
            g_PathSceneryRotHalfDelta[axis] != expectedDelta[axis]) {
            puts("FAIL: path scenery half delta");
            return 0;
        }
    }
    return 1;
}

int main(void) {
    if (!RunCase(5, 6, 5, 6) ||
        !RunCase(0, 0, 1, 1) ||
        !RunCase(-7, -8, 7, 8) ||
        !RunCase(-32767 - 1, -32767 - 1, 32767, 32767)) {
        return 1;
    }
    puts("path scenery initialization preserved");
    return 0;
}
