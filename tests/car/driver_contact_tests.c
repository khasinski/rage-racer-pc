#include "game/car_collision_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const CarHullPoint points[6] = {
        {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16},
    };
    const CarHullPoint corners[4] = {{-26, 96}, {26, 96}, {-26, -16}, {26, -16}};
    const DriverHull hull = {.points = points, .corners = corners};
    PlayerCarRuntime first = {0}, second = {0};
    first.drive.dragScale = 123;
    second.drive.dragScale = 456;
    PlayerCarRuntime savedFirst = first, savedSecond = second;
    DriverContact hit = FindDriverContact(&first, &hull, &second, &hull, 32768);
    CHECK(hit.firstRegion > 0 && hit.secondRegion > 0);
    CHECK(memcmp(&first, &savedFirst, sizeof(first)) == 0);
    CHECK(memcmp(&second, &savedSecond, sizeof(second)) == 0);
    const CarHullPoint emptyPoints[6] = {{0}};
    const DriverHull otherHull = {.points = emptyPoints, .corners = corners};
    hit = FindDriverContact(&first, &hull, &second, &otherHull, 32768);
    CHECK(hit.firstRegion > 0 && hit.secondRegion == 0);
    DriverContact reversed = FindDriverContact(&second, &otherHull, &first, &hull, 32768);
    CHECK(reversed.firstRegion == hit.secondRegion && reversed.secondRegion == hit.firstRegion);
    for (int x = -80; x <= 80; x += 8) {
        for (int z = -80; z <= 80; z += 8) {
            second.x = x;
            second.z = z;
            hit = FindDriverContact(&first, &hull, &second, &hull, 32768);
            DriverContact swapped = FindDriverContact(&second, &hull, &first, &hull, 32768);
            CHECK(hit.firstRegion == swapped.secondRegion && hit.secondRegion == swapped.firstRegion);
        }
    }
    second.x = 1000;
    second.z = 0;
    hit = FindDriverContact(&first, &hull, &second, &hull, 32768);
    CHECK(hit.firstRegion == 0 && hit.secondRegion == 0);
    second = savedSecond;
    second.activeFlag = -1;
    hit = FindDriverContact(&first, &hull, &second, &hull, 32768);
    CHECK(hit.firstRegion == 0 && hit.secondRegion == 0);
    hit = FindDriverContact(&first, &hull, &first, &hull, 32768);
    CHECK(hit.firstRegion == 0 && hit.secondRegion == 0);
    hit = FindDriverContact(&first, NULL, &second, &hull, 32768);
    CHECK(hit.firstRegion == 0 && hit.secondRegion == 0);
    first = savedFirst;
    second = savedSecond;
    ApplyDriverCollision(&first, &second, 1, 0, 0, 0, 0);
    CHECK(first.collisionFlag == 1 && second.collisionFlag == 1);
    CHECK(second.drive.drivetrainTorque == savedSecond.drive.drivetrainTorque);
    return 0;
}
