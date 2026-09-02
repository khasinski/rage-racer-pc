#include "game/car_internal.h"

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
    CarCollisionHit hit;
    s32 i;

    memset(grid, 0, sizeof(grid));
    memset(points, 0, sizeof(points));
    for (i = 0; i < 4; i++) {
        grid[i][2].x = (s16)(100 + i);
        grid[i][2].z = (s16)(-200 - i);
    }
    CHECK(GetCarCollisionPointPacked(&grid[0][2]) == (s32)0xFF380064u);
    points[1] = grid[2][2];

    hit = FindFirstCarCollisionQuad(grid, points, 3);
    CHECK(hit.region == 3);
    CHECK(hit.sampleIndex == 1 && hit.quadIndex == 2);

    points[1].x = 500;
    hit = FindFirstCarCollisionQuad(grid, points, 3);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    hit = FindFirstCarCollisionQuad(grid, points, 0);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    puts("collision quad search tests passed");
    return 0;
}
