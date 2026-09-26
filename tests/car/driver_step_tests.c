#include "driver_fixture.h"
#include "game/car_motion_internal.h"
#include "game/car_track_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
    };
    GameTrackPoint raised[3];
    memcpy(raised, points, sizeof(raised));
    for (int i = 0; i < 3; i++) raised[i].y = 50;
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    const TrackRoute otherRoute = {.points = raised, .count = 3, .length = 3000};
    const CarHullPoint corners[4] = {{-8, -8}, {8, -8}, {-8, 8}, {8, 8}};
    const LaunchSpeedThreshold threshold = {.initial = 30000, .sustain = 30000};
    GameCarSpec spec, otherSpec;
    CarPerformance engine = {0}, otherEngine = {0};
    PlayerCarRuntime first, second;
    PrepareDriver(&first, &spec, &engine);
    PrepareDriver(&second, &otherSpec, &otherEngine);
    DriverContext context = {.spec = &spec, .performance = &engine, .route = &route,
        .launchThreshold = &threshold, .corners = corners,
        .drive = {.racing = 1, .started = 1, .digitalSteering = 1}};
    DriverContext other = context;
    other.spec = &otherSpec;
    other.performance = &otherEngine;
    other.route = &otherRoute;
    const DriverInput input = {.steering.mode = STEERING_DIGITAL, .throttle = 256};
    PlayerCarRuntime alone = first;
    u32 seed = 123, otherSeed = 456, aloneSeed = seed;
    for (int i = 0; i < 20; i++) {
        DriverStep a = MoveDriver(&first, &input, &context, &seed);
        DriverStep b = MoveDriver(&second, &input, &other, &otherSeed);
        FinishDriver(&first, &context, &seed, 0, &a);
        FinishDriver(&second, &other, &otherSeed, 0, &b);
        DriverStep c = MoveDriver(&alone, &input, &context, &aloneSeed);
        FinishDriver(&alone, &context, &aloneSeed, 0, &c);
        CHECK(memcmp(&first, &alone, sizeof(first)) == 0 && seed == aloneSeed);
        CHECK(a.skid == 0 && a.skidAngle == -1);
    }
    CHECK(first.x != 200 && first.trackProgress != 0);
    CHECK(first.y == 0 && second.y == 50);
    PlayerCarRuntime saved = first;
    u32 savedSeed = seed;
    DriverStep a = MoveDriver(&first, &input, &context, &seed);
    DriverStep b = MoveDriver(&second, &input, &other, &otherSeed);
    DriverStep c = MoveDriver(&saved, &input, &context, &savedSeed);
    FinishDriver(&first, &context, &seed, 1, &a);
    FinishDriver(&second, &other, &otherSeed, 0, &b);
    FinishDriver(&saved, &context, &savedSeed, 1, &c);
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0 && seed == savedSeed);
    CHECK(first.motionModeTimer == CAR_BODY_KICK_DURATION);
    /* Landing audio consumes the duration reported by physics, rather than
     * reading a car timer after subsequent pose/crest updates. */
    PrepareDriver(&first, &spec, &engine);
    first.verticalMotionState = CAR_VERTICAL_FALLING;
    first.verticalMotionTimer = 18;
    first.verticalMotionRate = 32;
    first.y = 0;
    a = (DriverStep){.skidAngle = -1};
    FinishDriver(&first, &context, &seed, 0, &a);
    CHECK(first.verticalMotionState == CAR_VERTICAL_GROUNDED);
    CHECK(a.landingFrames == 19);
    FinishDriver(&first, &context, &seed, 0, &a);
    CHECK(a.landingFrames == 0);
    /* The client may inspect engine state for sound between these steps.
     * Splitting there must preserve the server's combined physics and RNG. */
    for (int started = 0; started <= 1; started++) {
        for (int motion = CAR_MOTION_DRIVING; motion <= CAR_MOTION_STANDING_START; motion++) {
            for (int spin = 10; spin <= 11; spin++) {
                PrepareDriver(&first, &spec, &engine);
                first.drive.motionState = motion;
                first.drive.standingStartSpin = spin;
                second = first;
                seed = otherSeed = 123;
                DriveContext drive = {.point = &points[0], .nextPoint = &points[1],
                    .racing = started, .started = started, .digitalSteering = 1};
                const int combined = StepCarDynamics(&first, &spec, &engine,
                    &drive, &route, &threshold, &seed);
                StepCarDrivetrain(&second, &spec, &engine, &drive);
                const int split = started
                    ? StepCarMotion(&second, &spec, &route, &threshold, &otherSeed) : 0;
                if (second.speed < CAR_STOPPED_SPEED_THRESHOLD) second.headingAngle = second.bodyYaw;
                CHECK(combined == split && seed == otherSeed);
                CHECK(memcmp(&first, &second, sizeof(first)) == 0);
                if (!started) CHECK(seed == 123);
            }
        }
    }
    return 0;
}
