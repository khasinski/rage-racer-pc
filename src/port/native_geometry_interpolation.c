#include "native_geometry_interpolation.h"

#include <math.h>

int FloorShift12(int64_t value) {
    if (value >= 0) return (int)(value >> 12);
    return -(int)((-value + 0xfff) >> 12);
}

int IntplComponent(int start, int end, int index, int steps) {
    int ir0 = 4096 - index * (4096 / steps);
    int difference = start - end;
    int result;
    if (difference < -32768) difference = -32768;
    if (difference > 32767) difference = 32767;
    result = FloorShift12((int64_t)end * 4096 +
                              (int64_t)ir0 * difference);
    if (result < -32768) result = -32768;
    if (result > 32767) result = 32767;
    return result;
}

int BilerpSxy(const int sxy[4], int u, int v, int uSteps, int vSteps) {
    int topX = IntplComponent((int16_t)sxy[0], (int16_t)sxy[1], u,
                                  uSteps);
    int bottomX = IntplComponent((int16_t)sxy[2], (int16_t)sxy[3], u,
                                     uSteps);
    int topY = IntplComponent((int16_t)(sxy[0] >> 16),
                                  (int16_t)(sxy[1] >> 16), u, uSteps);
    int bottomY = IntplComponent((int16_t)(sxy[2] >> 16),
                                     (int16_t)(sxy[3] >> 16), u, uSteps);
    int x = IntplComponent(topX, bottomX, v, vSteps);
    int y = IntplComponent(topY, bottomY, v, vSteps);
    return (int)((uint16_t)x | ((uint32_t)(uint16_t)y << 16));
}

SVECTOR BilerpVertex(const SVECTOR *v0, const SVECTOR *v1,
                         const SVECTOR *v2, const SVECTOR *v3, int outer,
                         int inner, int outerSteps, int innerSteps) {
    SVECTOR result;
#define RAGE_BILERP_VERTEX_COMPONENT(field) \
    IntplComponent( \
        IntplComponent(v0->field, v1->field, outer, outerSteps), \
        IntplComponent(v2->field, v3->field, outer, outerSteps), \
        inner, innerSteps)
    result.vx = (short)RAGE_BILERP_VERTEX_COMPONENT(vx);
    result.vy = (short)RAGE_BILERP_VERTEX_COMPONENT(vy);
    result.vz = (short)RAGE_BILERP_VERTEX_COMPONENT(vz);
#undef RAGE_BILERP_VERTEX_COMPONENT
    result.pad = 0;
    return result;
}

uint8_t BilerpByte(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                       int u, int v, int uSteps, int vSteps) {
    int top = IntplComponent(c0, c1, u, uSteps);
    int bottom = IntplComponent(c2, c3, u, uSteps);
    return (uint8_t)IntplComponent(top, bottom, v, vSteps);
}

int ClampSubdivisionLevel(int level) {
    if (level < 0) return 0;
    if (level > 6) return 6;
    return level;
}

int ModelFaceVisible(int mirror, int clip) {
    return mirror ? clip < 0 : clip > 0;
}

int CourseQuadVisible(int mirror, int clip0, int clip1) {
    /* The second triangle uses the opposite winding from the first. */
    return mirror ? (clip0 < 0 || clip1 > 0) : (clip0 > 0 || clip1 < 0);
}

int ScreenQuadOutsideBounds(const int sxy[4], int left, int right, int top,
                            int bottom, int horizontalMargin) {
    int allLeft = 1;
    int allRight = 1;
    int allAbove = 1;
    int allBelow = 1;
    int i;

    for (i = 0; i < 4; i++) {
        int x = (int16_t)sxy[i];
        int y = (int16_t)((uint32_t)sxy[i] >> 16);

        allLeft &= x < left - horizontalMargin;
        allRight &= x > right + horizontalMargin;
        allAbove &= y < top;
        allBelow &= y > bottom;
    }
    return allLeft || allRight || allAbove || allBelow;
}

int OrthonormalizeMatrix3x3(void *matrixStorage,
                                const void *fallbackStorage) {
    int16_t (*matrix)[3] = matrixStorage;
    const int16_t (*fallback)[3] = fallbackStorage;
    float r0[3], r1[3], r2[3], cross[3];
    float length0 = 0.0f, length1 = 0.0f, handedness = 0.0f, dot = 0.0f;
    int axis, row, column;
    for (axis = 0; axis < 3; axis++) {
        r0[axis] = (float)matrix[0][axis];
        r1[axis] = (float)matrix[1][axis];
        r2[axis] = (float)matrix[2][axis];
        length0 += r0[axis] * r0[axis];
    }
    length0 = sqrtf(length0);
    if (length0 > 1.0f) {
        for (axis = 0; axis < 3; axis++) r0[axis] /= length0;
        for (axis = 0; axis < 3; axis++) dot += r1[axis] * r0[axis];
        for (axis = 0; axis < 3; axis++) {
            r1[axis] -= dot * r0[axis];
            length1 += r1[axis] * r1[axis];
        }
        length1 = sqrtf(length1);
    }
    if (length0 <= 1.0f || length1 <= 1.0f) {
        for (row = 0; row < 3; row++)
            for (column = 0; column < 3; column++)
                matrix[row][column] = fallback[row][column];
        return 0;
    }
    for (axis = 0; axis < 3; axis++) r1[axis] /= length1;
    cross[0] = r0[1] * r1[2] - r0[2] * r1[1];
    cross[1] = r0[2] * r1[0] - r0[0] * r1[2];
    cross[2] = r0[0] * r1[1] - r0[1] * r1[0];
    for (axis = 0; axis < 3; axis++) handedness += cross[axis] * r2[axis];
    if (handedness < 0.0f)
        for (axis = 0; axis < 3; axis++) cross[axis] = -cross[axis];
    for (axis = 0; axis < 3; axis++) {
        matrix[0][axis] = (int16_t)lrintf(r0[axis] * 4096.0f);
        matrix[1][axis] = (int16_t)lrintf(r1[axis] * 4096.0f);
        matrix[2][axis] = (int16_t)lrintf(cross[axis] * 4096.0f);
    }
    return 1;
}
