#include "game/car_drive.h"
#include "game/random.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static void Prepare(GameCarSpec *spec, CarPerformance *performance,
                    PlayerCarRuntime *car, int torqueScale) {
    memset(spec, 0, sizeof(*spec));
    memset(performance, 0, sizeof(*performance));
    memset(car, 0, sizeof(*car));
    spec->topGear = 6;
    spec->revLimit = 15000;
    spec->redline = 8000;
    spec->automaticAccelerationScale = 1000;
    spec->baseSteeringGrip = 100;
    spec->steeringGripResponse = 1000;
    spec->speedDragDivisor = 1000;
    for (int i = 0; i < CAR_TORQUE_CURVE_SAMPLE_COUNT; i++) {
        spec->torqueCurve[i] = (i + 1) * 20000 * torqueScale;
        spec->torqueBand.values[i] = i * 1000;
    }
    for (int i = 1; i <= CAR_FORWARD_GEAR_COUNT; i++) {
        spec->gearRatio[i] = 100;
        spec->torqueScale[i - 1] = 100;
    }
    PrepareCarPerformance(&car->drive, spec, performance);
    car->speed = 2000;
    car->drive.gear = 1;
    car->drive.gearDisp = 1;
    car->drive.manual = 1;
    car->drive.drivetrainCoupled = 1;
    car->drive.engineRpm = 1500;
    car->drive.drivetrainTorque = GetCarGearLoad(spec, 1) * 1500;
    car->drive.acceleratorInput.value = 256;
    car->drive.dragScale = 1000;
}

static void TestDynamics(void) {
    GameCarSpec spec, otherSpec;
    CarPerformance engine, otherEngine;
    PlayerCarRuntime first, second;
    const GameTrackPoint points[5] = {
        {.x = 0}, {.x = 500}, {.x = 1000}, {.z = 1000}, {.z = 500},
    };
    const TrackRoute route = {.points = points, .count = 5};
    const LaunchSpeedThreshold threshold = {.initial = 30000, .sustain = 30000};
    const DriveContext context = {.started = 1, .racing = 1};
    for (int state = CAR_MOTION_DRIVING; state <= CAR_MOTION_STANDING_START; state++) {
        Prepare(&spec, &engine, &first, 1);
        Prepare(&otherSpec, &otherEngine, &second, 2);
        first.drive.motionState = state;
        first.drive.launchDirection = 1;
        first.drive.launchEnergy = 100000;
        first.drive.standingStartSpin = 100;
        first.drive.jumpTimer = 10;
        PlayerCarRuntime reference = first;
        PlayerCarRuntime alone = first;
        u32 seed = 123, referenceSeed = seed, aloneSeed = seed, otherSeed = 456;
        StepCarDrivetrain(&reference, &spec, &engine, &context);
        switch (reference.drive.motionState) {
        case CAR_MOTION_DRIVING: StepCarDriving(&reference, &threshold); break;
        case CAR_MOTION_TAKEOFF: StepCarLaunch(&reference, &spec, &route); break;
        case CAR_MOTION_AIRBORNE: StepCarAirborne(&reference); break;
        case CAR_MOTION_STANDING_START: {
            s32 vertical = RandomNext(&referenceSeed);
            s32 lateral = RandomNext(&referenceSeed);
            StepCarStandingStart(&reference, vertical, lateral);
            break;
        }
        }
        if (reference.speed < CAR_STOPPED_SPEED_THRESHOLD) reference.headingAngle = reference.bodyYaw;
        StepCarDynamics(&first, &spec, &engine, &context, &route, &threshold, &seed);
        CHECK(memcmp(&first, &reference, sizeof(first)) == 0);
        CHECK(seed == referenceSeed);
        StepCarDynamics(&alone, &spec, &engine, &context, &route, &threshold, &aloneSeed);
        for (int i = 0; i < 20; i++) {
            StepCarDynamics(&second, &otherSpec, &otherEngine, &context, &route, &threshold, &otherSeed);
            StepCarDynamics(&first, &spec, &engine, &context, &route, &threshold, &seed);
            StepCarDynamics(&alone, &spec, &engine, &context, &route, &threshold, &aloneSeed);
        }
        CHECK(memcmp(&first, &alone, sizeof(first)) == 0 && seed == aloneSeed);
        PlayerCarRuntime saved = first;
        u32 savedSeed = seed;
        StepCarDynamics(&first, &spec, &engine, &context, &route, &threshold, &seed);
        StepCarDynamics(&second, &otherSpec, &otherEngine, &context, &route, &threshold, &otherSeed);
        StepCarDynamics(&saved, &spec, &engine, &context, &route, &threshold, &savedSeed);
        CHECK(memcmp(&first, &saved, sizeof(first)) == 0 && seed == savedSeed);
    }
    Prepare(&spec, &engine, &first, 1);
    first.drive.motionState = CAR_MOTION_STANDING_START;
    first.drive.standingStartSpin = 100;
    u32 seed = 123;
    const DriveContext countdown = {0};
    CHECK(StepCarDynamics(&first, &spec, &engine, &countdown, &route, &threshold, &seed) == 0);
    CHECK(seed == 123 && first.speed == 0);
    Prepare(&spec, &engine, &first, 1);
    first.drive.motionState = CAR_MOTION_AIRBORNE;
    first.drive.jumpTimer = 0;
    CHECK(StepCarDynamics(&first, &spec, &engine, &context, &route, &threshold, &seed) == 1);
    CHECK(first.drive.motionState == CAR_MOTION_DRIVING && seed == 123);
}

