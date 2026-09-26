#include "game/car_drive.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        failures++; \
    } \
} while (0)

int main(void) {
    GameCarSpec spec = {0};
    spec.speedDragDivisor = 1000;
    spec.negconSteeringAssistScale = 1000;
    GameTrackPoint uphill[2] = {
        {.surfacePitch = 100, .crossSlope = 10, .arcRef = 1},
        {.surfacePitch = 200},
    };
    GameTrackPoint downhill[2] = {
        {.surfacePitch = -100, .arcRef = 2},
        {.surfacePitch = -200},
    };
    const DriveContext firstRoad = {
        .point = uphill, .nextPoint = uphill + 1, .racing = 1, .digitalSteering = 1,
    };
    const DriveContext secondRoad = {.point = downhill, .nextPoint = downhill + 1};
    PlayerCarRuntime first = {0};
    first.headingAngle = 0xC00;
    first.bodyYaw = 0xC00;
    first.segmentFraction = 512;
    first.drive.trackCurveMode = 2;
    first.drive.steeringGrip = 20;
    first.drive.steeringGripResponse = 1000;
    first.drive.gripLossTimer = 20;
    first.drive.driveBoostTimer = 15;
    first.drive.dragScale = 700;
    PlayerCarRuntime second = first;
    PlayerCarRuntime alone = first;

    for (int i = 0; i < 10; i++) {
        UpdateCarSteeringGrip(&alone, &spec, &firstRoad, 100);
        CalculateCarDrivetrainLoads(&alone, &spec, &firstRoad, 0, 0, 0);
    }
    for (int i = 0; i < 10; i++) {
        UpdateCarSteeringGrip(&first, &spec, &firstRoad, 100);
        UpdateCarSteeringGrip(&second, &spec, &secondRoad, 100);
        CalculateCarDrivetrainLoads(&second, &spec, &secondRoad, 0, 0, 0);
        CalculateCarDrivetrainLoads(&first, &spec, &firstRoad, 0, 0, 0);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(first.drive.roadGrade == 150);
    CHECK(second.drive.roadGrade == -150);
    CHECK(first.drive.steeringGrip < second.drive.steeringGrip);
    CHECK(first.drive.gripLossTimer == 10);
    CHECK(second.drive.driveBoostTimer == 5);

    PlayerCarRuntime restored = second;
    CalculateCarDrivetrainLoads(&first, &spec, &firstRoad, 0, 0, 0);
    CalculateCarDrivetrainLoads(&second, &spec, &secondRoad, 0, 0, 0);
    CalculateCarDrivetrainLoads(&restored, &spec, &secondRoad, 0, 0, 0);
    CHECK(memcmp(&second, &restored, sizeof(second)) == 0);

    PlayerCarRuntime car = {0};
    car.drive.steerPos = 1200;
    car.drive.steeringGripResponse = 1000;
    const DriveContext digital = {.digitalSteering = 1};
    const DriveContext analog = {0};
    CHECK(CalculateCarDrivetrainLoads(&car, &spec, &digital, 0, 0, 0)
              .motionResistance == 1);
    CHECK(CalculateCarDrivetrainLoads(&car, &spec, &analog, 0, 0, 0)
              .motionResistance == 0);
    car.drive.steerPos = 0;
    car.drive.motionState = CAR_MOTION_STANDING_START;
    car.drive.standingStartSpin = 3;
    const DriveContext racing = {.racing = 1};
    CHECK(CalculateCarDrivetrainLoads(&car, &spec, &racing, 0, 0, 0)
              .motionResistance == 15);
    CHECK(CalculateCarDrivetrainLoads(&car, &spec, &analog, 0, 0, 0)
              .motionResistance == 0);
    CHECK(car.drive.roadGrade == 0);
    return failures != 0;
}
