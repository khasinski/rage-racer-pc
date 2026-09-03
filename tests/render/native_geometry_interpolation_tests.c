#include <limits.h>
#include <stdint.h>
#include "native_geometry_interpolation.h"

#define CHECK(expression) do { if (!(expression)) return __LINE__; } while (0)
#define PACK_SXY(x, y) \
    ((int)((uint16_t)(x) | ((uint32_t)(uint16_t)(y) << 16)))

int main(void) {
    uint8_t buffer[16];
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

    CHECK(FloorShift12(4095) == 0);
    CHECK(FloorShift12(4096) == 1);
    CHECK(FloorShift12(-1) == -1);
    CHECK(FloorShift12(-4096) == -1);
    CHECK(FloorShift12(INT64_MIN) == INT_MIN);
    CHECK(FloorShift12(INT64_MAX) == INT_MAX);
    CHECK(IntplComponent(100, 300, 0, 4) == 100);
    CHECK(IntplComponent(100, 300, 4, 4) == 300);
    CHECK(IntplComponent(100, 300, 2, 4) == 200);
    CHECK(IntplComponent(100, 300, 2, 0) == 100);
    CHECK(IntplComponent(50000, 300, 2, -1) == INT16_MAX);
    CHECK(IntplComponent(INT_MIN, INT_MAX, 0, 1) == INT16_MAX);

    packed = BilerpSxy(sxy, 2, 2, 4, 4);
    CHECK((int16_t)packed == 20);
    CHECK((int16_t)(packed >> 16) == 30);
    midpoint = BilerpVertex(&a, &b, &c, &d, 2, 2, 4, 4);
    CHECK(midpoint.vx == 50 && midpoint.vy == 50 && midpoint.vz == 25);
    CHECK(BilerpByte(0, 100, 100, 200, 2, 2, 4, 4) == 100);
    CHECK(BilerpSxy(NULL, 0, 0, 1, 1) == 0);
    midpoint = BilerpVertex(NULL, &b, &c, &d, 0, 0, 1, 1);
    CHECK(midpoint.vx == 0 && midpoint.vy == 0 && midpoint.vz == 0);
    CHECK(ClampSubdivisionLevel(-1) == 0);
    CHECK(ClampSubdivisionLevel(4) == 4);
    CHECK(ClampSubdivisionLevel(7) == 6);
    CHECK(ModelFaceVisible(0, 1));
    CHECK(!ModelFaceVisible(0, -1));
    CHECK(ModelFaceVisible(1, -1));
    CHECK(!ModelFaceVisible(1, 1));
    CHECK(!ModelFaceVisible(0, 0));
    CHECK(!ModelFaceVisible(1, 0));
    /* The second course triangle has the opposite winding. */
    CHECK(CourseQuadVisible(0, 1, -1));
    CHECK(!CourseQuadVisible(0, -1, 1));
    CHECK(CourseQuadVisible(1, -1, 1));
    CHECK(!CourseQuadVisible(1, 1, -1));
    CHECK(!CourseQuadVisible(0, 0, 0));
    CHECK(!CourseQuadVisible(1, 0, 0));
    {
        const int left[4] = {
            PACK_SXY(-20, 10), PACK_SXY(-10, 20),
            PACK_SXY(-30, 30), PACK_SXY(-1, 40)};
        const int crossing[4] = {
            PACK_SXY(-1, 10), PACK_SXY(20, 20),
            PACK_SXY(20, 30), PACK_SXY(20, 40)};
        const int above[4] = {
            PACK_SXY(10, -20), PACK_SXY(20, -10),
            PACK_SXY(30, -30), PACK_SXY(40, -1)};
        const int right[4] = {
            PACK_SXY(321, 10), PACK_SXY(330, 20),
            PACK_SXY(340, 30), PACK_SXY(350, 40)};
        const int below[4] = {
            PACK_SXY(10, 241), PACK_SXY(20, 250),
            PACK_SXY(30, 260), PACK_SXY(40, 270)};

        CHECK(ScreenQuadOutsideBounds(left, 0, 320, 0, 240, 0));
        CHECK(!ScreenQuadOutsideBounds(left, 0, 320, 0, 240, 32));
        CHECK(!ScreenQuadOutsideBounds(crossing, 0, 320, 0, 240, 0));
        CHECK(ScreenQuadOutsideBounds(above, 0, 320, 0, 240, 0));
        CHECK(ScreenQuadOutsideBounds(right, 0, 320, 0, 240, 0));
        CHECK(ScreenQuadOutsideBounds(below, 0, 320, 0, 240, 0));
    }
    CHECK(ScreenQuadOutsideBounds(NULL, 0, 320, 0, 240, 0));
    CHECK(GeometryBufferHasSpace(buffer, sizeof(buffer), buffer, 16));
    CHECK(GeometryBufferHasSpace(buffer, sizeof(buffer), buffer + 8, 8));
    CHECK(!GeometryBufferHasSpace(buffer, sizeof(buffer), buffer + 8, 9));
    CHECK(GeometryBufferHasSpace(buffer, sizeof(buffer), buffer + 16, 0));
    CHECK(!GeometryBufferHasSpace(buffer, sizeof(buffer), buffer + 16, 1));
    CHECK(!GeometryBufferHasSpace(
        buffer, sizeof(buffer),
        (void *)((uintptr_t)buffer - 1), 0));
    CHECK(!GeometryBufferHasSpace(
        (void *)(uintptr_t)(UINTPTR_MAX - 3), 8, buffer, 0));
    CHECK(!OrthonormalizeMatrix3x3(NULL, fallback));
    CHECK(!OrthonormalizeMatrix3x3(rotating, NULL));
    CHECK(OrthonormalizeMatrix3x3(rotating, fallback));
    CHECK(rotating[0][0] > 2890 && rotating[0][0] < 2905);
    CHECK(rotating[0][2] == rotating[0][0]);
    CHECK(rotating[1][1] == 4096);
    CHECK(rotating[2][0] == -rotating[0][0]);
    CHECK(rotating[2][2] == rotating[0][0]);
    return 0;
}

#undef PACK_SXY
