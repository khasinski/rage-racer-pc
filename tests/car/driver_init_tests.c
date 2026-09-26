#include "driver_fixture.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .y = 50, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 1000, .y = 50, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 2000, .y = 50, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
    };
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    const TrackRivalStart position = {.x = 200, .z = 0, .trackPointIndex = 0};
    const DriverStart start = {.route = &route, .position = &position, .walkStart = 0,
        .manual = 1, .modelIndex = 23, .launchThresholdIndex = 4};
    PlayerCarRuntime first, second;
    GameCarSpec spec, otherSpec;
    CarPerformance engine = {0}, otherEngine = {0};
    PrepareDriver(&first, &spec, &engine);
    PrepareDriver(&second, &otherSpec, &otherEngine);
    memset(&first, 0x7F, sizeof(first));
    InitDriver(&first, &spec, &engine, &start);
    CHECK(first.drive.manual == 1 && first.modelIndex == 23);
    CHECK(first.drive.launchThresholdIndex == 4);
    CHECK(first.drive.motionState == CAR_MOTION_STANDING_START);
    CHECK(first.drive.gear == 1 && first.drive.gearDisp == 1);
    CHECK(first.drive.drivetrainCoupled == 1 && first.drive.dragScale == 1000);
    CHECK(first.drive.hudLapHighlightRow == -1 && first.drive.racePosition == 1);
    CHECK(first.drive.acceleratorInput.value == 0 && first.drive.brakeInput == 0);
    CHECK(first.drive.autoShiftCooldown == 0 && first.drive.gripLossTimer == 0);
    CHECK(first.drive.shiftRpmDelta == 0 && first.drive.shiftSoundLevel == 0);
    CHECK(first.bodyYaw == 3072 && first.headingAngle == 3072);
    CHECK(first.y == 50 && first.modelY == 50 && first.modelYaw == first.bodyYaw);
    CHECK(first.x == position.x + first.motionX && first.z == position.z + first.motionZ);
    CHECK(first.previousTrackProgress == first.trackProgress);
    CHECK(first.lapTimes.words[11] == 0 && first.positionW == 0);
    PlayerCarRuntime baseline = first;
    DriverStart reverse = start;
    reverse.reverse = 1;
    reverse.manual = 0;
    reverse.modelIndex = 42;
    InitDriver(&second, &otherSpec, &otherEngine, &reverse);
    CHECK(second.bodyYaw == 1024 && second.facingBackwards == 1);
    CHECK(second.drive.manual == 0 && second.modelIndex == 42);
    CHECK(memcmp(&first, &baseline, sizeof(first)) == 0);
    InitDriver(&first, &spec, &engine, &start);
    CHECK(memcmp(&first, &baseline, sizeof(first)) == 0);
    DriverStart missing = start;
    missing.position = NULL;
    memset(&second, 0x55, sizeof(second));
    InitDriver(&second, &otherSpec, &otherEngine, &missing);
    CHECK(second.x == 0 && second.y == 0 && second.z == 0);
    CHECK(second.drive.manual == 1 && second.drive.motionState == CAR_MOTION_STANDING_START);
    missing = start;
    missing.route = NULL;
    InitDriver(&second, &otherSpec, &otherEngine, &missing);
    CHECK(second.x == 0 && second.y == 0 && second.z == 0);
    CHECK(second.drive.gear == 1);
    return 0;
}