int main(void) {
    TestDynamics();
    GameCarSpec firstSpec, secondSpec;
    CarPerformance firstEngine, secondEngine;
    PlayerCarRuntime first, second;
    Prepare(&firstSpec, &firstEngine, &first, 1);
    Prepare(&secondSpec, &secondEngine, &second, 2);
    const DriveContext racing = {.racing = 1, .started = 1};
    PlayerCarRuntime alone = first;
    for (int i = 0; i < 20; i++) {
        StepCarDrivetrain(&alone, &firstSpec, &firstEngine, &racing);
    }
    for (int i = 0; i < 20; i++) {
        StepCarDrivetrain(&second, &secondSpec, &secondEngine, &racing);
        StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(first.drive.engineRpm != second.drive.engineRpm);
    CHECK(first.drive.acceleratorLatch == 2);
    CHECK(first.speed < 2000 && first.speed > 0);
    PlayerCarRuntime restored = first;
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    StepCarDrivetrain(&second, &secondSpec, &secondEngine, &racing);
    StepCarDrivetrain(&restored, &firstSpec, &firstEngine, &racing);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);

    first.drive.acceleratorInput.value = 123;
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    CHECK(first.drive.acceleratorLatch == 0);
    Prepare(&firstSpec, &firstEngine, &first, 1);
    first.headingAngle = 1000;
    first.bodyYaw = 2000;
    const DriveContext countdown = {0};
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &countdown);
    CHECK(first.speed == 0);
    CHECK(first.headingAngle == first.bodyYaw);
    CHECK(first.drive.gearDisp == first.drive.gear);
    Prepare(&firstSpec, &firstEngine, &first, 1);
    const DriveContext finished = {.started = 1};
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &finished);
    CHECK(first.speed == 1880);
    first.verticalMotionState = CAR_VERTICAL_RISING;
    first.speed = 2000;
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    CHECK(first.speed == 1998 && first.acceleration == 0);
    Prepare(&firstSpec, &firstEngine, &first, 1);
    first.drive.gear = 2;
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    CHECK(first.drive.gearDisp == 2 && first.drive.clutch == 10);
    StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    CHECK(first.drive.clutch == 9);
    for (int i = 0; i < 9; i++) {
        StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
    }
    CHECK(first.drive.clutch == 0 && first.drive.drivetrainCoupled == 1);

    Prepare(&firstSpec, &firstEngine, &first, 1);
    Prepare(&secondSpec, &secondEngine, &second, 2);
    second.drive.targetHeading = 0x400;
    const LaunchSpeedThreshold threshold = {960, 320};
    alone = first;
    for (int i = 0; i < 20; i++) {
        StepCarDrivetrain(&alone, &firstSpec, &firstEngine, &racing);
        StepCarDriving(&alone, &threshold);
    }
    for (int i = 0; i < 20; i++) {
        StepCarDrivetrain(&first, &firstSpec, &firstEngine, &racing);
        StepCarDriving(&first, &threshold);
        StepCarDrivetrain(&second, &secondSpec, &secondEngine, &racing);
        StepCarDriving(&second, &threshold);
    }
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    CHECK(first.bodyYaw != second.bodyYaw);
    CHECK(first.drive.brakePos != 0);
    restored = first;
    StepCarDriving(&second, &threshold);
    StepCarDriving(&first, &threshold);
    StepCarDriving(&restored, &threshold);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    return failures != 0;
}
