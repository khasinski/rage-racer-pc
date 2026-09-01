#include "common.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

s32 g_CourseIndex;
ShuttlePath g_ShuttlePathPoints[3];
SVec g_ShuttlePathAngles[3];
s16 g_ShuttlePathDwellMax[3];
GameShuttleScenery g_ShuttleScenery[2];

static int CheckShuttle(s32 instance, s32 pathIndex) {
    const GameShuttleScenery *state = &g_ShuttleScenery[instance];

    return state->pathIndex == pathIndex &&
           state->position.x == 1000 + pathIndex &&
           state->position.y == 2000 + pathIndex &&
           state->position.z == 3000 + pathIndex &&
           state->position.w == 4000 + pathIndex &&
           state->angleX == 10 + pathIndex &&
           state->angleY == 20 + pathIndex &&
           state->angleZ == 30 + pathIndex &&
           state->startEndpoint == 0 &&
           state->travelStep == 0 &&
           state->dwellCounter == 40 + pathIndex;
}

static void SeedFixtures(void) {
    s32 path;

    for (path = 0; path < 3; path++) {
        g_ShuttlePathPoints[path].endpoint[0] =
            (Vec4){1000 + path, 2000 + path, 3000 + path, 4000 + path};
        g_ShuttlePathAngles[path] =
            (SVec){10 + path, 20 + path, 30 + path, 0};
        g_ShuttlePathDwellMax[path] = (s16)(40 + path);
    }
}

int main(void) {
    GameShuttleScenery untouched;

    SeedFixtures();
    memset(g_ShuttleScenery, 0x5A, sizeof(g_ShuttleScenery));
    untouched = g_ShuttleScenery[1];
    g_CourseIndex = 0;
    InitShuttleScenery();
    if (!CheckShuttle(0, 0) ||
        memcmp(&g_ShuttleScenery[1], &untouched, sizeof(untouched)) != 0) {
        puts("FAIL: single-shuttle course initialization");
        return 1;
    }

    memset(g_ShuttleScenery, 0x5A, sizeof(g_ShuttleScenery));
    g_CourseIndex = 2;
    InitShuttleScenery();
    if (!CheckShuttle(0, 1) || !CheckShuttle(1, 2)) {
        puts("FAIL: Lakeside Gate shuttle initialization");
        return 1;
    }

    puts("shuttle scenery initialization preserved");
    return 0;
}
