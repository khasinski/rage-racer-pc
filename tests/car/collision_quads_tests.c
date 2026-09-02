#include "game/car.h"

#include <stdio.h>
#include <string.h>

s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 point) {
    (void)p1;
    (void)p2;
    (void)p3;
    return p0 == point;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CarCollisionPoint grid[4][4];
    CarCollisionPoint points[3];
    s32 sample;
    s32 quad;
    s32 i;

    memset(grid, 0, sizeof(grid));
    memset(points, 0, sizeof(points));
    for (i = 0; i < 4; i++) {
        grid[i][2].x = (s16)(100 + i);
        grid[i][2].z = (s16)(-200 - i);
    }
    CHECK(GetCarCollisionPointPacked(&grid[0][2]) == (s32)0xFF380064u);
    points[1] = grid[2][2];

    CHECK(FirstQuadHit(grid, points, 3, &sample, &quad) == 3);
    CHECK(sample == 1 && quad == 2);

    points[1].x = 500;
    CHECK(FirstQuadHit(grid, points, 3, &sample, &quad) == 0);
    CHECK(sample == 3 && quad == 4);

    sample = -1;
    quad = -1;
    CHECK(FirstQuadHit(grid, points, 0, &sample, &quad) == 0);
    CHECK(sample == 0 && quad == 0);

    puts("collision quad search tests passed");
    return 0;
}
