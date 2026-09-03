#include "game/track_internal.h"

static s32 InterpolateCoordinate(s32 from, s32 to, s32 step,
                                 s32 travelDuration) {
    const s32 remaining = WrapSigned32(
        (int64_t)travelDuration - step);
    const s32 fromContribution = WrapSigned32(
        (int64_t)remaining * from);
    const s32 toContribution = WrapSigned32((int64_t)step * to);
    const s32 total = WrapSigned32(
        (int64_t)fromContribution + toContribution);

    return total / travelDuration;
}

void UpdateShuttleScenery(s32 instance) {
    GameShuttleScenery *state;
    s32 pathIndex;
    s32 step;
    s32 travelDuration;
    const Vec4 *from;
    const Vec4 *to;

    if (instance < 0 || instance >= SHUTTLE_INSTANCE_COUNT) {
        return;
    }
    state = &g_ShuttleScenery[instance];
    pathIndex = state->pathIndex;
    if (pathIndex < 0 || pathIndex >= SHUTTLE_PATH_COUNT ||
        state->startEndpoint < 0 ||
        state->startEndpoint >= SHUTTLE_ENDPOINT_COUNT) {
        return;
    }

    step = state->travelStep;
    travelDuration = g_ShuttlePathTravelMax[pathIndex];
    if (travelDuration <= 0) {
        return;
    }
    from = &g_ShuttlePathPoints[pathIndex].endpoint[state->startEndpoint];
    to = &g_ShuttlePathPoints[pathIndex]
              .endpoint[1 - state->startEndpoint];

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
