#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/audio.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
LaunchSpeedThreshold g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];
s16 g_RacePhase;

static void StepDriving(PlayerCarRuntime *car) {
    const LaunchSpeedThreshold *threshold = &g_LaunchSpeedThresholds[
        NormalizeCarLaunchThresholdIndex(car->drive.launchThresholdIndex)];
    PlayCarDrivingVoice(car, g_CarSpec);
    StepCarDriving(car, threshold);
    if (car->drive.motionState == CAR_MOTION_TAKEOFF) {
        SetIndexedEffectVoice(0, 0, 0);
    }
}

static s32 s_voiceIndex;
static s32 s_voiceLevel;

s32 GetAngleDelta(s32 from, s32 to) {
    (void)from;
    (void)to;
    return 0;
}

s32 rsin(s32 angle) {
    return (angle & 0xFFF) == 0x400 ? 4096 : 0;
}

s32 rcos(s32 angle) {
    return (angle & 0xFFF) == 0 ? 4096 : 0;
}

void UpdateCarTravelVelocity(GameCarRuntime *car) {
    (void)car;
}

void SetIndexedEffectVoice(s32 index, s32 pitch, s32 level) {
    (void)pitch;
    s_voiceIndex = index;
    s_voiceLevel = level;
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
    GameCarSpec spec;
    PlayerCarRuntime car;

    memset(&spec, 0, sizeof(spec));
    memset(g_LaunchSpeedThresholds, 0, sizeof(g_LaunchSpeedThresholds));
    g_CarSpec = &spec;
    g_RacePhase = 2;
    g_LaunchSpeedThresholds[4].initial = 100;
    g_LaunchSpeedThresholds[4].sustain = 100;

    memset(&car, 0, sizeof(car));
    car.speed = 200;
    car.drive.launchThresholdIndex = -1;
    car.drive.acceleratorLatch = 1;
    car.drive.coastFrames = 2;
    car.drive.steeringGripResponse = 1000;
    StepDriving(&car);
    CHECK(car.drive.motionState == CAR_MOTION_TAKEOFF);
    CHECK(car.drive.launchEnergy == 400 && car.drive.coastFrames == 0);
    CHECK(s_voiceIndex == 0);

    memset(&car, 0, sizeof(car));
    car.speed = 200;
    car.headingAngle = 0x400;
    car.drive.launchThresholdIndex = -1;
    car.drive.brakeLatch = 1;
    StepDriving(&car);
    CHECK(car.drive.motionState == CAR_MOTION_TAKEOFF);
    CHECK(car.drive.launchEnergy == 10000);
    CHECK(car.drive.spinRate == -3200);

    memset(&car, 0, sizeof(car));
    car.drive.launchThresholdIndex = 8;
    car.drive.coastFrames = 4;
    StepDriving(&car);
    CHECK(car.drive.coastFrames == 5 && car.drive.launchEnergy == 0);

    memset(&car, 0, sizeof(car));
    spec.redline = 7000;
    spec.revLimit = 6000;
    spec.topGear = 6;
    car.drive.launchThresholdIndex = 0;
    car.drive.engineRpm = 9000;
    car.drive.gear = 5;
    car.drive.steerHoldFrames = 100;
    StepDriving(&car);
    CHECK(s_voiceIndex == -1);

    car.drive.gear = 6;
    StepDriving(&car);
    CHECK(s_voiceIndex == 2 && s_voiceLevel == 100);

    memset(&car, 0, sizeof(car));
    car.speed = INT_MAX;
    car.drive.acceleratorLatch = 1;
    car.drive.coastFrames = 2;
    StepDriving(&car);
    CHECK(car.drive.launchEnergy == -2);
    CHECK(car.drive.motionState != CAR_MOTION_TAKEOFF);

    puts("normal car driving tests passed");
    return 0;
}

s32 SinAngle(s32 angle) { return rsin(angle); }
s32 CosAngle(s32 angle) { return rcos(angle); }
