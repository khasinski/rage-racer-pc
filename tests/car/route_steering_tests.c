#include "game/car_track_internal.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[5] = {
        {.x = 0, .z = 0}, {.x = 500, .z = 0}, {.x = 1000, .z = 0},
        {.x = 0, .z = 1000}, {.x = 0, .z = 500},
    };
    const GameTrackPoint otherPoints[5] = {
        {.x = 0, .z = 0}, {.x = 0, .z = 500}, {.x = 0, .z = 2000},
        {.x = 1000, .z = 0}, {.x = 500, .z = 0},
    };
    const TrackRoute route = {.points = points, .count = 5};
    const TrackRoute otherRoute = {.points = otherPoints, .count = 5};
    GameCarSpec fastSpec = {0};
    GameCarSpec slowSpec = {0};
    fastSpec.steerResponse = 20;
    slowSpec.steerResponse = 40;
    PlayerCarRuntime initial = {0};
    initial.drive.launchDirection = 1;
    PlayerCarRuntime first = initial;
    SteerCarOnRoute(&first, &fastSpec, &route);
    CHECK(first.headingAngle == 1024);
    PlayerCarRuntime second = initial;
    SteerCarOnRoute(&second, &slowSpec, &route);
    CHECK(second.headingAngle == 512 && first.headingAngle == 1024);
    PlayerCarRuntime third = initial;
    SteerCarOnRoute(&third, &fastSpec, &otherRoute);
    CHECK(third.headingAngle == 0 && second.headingAngle == 512);
    PlayerCarRuntime restored = initial;
    SteerCarOnRoute(&restored, &fastSpec, &route);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);

    restored = initial;
    restored.drive.launchDirection = 0;
    SteerCarOnRoute(&restored, &fastSpec, &route);
    CHECK(restored.headingAngle == 0);
    CHECK(CalculateRouteOffsetHeading(&route, 2, 0, 0, 0, 0) == 1024);
    CHECK(CalculateRouteOffsetHeading(&route, 2, 0, 0, 0, 200) != 1024);
    restored = initial;
    restored.verticalMotionState = CAR_VERTICAL_RISING;
    restored.headingAngle = 42;
    PlayerCarRuntime airborne = restored;
    SteerCarOnRoute(&restored, &fastSpec, &route);
    CHECK(memcmp(&airborne, &restored, sizeof(restored)) == 0);
    restored = initial;
    SteerCarOnRoute(&restored, NULL, &route);
    SteerCarOnRoute(&restored, &fastSpec, NULL);
    CHECK(memcmp(&restored, &initial, sizeof(initial)) == 0);
    SteerCarOnRoute(NULL, &fastSpec, &route);
    puts("route steering tests passed");
    return 0;
}
