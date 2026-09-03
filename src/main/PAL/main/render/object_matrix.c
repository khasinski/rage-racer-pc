#include "game/diagnostics.h"
#include "game/render.h"
#include "game/state.h"

/* Converts a world-space object position to the camera-relative GTE matrix. */
void SetGteObjectMatrix(ObjectMatrixWork *work, LVec *position,
                        Matrix *rotation) {
    work->relative[0] = position->x - RENDER_VIEW_STATE->position.vector.x;
    work->relative[1] = position->y - RENDER_VIEW_STATE->position.vector.y;
    work->relative[2] = position->z - RENDER_VIEW_STATE->position.vector.z;
    ApplyMatrix(&g_RenderState.matrix, work->relative, &work->view);
    work->mtx.t[0] = work->view.x * 4;
    work->mtx.t[1] = work->view.y * 4;
    work->mtx.t[2] = work->view.z * 4;
    SetRotMatrix(rotation);
    SetTransMatrix(&work->mtx);

    if (DiagnosticsEnabled("render.car_draw_trace") &&
        g_RenderState.mode == GAME_RENDER_PASS_MIRROR) {
        if (g_SceneTimer == DiagnosticsIntValue(
                "render.car_draw_trace_timer", g_SceneTimer)) {
            Trace("object-matrix", "timer=%d position=%d,%d,%d relative=%d,%d,%d "
                  "view=%d,%d,%d rotation=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                  "translation=%d,%d,%d", g_SceneTimer,
                  position->x, position->y, position->z, work->relative[0],
                  work->relative[1], work->relative[2], work->view.x,
                  work->view.y, work->view.z, rotation->m[0][0],
                  rotation->m[0][1], rotation->m[0][2], rotation->m[1][0],
                  rotation->m[1][1], rotation->m[1][2], rotation->m[2][0],
                  rotation->m[2][1], rotation->m[2][2], work->mtx.t[0],
                  work->mtx.t[1], work->mtx.t[2]);
        }
    }
}
