#include "game/track_internal.h"

void UpdateShuttleScenery(s32 instance) {
    if ((u32)instance >= SHUTTLE_INSTANCE_COUNT) return;
    GameShuttleScenery *state = &g_ShuttleScenery[instance];
    const s32 path = state->pathIndex;
    if ((u32)path >= SHUTTLE_PATH_COUNT) return;
    StepShuttle(state, &g_ShuttlePathPoints[path],
                g_ShuttlePathTravelMax[path], g_ShuttlePathDwellMax[path]);
}
