#ifndef RAGE_NATIVE_GEOMETRY_INTERPOLATION_H
#define RAGE_NATIVE_GEOMETRY_INTERPOLATION_H

#include <libgte.h>
#include <stdint.h>

int RageFloorShift12(int64_t value);
int RageIntplComponent(int start, int end, int index, int steps);
int RageBilerpSxy(const int sxy[4], int u, int v, int uSteps, int vSteps);
SVECTOR RageBilerpVertex(const SVECTOR *v0, const SVECTOR *v1,
                         const SVECTOR *v2, const SVECTOR *v3, int outer,
                         int inner, int outerSteps, int innerSteps);
uint8_t RageBilerpByte(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                       int u, int v, int uSteps, int vSteps);
int RageClampSubdivisionLevel(int level);
int RageModelFaceVisible(int mirror, int clip);
int RageCourseQuadVisible(int mirror, int clip0, int clip1);
int RageOrthonormalizeMatrix3x3(void *matrixStorage,
                                const void *fallbackStorage);

#endif
