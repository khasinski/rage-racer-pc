#ifndef PORT_SCENE_MATRIX_H
#define PORT_SCENE_MATRIX_H
#include <math.h>
#include "game/angle.h"
#include "render/render_world.h"

static inline float AngleToDegrees(s32 angle) {
    return (float)(angle & ANGLE_MASK) * (360.0f / 4096.0f);
}

typedef struct SceneMat3 {
    float m[3][3];
} SceneMat3;

static inline SceneMat3 SceneMat3Multiply(SceneMat3 a, SceneMat3 b) {
    SceneMat3 out = {{{0}}};
    int row, column, i;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++)
            for (i = 0; i < 3; i++) out.m[row][column] += a.m[row][i] * b.m[i][column];
    return out;
}

static inline SceneMat3 SceneMat3Transpose(SceneMat3 source) {
    SceneMat3 out;
    int row, column;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++) out.m[row][column] = source.m[column][row];
    return out;
}

static inline SceneMat3 SceneRotationX(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    SceneMat3 out = {{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
    return out;
}

static inline SceneMat3 SceneRotationY(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    /* This is the game's BuildRotMatrixY convention, not a generic
     * right-handed Euler helper.  The PS1->scene basis conversion below
     * turns it into the renderer's conventional rotation. */
    SceneMat3 out = {{{c, 0, -s}, {0, 1, 0}, {s, 0, c}}};
    return out;
}

static inline SceneMat3 SceneRotationZ(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    SceneMat3 out = {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
    return out;
}

static inline Quaternion SceneQuaternion(SceneMat3 source) {
    Quaternion out;
    float (*m)[3] = source.m;
    float trace, root;
    trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        root = sqrtf(trace + 1.0f) * 2.0f;
        out.w = 0.25f * root;
        out.x = (m[2][1] - m[1][2]) / root;
        out.y = (m[0][2] - m[2][0]) / root;
        out.z = (m[1][0] - m[0][1]) / root;
    } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        root = sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        out.w = (m[2][1] - m[1][2]) / root;
        out.x = 0.25f * root;
        out.y = (m[0][1] + m[1][0]) / root;
        out.z = (m[0][2] + m[2][0]) / root;
    } else if (m[1][1] > m[2][2]) {
        root = sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        out.w = (m[0][2] - m[2][0]) / root;
        out.x = (m[0][1] + m[1][0]) / root;
        out.y = 0.25f * root;
        out.z = (m[1][2] + m[2][1]) / root;
    } else {
        root = sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        out.w = (m[1][0] - m[0][1]) / root;
        out.x = (m[0][2] + m[2][0]) / root;
        out.y = (m[1][2] + m[2][1]) / root;
        out.z = 0.25f * root;
    }
    return out;
}

static inline Quaternion SceneQuaternionFromPsx(SceneMat3 source) {
    SceneMat3 converted;
    RenderConvertPsxMatrix(source.m, converted.m);
    return SceneQuaternion(converted);
}

static inline Vec3 SceneRotatePoint(SceneMat3 matrix,
                                            float x, float y, float z) {
    Vec3 out;
    out.x = matrix.m[0][0] * x + matrix.m[0][1] * y + matrix.m[0][2] * z;
    out.y = matrix.m[1][0] * x + matrix.m[1][1] * y + matrix.m[1][2] * z;
    out.z = matrix.m[2][0] * x + matrix.m[2][1] * y + matrix.m[2][2] * z;
    return out;
}

#endif
