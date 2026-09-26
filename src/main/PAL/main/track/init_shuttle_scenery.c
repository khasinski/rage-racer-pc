#include "game/race.h"
#include "game/track.h"

static void InitializeShuttle(GameShuttleScenery *state, s32 pathIndex) {
    InitShuttle(state, &g_ShuttlePathPoints[pathIndex],
                &g_ShuttlePathAngles[pathIndex], pathIndex, g_ShuttlePathDwellMax[pathIndex]);
}

void InitShuttleScenery(void) {
    s32 firstPath = 0;
    if (SeriesCourseIndex() == 2) {
        firstPath = 1;
        InitializeShuttle(&g_ShuttleScenery[1], 2);
    }
    InitializeShuttle(&g_ShuttleScenery[0], firstPath);
}
