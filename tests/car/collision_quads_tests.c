#include "game/car_collision_internal.h"

#include <stdio.h>
#include <string.h>

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
    CarCollisionPoint midpoint;
    CarCollisionHit hit;
    s32 i;

    midpoint = CarCollisionMidpoint(
        (CarCollisionPoint){-4, -3}, (CarCollisionPoint){1, 2});
    CHECK(midpoint.x == -1 && midpoint.z == 0);
    midpoint = CarCollisionMidpoint(
        (CarCollisionPoint){INT16_MIN, INT16_MAX},
        (CarCollisionPoint){INT16_MIN, INT16_MAX});
    CHECK(midpoint.x == INT16_MIN && midpoint.z == INT16_MAX);

    memset(grid, 0, sizeof(grid));
    memset(points, 0, sizeof(points));
    for (i = 0; i < 4; i++) {
        grid[i][0] = (CarCollisionPoint){10, 10};
        grid[i][1] = (CarCollisionPoint){10, 30};
        grid[i][2] = (CarCollisionPoint){30, 10};
        grid[i][3] = (CarCollisionPoint){30, 30};
    }
    CHECK(GetCarCollisionPointPacked(&grid[0][2]) == (s32)0x000A001Eu);
    points[1] = (CarCollisionPoint){20, 20};

    hit = FindFirstCarCollisionQuad(grid, points, 3);
    CHECK(hit.region == 1);
    CHECK(hit.sampleIndex == 1 && hit.quadIndex == 0);

    points[1] = (CarCollisionPoint){10, 20};
    hit = FindFirstCarCollisionQuad(grid, points, 3);
    CHECK(hit.region == 1);
    CHECK(hit.sampleIndex == 1 && hit.quadIndex == 0);

    points[1].x = 31;
    hit = FindFirstCarCollisionQuad(grid, points, 3);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    memset(grid, 0, sizeof(grid));
    points[0] = (CarCollisionPoint){0, 0};
    hit = FindFirstCarCollisionQuad(grid, points, 1);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    hit = FindFirstCarCollisionQuad(grid, points, 0);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    hit = FindFirstCarCollisionQuad(NULL, points, 1);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    hit = FindFirstCarCollisionQuad(grid, NULL, 1);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    hit = FindFirstCarCollisionQuad(grid, NULL, -1);
    CHECK(hit.region == 0);
    CHECK(hit.sampleIndex == -1 && hit.quadIndex == -1);

    puts("collision quad search tests passed");
    return 0;
}
