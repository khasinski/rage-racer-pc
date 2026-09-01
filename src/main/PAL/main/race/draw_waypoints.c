#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

/* Each waypoint is submitted twice, with the second face turned 180 degrees,
 * so the marker remains visible from either direction along the track. */
void DrawWaypoints(void) {
    s32 modelIndex;
    s32 index;

    SelectModelBank(0);
    modelIndex = g_ModelBankCount > 2 ? 2 : 1;

    for (index = 0; index < 6; index++) {
        TrackWaypointRuntime *waypoint = &g_Waypoints[index];
        Matrix waypointMatrix;
        Matrix rotationMatrix;

        BuildRotMatrixY(&waypointMatrix, waypoint->motion.rotationY);
        MulMatrix2(&g_RenderState.matrix, &waypointMatrix);
        BuildRotMatrixZ(&rotationMatrix, waypoint->motion.rotationZ);
        MulMatrix(&waypointMatrix, &rotationMatrix);
        SetGteObjectMatrix(&g_ObjectMatrixWork,
                           AsPositionWords(&waypoint->motion.x),
                           &waypointMatrix);
        g_RenderState.envMode4 = 0;
        SubmitModel(&g_RenderState, modelIndex);

        BuildRotMatrixY(&rotationMatrix, 0x800);
        MulMatrix2(&waypointMatrix, &rotationMatrix);
        SetGteObjectMatrix(&g_ObjectMatrixWork,
                           AsPositionWords(&waypoint->motion.x),
                           &rotationMatrix);
        g_RenderState.envMode4 = 0;
        SubmitModel(&g_RenderState, modelIndex);
    }
}
