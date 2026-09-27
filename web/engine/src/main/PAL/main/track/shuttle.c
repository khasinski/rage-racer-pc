#include "game/shuttle_scenery.h"
#include "game/integer.h"

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

int StepShuttle(GameShuttleScenery *state, const ShuttlePath *path, s32 travelDuration, s32 dwell) {
    if (!state || !path || travelDuration <= 0 ||
        (u32)state->startEndpoint >= SHUTTLE_ENDPOINT_COUNT) return 0;
    s32 step;
    const Vec4 *from, *to;
    step = state->travelStep;
    if (step < 0) {
        step = 0;
        state->travelStep = 0;
    } else if (step > travelDuration) {
        step = travelDuration;
    }
    from = &path->endpoint[state->startEndpoint];
    to = &path->endpoint[1 - state->startEndpoint];

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
    } else if (state->dwellCounter >= dwell) {
        state->travelStep++;
        state->dwellCounter = dwell;
    } else {
        state->dwellCounter++;
    }
    return 1;
}

int InitShuttle(GameShuttleScenery *state, const ShuttlePath *path, const SVec *angles,
                s32 pathIndex, s32 dwell) {
    if (!state || !path || !angles || (u32)pathIndex >= SHUTTLE_PATH_COUNT) return 0;
    *state = (GameShuttleScenery){.pathIndex = (s16)pathIndex,
        .position = path->endpoint[0], .angleX = angles->vx, .angleY = angles->vy,
        .angleZ = angles->vz, .dwellCounter = dwell};
    return 1;
}
