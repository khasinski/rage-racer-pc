#include "game/render.h"

void BuildAxisRotMatrix(GameRenderAxisMatrix *out, s32 sinTerm, s32 cosTerm, s32 axisMode) {
    s32 one;

    switch (((u8)axisMode) - 0x58) {
    case 0:   /* X-axis rotation */
    case 32:
        one = 0x1000;
        out->m[0][0] = one;
        one = -sinTerm;
        out->m[0][1] = 0;
        out->m[0][2] = 0;
        out->m[1][0] = 0;
        out->m[1][1] = cosTerm;
        out->m[1][2] = one;
        out->m[2][0] = 0;
        out->m[2][1] = sinTerm;
        out->m[2][2] = cosTerm;
        break;

    case 1:   /* Y-axis rotation */
    case 33:
        one = 0x1000;
        out->m[1][1] = one;
        one = -sinTerm;
        out->m[0][0] = cosTerm;
        out->m[0][1] = 0;
        out->m[0][2] = sinTerm;
        out->m[1][0] = 0;
        out->m[1][2] = 0;
        out->m[2][0] = one;
        out->m[2][1] = 0;
        out->m[2][2] = cosTerm;
        break;

    case 2:   /* Z-axis rotation */
    case 34:
        out->m[0][1] = -sinTerm;
        out->m[0][0] = cosTerm;
        out->m[0][2] = 0;
        out->m[1][0] = sinTerm;
        out->m[1][1] = cosTerm;
        out->m[1][2] = 0;
        out->m[2][0] = 0;
        out->m[2][1] = 0;
        out->m[2][2] = 0x1000;
        break;
    }
}


/*
 * Builds a billboard / look-at view Matrix from an eye and target point.
 * `len` is the distance (SquareRoot0 sqrt); computes a pitch and a yaw axis
 * rotation (BuildAxisRotMatrix), the translation (MatrixApplyVectorComponents), then per-row
 * fixed-point projection scaling (<<1 / <<2). Returns 1 if eye==target, else 0.
 */
s32 SetLookAtMatrix(const CameraLookAt *camera) {
    Matrix m;
    GameRenderAxisMatrix am;
    s32 outX;
    s32 outY;
    s32 outZ;
    s32 len;
    s32 horiz;
    s32 pitch;

    m.m[0][0] = 0x1000;
    m.m[0][1] = 0;
    m.m[0][2] = 0;
    m.m[1][0] = 0;
    m.m[1][1] = 0x1000;
    m.m[1][2] = 0;
    m.m[2][0] = 0;
    m.m[2][1] = 0;
    m.m[2][2] = 0x1000;
    MatrixApplyZRotation(&m, 0);

    len = SquareRoot0((camera->fields.targetX - camera->fields.eyeX) *
                          (camera->fields.targetX - camera->fields.eyeX) +
                      (camera->fields.targetY - camera->fields.eyeY) *
                          (camera->fields.targetY - camera->fields.eyeY) +
                      (camera->fields.targetZ - camera->fields.eyeZ) *
                          (camera->fields.targetZ - camera->fields.eyeZ));
    if (len == 0) {
        return 1;
    }

    horiz = camera->fields.eyeY - camera->fields.targetY;
    pitch = (horiz << 12) / len;
    pitch = -pitch;
    horiz = SquareRoot0((camera->fields.targetX - camera->fields.eyeX) *
                            (camera->fields.targetX - camera->fields.eyeX) +
                        (camera->fields.targetZ - camera->fields.eyeZ) *
                            (camera->fields.targetZ - camera->fields.eyeZ));
    BuildAxisRotMatrix(&am, (s16)pitch, (s16)((horiz << 12) / len), 0x78);
    MulMatrix(&m, &am);

    if (horiz != 0) {
        s32 t1;
        s32 t2;

        len = horiz;
        horiz = camera->fields.targetX - camera->fields.eyeX;
        t1 = (horiz << 12) / len;
        horiz = camera->fields.targetZ - camera->fields.eyeZ;
        t2 = (horiz << 12) / len;
        BuildAxisRotMatrix(&am, (s16)(-t1), (s16)t2, 0x79);
        MulMatrix(&m, &am);
    }

    MatrixApplyVectorComponents(&m, -camera->fields.eyeX, -camera->fields.eyeY,
                                -camera->fields.eyeZ, &outX, &outY, &outZ);
    m.t[0] = outX;
    m.t[1] = outY;
    m.t[2] = outZ;
    m.m[0][0] <<= 1;
    m.m[0][1] <<= 1;
    m.m[0][2] <<= 1;
    m.m[1][0] <<= 2;
    m.m[1][1] <<= 2;
    m.m[1][2] <<= 2;
    m.m[2][0] <<= 1;
    m.m[2][1] <<= 1;
    m.m[2][2] <<= 1;
    m.t[0] <<= 1;
    m.t[1] <<= 1;
    m.t[2] <<= 1;
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    return 0;
}

// Fixed-point blend in 0..10000 scale.
s32 BezierEase(s32 t, s32 control) {
    s32 initial;
    s32 value;
    s32 doubled;

    initial = 0x2710 - t;
    value = initial;
    doubled = t * 2;
    value = (value * doubled) / 10000;
    return ((value * control) + (t * t)) / 10000;
}
