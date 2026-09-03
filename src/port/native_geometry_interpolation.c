#include "native_geometry_interpolation.h"

#include <limits.h>
#include <math.h>

int FloorShift12(int64_t value) {
    int64_t result = value / 4096;
    if (value < 0 && value % 4096 != 0) result--;
    if (result < INT_MIN) return INT_MIN;
    if (result > INT_MAX) return INT_MAX;
    return (int)result;
}

static int ClampInt16(int64_t value) {
    if (value < INT16_MIN) return INT16_MIN;
    if (value > INT16_MAX) return INT16_MAX;
    return value;
}

int IntplComponent(int start, int end, int index, int steps) {
    int64_t ir0;
    int difference;
    int result;

    if (steps <= 0) return ClampInt16(start);
    ir0 = 4096 - (int64_t)index * (4096 / steps);
    difference = ClampInt16((int64_t)start - end);
    result = FloorShift12((int64_t)end * 4096 +
                          ir0 * difference);
    return ClampInt16(result);
}

int BilerpSxy(const int sxy[4], int u, int v, int uSteps, int vSteps) {
    if (sxy == NULL) return 0;
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

static int BilerpComponent(int c0, int c1, int c2, int c3,
                           int outer, int inner,
                           int outerSteps, int innerSteps) {
    return IntplComponent(
        IntplComponent(c0, c1, outer, outerSteps),
        IntplComponent(c2, c3, outer, outerSteps), inner, innerSteps);
}

SVECTOR BilerpVertex(const SVECTOR *v0, const SVECTOR *v1,
                     const SVECTOR *v2, const SVECTOR *v3, int outer,
                     int inner, int outerSteps, int innerSteps) {
    SVECTOR result;

    if (v0 == NULL || v1 == NULL || v2 == NULL || v3 == NULL) {
        result = (SVECTOR){0, 0, 0, 0};
        return result;
    }

    result.vx = (short)BilerpComponent(
        v0->vx, v1->vx, v2->vx, v3->vx,
        outer, inner, outerSteps, innerSteps);
    result.vy = (short)BilerpComponent(
        v0->vy, v1->vy, v2->vy, v3->vy,
        outer, inner, outerSteps, innerSteps);
    result.vz = (short)BilerpComponent(
        v0->vz, v1->vz, v2->vz, v3->vz,
        outer, inner, outerSteps, innerSteps);
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

    if (sxy == NULL) return 1;

    for (i = 0; i < 4; i++) {
        int x = (int16_t)sxy[i];
        int y = (int16_t)((uint32_t)sxy[i] >> 16);

        allLeft &= (int64_t)x < (int64_t)left - horizontalMargin;
        allRight &= (int64_t)x > (int64_t)right + horizontalMargin;
        allAbove &= y < top;
        allBelow &= y > bottom;
    }
    return allLeft || allRight || allAbove || allBelow;
}

int GeometryBufferHasSpace(const void *buffer, size_t capacity,
                           const void *cursor, size_t required) {
    uintptr_t begin = (uintptr_t)buffer;
    uintptr_t at = (uintptr_t)cursor;

    if (capacity > UINTPTR_MAX - begin) return 0;
    if (at < begin || at > begin + capacity) return 0;
    return required <= begin + capacity - at;
}

int OrthonormalizeMatrix3x3(void *matrixStorage,
                                const void *fallbackStorage) {
    int16_t (*matrix)[3] = matrixStorage;
    const int16_t (*fallback)[3] = fallbackStorage;
    float r0[3], r1[3], r2[3], cross[3];
    float length0 = 0.0f, length1 = 0.0f, handedness = 0.0f, dot = 0.0f;
    int axis, row, column;
    if (matrix == NULL || fallback == NULL) return 0;
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
