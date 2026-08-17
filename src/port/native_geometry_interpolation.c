#include "native_geometry_interpolation.h"

int RageFloorShift12(int64_t value) {
    if (value >= 0) return (int)(value >> 12);
    return -(int)((-value + 0xfff) >> 12);
}

int RageIntplComponent(int start, int end, int index, int steps) {
    int ir0 = 4096 - index * (4096 / steps);
    int difference = start - end;
    int result;
    if (difference < -32768) difference = -32768;
    if (difference > 32767) difference = 32767;
    result = RageFloorShift12((int64_t)end * 4096 +
                              (int64_t)ir0 * difference);
    if (result < -32768) result = -32768;
    if (result > 32767) result = 32767;
    return result;
}

int RageBilerpSxy(const int sxy[4], int u, int v, int uSteps, int vSteps) {
    int topX = RageIntplComponent((int16_t)sxy[0], (int16_t)sxy[1], u,
                                  uSteps);
    int bottomX = RageIntplComponent((int16_t)sxy[2], (int16_t)sxy[3], u,
                                     uSteps);
    int topY = RageIntplComponent((int16_t)(sxy[0] >> 16),
                                  (int16_t)(sxy[1] >> 16), u, uSteps);
    int bottomY = RageIntplComponent((int16_t)(sxy[2] >> 16),
                                     (int16_t)(sxy[3] >> 16), u, uSteps);
    int x = RageIntplComponent(topX, bottomX, v, vSteps);
    int y = RageIntplComponent(topY, bottomY, v, vSteps);
    return (int)((uint16_t)x | ((uint32_t)(uint16_t)y << 16));
}

SVECTOR RageBilerpVertex(const SVECTOR *v0, const SVECTOR *v1,
                         const SVECTOR *v2, const SVECTOR *v3, int outer,
                         int inner, int outerSteps, int innerSteps) {
    SVECTOR result;
#define RAGE_BILERP_VERTEX_COMPONENT(field) \
    RageIntplComponent( \
        RageIntplComponent(v0->field, v1->field, outer, outerSteps), \
        RageIntplComponent(v2->field, v3->field, outer, outerSteps), \
        inner, innerSteps)
    result.vx = (short)RAGE_BILERP_VERTEX_COMPONENT(vx);
    result.vy = (short)RAGE_BILERP_VERTEX_COMPONENT(vy);
    result.vz = (short)RAGE_BILERP_VERTEX_COMPONENT(vz);
#undef RAGE_BILERP_VERTEX_COMPONENT
    result.pad = 0;
    return result;
}

uint8_t RageBilerpByte(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                       int u, int v, int uSteps, int vSteps) {
    int top = RageIntplComponent(c0, c1, u, uSteps);
    int bottom = RageIntplComponent(c2, c3, u, uSteps);
    return (uint8_t)RageIntplComponent(top, bottom, v, vSteps);
}

int RageClampSubdivisionLevel(int level) {
    if (level < 0) return 0;
    if (level > 6) return 6;
    return level;
}

int RageModelFaceVisible(int mirror, int clip) {
    return mirror ? clip < 0 : clip > 0;
}

int RageCourseQuadVisible(int mirror, int clip0, int clip1) {
    /* The second triangle uses the opposite winding from the first. */
    return mirror ? (clip0 < 0 || clip1 > 0) : (clip0 > 0 || clip1 < 0);
}
