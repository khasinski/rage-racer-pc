#include "game/race.h"
#include "game/track.h"

static void InitializeShuttle(GameShuttleScenery *state, s32 pathIndex) {
    const SVec *angles = &g_ShuttlePathAngles[pathIndex];

    state->pathIndex = (s16)pathIndex;
    state->position = g_ShuttlePathPoints[pathIndex].endpoint[0];
    state->angleX = angles->vx;
    state->angleY = angles->vy;
    state->angleZ = angles->vz;
    state->startEndpoint = 0;
    state->travelStep = 0;
    state->dwellCounter = g_ShuttlePathDwellMax[pathIndex];
}

void InitShuttleScenery(void) {
    s32 firstPath = 0;

    if (SeriesCourseIndex() == 2) {
        firstPath = 1;
        InitializeShuttle(&g_ShuttleScenery[1], 2);
    }
    InitializeShuttle(&g_ShuttleScenery[0], firstPath);
}
