#include <stdint.h>
#include "native_geometry_interpolation.h"

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (0)

int main(void) {
    const int sxy[4] = {
        (10 & 0xffff) | (20 << 16),
        (30 & 0xffff) | (20 << 16),
        (10 & 0xffff) | (40 << 16),
        (30 & 0xffff) | (40 << 16),
    };
    SVECTOR a = {0, 0, 0, 0};
    SVECTOR b = {100, 0, 0, 0};
    SVECTOR c = {0, 100, 0, 0};
    SVECTOR d = {100, 100, 100, 0};
    SVECTOR midpoint;
    int packed;
    int16_t rotating[3][3] = {
        {2048, 0, 2048}, {0, 4096, 0}, {-2048, 0, 2048}};
    int16_t fallback[3][3] = {
        {0, 0, 4096}, {0, 4096, 0}, {-4096, 0, 0}};

    CHECK(RageFloorShift12(4095) == 0);
    CHECK(RageFloorShift12(4096) == 1);
    CHECK(RageFloorShift12(-1) == -1);
    CHECK(RageFloorShift12(-4096) == -1);
    CHECK(RageIntplComponent(100, 300, 0, 4) == 100);
    CHECK(RageIntplComponent(100, 300, 4, 4) == 300);
    CHECK(RageIntplComponent(100, 300, 2, 4) == 200);

    packed = RageBilerpSxy(sxy, 2, 2, 4, 4);
    CHECK((int16_t)packed == 20);
    CHECK((int16_t)(packed >> 16) == 30);
    midpoint = RageBilerpVertex(&a, &b, &c, &d, 2, 2, 4, 4);
    CHECK(midpoint.vx == 50 && midpoint.vy == 50 && midpoint.vz == 25);
    CHECK(RageBilerpByte(0, 100, 100, 200, 2, 2, 4, 4) == 100);
    CHECK(RageClampSubdivisionLevel(-1) == 0);
    CHECK(RageClampSubdivisionLevel(4) == 4);
    CHECK(RageClampSubdivisionLevel(7) == 6);
    CHECK(RageModelFaceVisible(0, 1));
    CHECK(!RageModelFaceVisible(0, -1));
    CHECK(RageModelFaceVisible(1, -1));
    CHECK(!RageModelFaceVisible(1, 1));
    CHECK(!RageModelFaceVisible(0, 0));
    CHECK(!RageModelFaceVisible(1, 0));
    /* The second course triangle has the opposite winding. */
    CHECK(RageCourseQuadVisible(0, 1, -1));
    CHECK(!RageCourseQuadVisible(0, -1, 1));
    CHECK(RageCourseQuadVisible(1, -1, 1));
    CHECK(!RageCourseQuadVisible(1, 1, -1));
    CHECK(!RageCourseQuadVisible(0, 0, 0));
    CHECK(!RageCourseQuadVisible(1, 0, 0));
    CHECK(RageOrthonormalizeMatrix3x3(rotating, fallback));
    CHECK(rotating[0][0] > 2890 && rotating[0][0] < 2905);
    CHECK(rotating[0][2] == rotating[0][0]);
    CHECK(rotating[1][1] == 4096);
    CHECK(rotating[2][0] == -rotating[0][0]);
    CHECK(rotating[2][2] == rotating[0][0]);
    return 0;
}
