#include "game/diagnostics.h"
#include "game/render.h"
#include "game/state.h"

#include <limits.h>

static s16 SubtractPositionComponent(s32 position, s32 camera) {
    u16 lowBits = (u16)((u32)position - (u32)camera);

    return lowBits <= INT16_MAX
        ? (s16)lowBits
        : (s16)((s32)lowBits - 0x10000);
}

/* Converts a world-space object position to the camera-relative GTE matrix. */
void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    SVec relative;
    LVec view;
    Matrix translation;

    relative.vx = SubtractPositionComponent(
        position->x, RENDER_VIEW_STATE->position.vector.x);
    relative.vy = SubtractPositionComponent(
        position->y, RENDER_VIEW_STATE->position.vector.y);
    relative.vz = SubtractPositionComponent(
        position->z, RENDER_VIEW_STATE->position.vector.z);
    ApplyMatrix(&g_RenderState.matrix, &relative, &view);
    translation.t[0] = view.x * 4;
    translation.t[1] = view.y * 4;
    translation.t[2] = view.z * 4;
    SetRotMatrix(rotation);
    SetTransMatrix(&translation);

    if (DiagnosticsEnabled("render.car_draw_trace") &&
        g_RenderState.mode == GAME_RENDER_PASS_MIRROR) {
        if (g_SceneTimer == DiagnosticsIntValue(
                "render.car_draw_trace_timer", g_SceneTimer)) {
            Trace("object-matrix", "timer=%d position=%d,%d,%d relative=%d,%d,%d "
                  "view=%d,%d,%d rotation=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                  "translation=%d,%d,%d", g_SceneTimer,
                  position->x, position->y, position->z, relative.vx,
                  relative.vy, relative.vz, view.x, view.y, view.z,
                  rotation->m[0][0],
                  rotation->m[0][1], rotation->m[0][2], rotation->m[1][0],
                  rotation->m[1][1], rotation->m[1][2], rotation->m[2][0],
                  rotation->m[2][1], rotation->m[2][2], translation.t[0],
                  translation.t[1], translation.t[2]);
        }
    }
}
