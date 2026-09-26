#include "game/car_track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[2] = {
        {.segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 1000, .segmentLength = 1000,
         .leftHalfWidth = 100, .rightHalfWidth = 100},
    };
    const TrackRoute route = {.points = points, .count = 2, .length = 2000};
    const CarHullPoint corners[4] = {{-15, 20}, {15, 20}, {-8, -10}, {8, -10}};
    PlayerCarRuntime initial = {0};
    initial.x = 200;
    initial.z = 150;
    initial.bodyYaw = 0xC00;
    initial.speed = 100;
    PlayerCarRuntime car = initial;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 2);
    CHECK(car.z == 40 && car.motionActive == 1);
    CHECK(car.trackProgress == 800);

    car = initial;
    car.speed = 63;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 0);
    CHECK(car.z == 40 && car.motionActive == 1);
    car = initial;
    car.speed = 64;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 2);
    car = initial;
    car.speed = 63;
    car.z = -150;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 1);
    CHECK(car.z == -40);

    const CarHullPoint rearCorners[4] = {{-8, 20}, {8, 20}, {15, -10}, {-15, -10}};
    car = initial;
    car.speed = 63;
    CHECK(ResolveCarTrackContact(&car, &route, rearCorners, 0) == 0);
    car = initial;
    car.speed = 63;
    car.z = -150;
    CHECK(ResolveCarTrackContact(&car, &route, rearCorners, 0) == 4);

    car = initial;
    car.z = 0;
    car.motionActive = 1;
    car.motionTimer = 1;
    car.velocityX = 8;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 0);
    CHECK(car.x == 192 && car.velocityX == 7 && car.motionActive == 0);
    car = initial;
    car.z = 0;
    car.motionActive = 1;
    car.motionTimer = 0x8000;
    car.velocityX = 8;
    ResolveCarTrackContact(&car, &route, corners, 0);
    CHECK(car.x == 192 && car.motionTimer == 0x7FFF);
    car = initial;
    car.z = 0;
    car.motionTimer = 1;
    car.velocityX = 8;
    ResolveCarTrackContact(&car, &route, corners, 0);
    CHECK(car.x == 200);

    /* A quarter-turn changes the reaching corner through the track-frame
     * convention; using the game's opposite Y rotation would pick another one. */
    car = initial;
    car.bodyYaw += 0x400;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 1);
    CHECK(car.z == 20);
    car = initial;
    car.bodyYaw += 0x400;
    car.z = -150;
    CHECK(ResolveCarTrackContact(&car, &route, corners, 0) == 3);
    CHECK(car.z == -60);

    PlayerCarRuntime isolated = initial;
    ResolveCarTrackContact(&isolated, &route, corners, 0);
    PlayerCarRuntime other = initial;
    other.bodyYaw += 0x400;
    ResolveCarTrackContact(&other, &route, rearCorners, 1);
    car = initial;
    ResolveCarTrackContact(&car, &route, corners, 0);
    CHECK(memcmp(&isolated, &car, sizeof(car)) == 0);
    car = initial;
    CHECK(ResolveCarTrackContact(&car, NULL, corners, 0) == 0);
    CHECK(memcmp(&initial, &car, sizeof(car)) == 0);
    CHECK(ResolveCarTrackContact(NULL, &route, corners, 0) == 0);
    TrackRoute empty = route;
    empty.count = 0;
    CHECK(ResolveCarTrackContact(&car, &empty, corners, 0) == 0);
    empty = route;
    empty.points = NULL;
    CHECK(ResolveCarTrackContact(&car, &empty, corners, 0) == 0);
    puts("real player track contact checks passed");
    return 0;
}
