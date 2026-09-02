#include "game/car_render_rules.h"

#include <limits.h>
#include <stdio.h>

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,        \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CHECK_EQ(CarRenderManhattanDistance(100, -50, 70, -20), 60);
    CHECK_EQ(CarRenderManhattanDistance(-100, 50, -70, 20), 60);
    CHECK_EQ(CarRenderManhattanDistance(INT_MIN, 0, INT_MAX, 0), INT_MAX);
    CHECK_EQ(CarRenderManhattanDistance(INT_MIN, INT_MIN,
                                        INT_MIN, INT_MIN), 0);

    CHECK_EQ(ClassifyCarRenderRange(-1, 0), CAR_RENDER_BEHIND);
    CHECK_EQ(ClassifyCarRenderRange(0, 0xCFF), CAR_RENDER_CLOSE);
    CHECK_EQ(ClassifyCarRenderRange(0, 0xD00), CAR_RENDER_FAR);
    CHECK_EQ(ClassifyCarRenderRange(0, 0x24FF), CAR_RENDER_FAR);
    CHECK_EQ(ClassifyCarRenderRange(0, 0x2500), CAR_RENDER_CULLED);

    CHECK_EQ(ResolveCarModelBank(3, 2, 6), 5);
    CHECK_EQ(ResolveCarModelBank(3, 3, 6), 1);
    CHECK_EQ(ResolveCarModelBank(6, 0, 6), 1);
    CHECK_EQ(ResolveCarModelBank(-1, 0, 6), 1);
    CHECK_EQ(ResolveCarModelBank(INT_MAX, 1, 6), 1);
    CHECK_EQ(ResolveCarModelBank(INT_MIN, -1, 6), 1);

    puts("car render rules tests passed");
    return 0;
}
