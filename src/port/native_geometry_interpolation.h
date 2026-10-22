#ifndef RAGE_NATIVE_GEOMETRY_INTERPOLATION_H
#define RAGE_NATIVE_GEOMETRY_INTERPOLATION_H

#include <libgte.h>
#include <stddef.h>
#include <stdint.h>

int FloorShift12(int64_t value);
int IntplComponent(int start, int end, int index, int steps);
int BilerpSxy(const int sxy[4], int u, int v, int uSteps, int vSteps);
SVECTOR BilerpVertex(const SVECTOR *v0, const SVECTOR *v1,
                         const SVECTOR *v2, const SVECTOR *v3, int outer,
                         int inner, int outerSteps, int innerSteps);
uint8_t BilerpByte(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                       int u, int v, int uSteps, int vSteps);
int ClampSubdivisionLevel(int level);
int ModelFaceVisible(int mirror, int clip);
int CourseQuadVisible(int mirror, int clip0, int clip1);
int ScreenQuadOutsideBounds(const int sxy[4], int left, int right, int top,
                            int bottom, int horizontalMargin);
/* Visible part of a quad clipped to camera Z >= nearZ. inPos is model space
 * and inZ is camera-space Z of the same corners, so the cut lies on the
 * plane even though the positions stay in model space. Returns 0 when the
 * quad is entirely nearer than nearZ, otherwise 3, 4 or 5 corners. A quad
 * already on the far side of the plane is copied unchanged. */
int ClipQuadToCameraNear(const int16_t inPos[4][3], const int inZ[4],
                         const uint8_t inUv[8], int nearZ,
                         int16_t outPos[5][3], uint8_t outUv[10]);
int GeometryBufferHasSpace(const void *buffer, size_t capacity,
                           const void *cursor, size_t required);
int OrthonormalizeMatrix3x3(void *matrixStorage,
                                const void *fallbackStorage);

#endif
