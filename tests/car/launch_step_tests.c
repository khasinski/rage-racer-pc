#include "game/car_drive.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[5] = {
        {.x = 0}, {.x = 500}, {.x = 1000}, {.z = 1000}, {.z = 500},
    };
    const GameTrackPoint otherPoints[5] = {
        {.z = 0}, {.z = 500}, {.z = 1000}, {.x = 1000}, {.x = 500},
    };
    const TrackRoute route = {.points = points, .count = 5};
    const TrackRoute otherRoute = {.points = otherPoints, .count = 5};
    GameCarSpec spec = {.steerResponse = 20};
    GameCarSpec otherSpec = {.steerResponse = 40};
    PlayerCarRuntime first = {0};
    first.speed = 1000;
    first.drive.motionState = CAR_MOTION_TAKEOFF;
    first.drive.launchDirection = 1;
    first.drive.launchEnergy = 1000000;
    first.drive.spinRate = 3000;
    first.drive.acceleratorInput.value = 256;
    PlayerCarRuntime second = first;
    PlayerCarRuntime alone = first;
    PlayerCarRuntime otherAlone = second;
    for (int i = 0; i < 12; i++) {
        StepCarLaunch(&first, &spec, &route);
        StepCarLaunch(&second, &otherSpec, &otherRoute);
    }
    for (int i = 0; i < 12; i++) {
        StepCarLaunch(&alone, &spec, &route);
        StepCarLaunch(&otherAlone, &otherSpec, &otherRoute);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(memcmp(&second, &otherAlone, sizeof(second)) == 0);
    CHECK(first.headingAngle != second.headingAngle);
    CHECK(first.drive.launchEnergy < 1000000);
    PlayerCarRuntime restored = first;
    StepCarLaunch(&first, &spec, &route);
    StepCarLaunch(&second, &otherSpec, &otherRoute);
    StepCarLaunch(&restored, &spec, &route);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    first.drive.launchEnergy = 0;
    first.drive.spinRate = 4096;
    StepCarLaunch(&first, &spec, &route);
    CHECK(first.drive.motionState == CAR_MOTION_TAKEOFF);
    CHECK(first.drive.spinRate == 3840);
    first.drive.spinRate = 0;
    StepCarLaunch(&first, &spec, &route);
    CHECK(first.drive.motionState == CAR_MOTION_AIRBORNE);
    CHECK(first.drive.spinRate == 0);
    return 0;
}
