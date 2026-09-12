#include "game/render.h"

#include <limits.h>

static s16 SubtractPositionComponent(s32 position, s32 camera) {
    u16 lowBits = (u16)((u32)position - (u32)camera);

    return lowBits <= INT16_MAX
        ? (s16)lowBits
        : (s16)((s32)lowBits - 0x10000);
}

/* Converts a world-space object position to the camera-relative GTE matrix. */
void SetGteObjectMatrix(const LVec *position, Matrix *rotation) {
    SVec relative;
    LVec view;
    Matrix translation;

    relative.vx = SubtractPositionComponent(
        position->x, g_RenderState.camera.x);
    relative.vy = SubtractPositionComponent(
        position->y, g_RenderState.camera.y);
    relative.vz = SubtractPositionComponent(
        position->z, g_RenderState.camera.z);
    ApplyMatrix(&g_RenderState.geometry.matrix, &relative, &view);
    translation.t[0] = view.x * 4;
    translation.t[1] = view.y * 4;
    translation.t[2] = view.z * 4;
    SetRotMatrix(rotation);
    SetTransMatrix(&translation);

}
