#include "game/track.h"

static s32 InterpolateCoordinate(s32 from, s32 to, s32 step,
                                 s32 travelDuration) {
    return ((travelDuration - step) * from + step * to) / travelDuration;
}

void UpdateShuttleScenery(s32 instance) {
    GameShuttleScenery *state = &g_ShuttleScenery[instance];
    s32 pathIndex = state->pathIndex;
    s32 step = state->travelStep;
    s32 travelDuration = g_ShuttlePathTravelMax[pathIndex];
    const Vec4 *from =
        &g_ShuttlePathPoints[pathIndex].endpoint[state->startEndpoint];
    const Vec4 *to =
        &g_ShuttlePathPoints[pathIndex].endpoint[1 - state->startEndpoint];

    state->position.x =
        InterpolateCoordinate(from->x, to->x, step, travelDuration);
    state->position.y =
        InterpolateCoordinate(from->y, to->y, step, travelDuration);
    state->position.z =
        InterpolateCoordinate(from->z, to->z, step, travelDuration);

    if (step >= travelDuration) {
        state->travelStep = 0;
        state->dwellCounter = 0;
        state->startEndpoint ^= 1;
    } else if (state->dwellCounter >= g_ShuttlePathDwellMax[pathIndex]) {
        state->travelStep++;
        state->dwellCounter = g_ShuttlePathDwellMax[pathIndex];
    } else {
        state->dwellCounter++;
    }
}
