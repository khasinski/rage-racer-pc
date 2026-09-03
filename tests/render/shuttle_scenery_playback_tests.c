#include "common.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

ShuttlePath g_ShuttlePathPoints[3];
s16 g_ShuttlePathTravelMax[3];
s16 g_ShuttlePathDwellMax[3];
GameShuttleScenery g_ShuttleScenery[2];

static int ExpectState(const char *label, s32 x, s32 y, s32 z,
                       s32 endpoint, s32 step, s32 dwell) {
    const GameShuttleScenery *state = &g_ShuttleScenery[0];

    if (state->position.x != x || state->position.y != y ||
        state->position.z != z || state->position.w != 777 ||
        state->startEndpoint != endpoint || state->travelStep != step ||
        state->dwellCounter != dwell) {
        printf("FAIL %s: pos=(%d,%d,%d,%d) endpoint=%d step=%d dwell=%d\n",
               label, state->position.x, state->position.y,
               state->position.z, state->position.w,
               state->startEndpoint, state->travelStep,
               state->dwellCounter);
        return 0;
    }
    return 1;
}

int main(void) {
    GameShuttleScenery *state = &g_ShuttleScenery[0];
    GameShuttleScenery snapshot[SHUTTLE_INSTANCE_COUNT];

    memset(g_ShuttleScenery, 0, sizeof(g_ShuttleScenery));
    g_ShuttlePathPoints[0].endpoint[0] = (Vec4){0, 10, 20, 30};
    g_ShuttlePathPoints[0].endpoint[1] = (Vec4){100, 210, 320, 430};
    g_ShuttlePathTravelMax[0] = 2;
    g_ShuttlePathDwellMax[0] = 1;
    state->pathIndex = 0;
    state->position.w = 777;
    state->dwellCounter = 1;

    UpdateShuttleScenery(0);
    if (!ExpectState("depart", 0, 10, 20, 0, 1, 1)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("midpoint", 50, 110, 170, 0, 2, 1)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("arrive", 100, 210, 320, 1, 0, 0)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("dwell", 100, 210, 320, 1, 0, 1)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("return depart", 100, 210, 320, 1, 1, 1)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("return midpoint", 50, 110, 170, 1, 2, 1)) return 1;
    UpdateShuttleScenery(0);
    if (!ExpectState("return arrive", 0, 10, 20, 0, 0, 0)) return 1;

    memcpy(snapshot, g_ShuttleScenery, sizeof(snapshot));
    UpdateShuttleScenery(-1);
    UpdateShuttleScenery(SHUTTLE_INSTANCE_COUNT);
    if (memcmp(snapshot, g_ShuttleScenery, sizeof(snapshot)) != 0) {
        puts("FAIL: invalid shuttle instance changed state");
        return 1;
    }

    state->pathIndex = SHUTTLE_PATH_COUNT;
    UpdateShuttleScenery(0);
    state->pathIndex = 0;
    state->startEndpoint = SHUTTLE_ENDPOINT_COUNT;
    UpdateShuttleScenery(0);
    state->startEndpoint = 0;
    g_ShuttlePathTravelMax[0] = 0;
    UpdateShuttleScenery(0);
    if (state->position.x != 0 || state->position.y != 10 ||
        state->position.z != 20) {
        puts("FAIL: invalid shuttle path data changed position");
        return 1;
    }

    g_ShuttlePathPoints[0].endpoint[0] =
        (Vec4){INT_MAX, INT_MAX, INT_MAX, 0};
    g_ShuttlePathPoints[0].endpoint[1] =
        (Vec4){INT_MIN, INT_MIN, INT_MIN, 0};
    g_ShuttlePathTravelMax[0] = 2;
    state->travelStep = 1;
    UpdateShuttleScenery(0);
    if (state->position.x != 0 || state->position.y != 0 ||
        state->position.z != 0) {
        puts("FAIL: shuttle interpolation did not preserve 32-bit wrapping");
        return 1;
    }

    puts("shuttle scenery playback preserved");
    return 0;
}
