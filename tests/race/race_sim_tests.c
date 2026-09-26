#include "game/race_sim.h"
#include "game/car_catalog.h"
#include "game/car_track_internal.h"
#include "../car/driver_fixture.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    GameTrackPoint raised[3];
    memcpy(raised, points, sizeof(raised));
    for (int i = 0; i < 3; i++) raised[i].y = 50;
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    const TrackRoute otherRoute = {.points = raised, .count = 3, .length = 3000};
    const CarHullPoint hullPoints[6] = {
        {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16},
    };
    const CarHullPoint corners[4] = {{-26, 96}, {26, 96}, {-26, -16}, {26, -16}};
    const CarHullPoint roadCorners[4] = {{-15,20},{15,20},{-8,-10},{8,-10}};
    const DriverHull hull = {.points = hullPoints, .corners = corners};
    const LaunchSpeedThreshold threshold = {.initial = 30000, .sustain = 30000};
    const TrackRivalStart position = {.x = 200, .trackPointIndex = 0};
    PlayerCarRuntime fixture;
    GameCarSpec spec;
    CarPerformance engine = {0};
    PrepareDriver(&fixture, &spec, &engine);
    RaceSim first, second, alone;
    CHECK(InitRaceSim(&first, &route, NULL, 1, 0));
    CHECK(!StartRaceSim(&first, 0));
    CHECK(InitRaceSim(&second, &otherRoute, NULL, 1, 0));
    for (int i = 0; i < 2; i++) {
        CHECK(AddRaceDriver(&first, i, &spec, &hull, roadCorners, &threshold, &position, 0, 1, 23, 123 + i));
        CHECK(AddRaceDriver(&second, i, &spec, &hull, roadCorners, &threshold, &position, 0, 1, 23, 456 + i));
    }
    const DriverInput input = {.steering.mode = STEERING_DIGITAL, .throttle = 256};
    CHECK(SetRaceInput(&first, 0, &input) && SetRaceInput(&first, 1, &input));
    CHECK(SetRaceInput(&second, 0, &input) && SetRaceInput(&second, 1, &input));
    CHECK(StartRaceSim(&first, 2) && StartRaceSim(&second, 2));
    alone = first;
    PlayerCarRuntime before = first.drivers[0].car;
    PlayerCarRuntime expectedCountdown = before;
    ApplyDriverInput(&expectedCountdown, &first.drivers[0].spec, &input);
    const DriveContext countdownDrive = {.point = &points[before.trackPointIndex],
        .nextPoint = &points[(before.trackPointIndex + 1) % route.count],
        .digitalSteering = 1};
    StepCarDrivetrain(&expectedCountdown, &first.drivers[0].spec,
        &first.drivers[0].engine, &countdownDrive);

    for (int i = 0; i < 2; i++) {
        CHECK(StepRaceSim(&first) && StepRaceSim(&second) && StepRaceSim(&alone));
    }
    CHECK(first.phase == SIM_RACING && first.elapsed == 0);
    CHECK(memcmp(&expectedCountdown, &first.drivers[0].car, sizeof(expectedCountdown)) == 0);
    CHECK(first.drivers[0].car.x == before.x && first.drivers[0].car.z == before.z);
    CHECK(first.drivers[0].car.speed == 0);
    CHECK(first.drivers[0].car.drive.acceleratorInput.value == 256);
    CHECK(first.drivers[0].car.drive.engineRpm > before.drive.engineRpm);
    CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    before = first.drivers[0].car;

    CHECK(StepRaceSim(&first) && StepRaceSim(&alone));
    CHECK(memcmp(&before, &first.drivers[0].car, sizeof(before)) == 0);
    CHECK(first.elapsed == 1);
    for (int i = 0; i < 100; i++) {
        CHECK(StepRaceSim(&first) && StepRaceSim(&second) && StepRaceSim(&alone));
        CHECK(memcmp(&first, &alone, sizeof(first)) == 0);
    }
    CHECK(first.elapsed == 101);
    CHECK(first.drivers[0].car.collisionFlag == 1);
    CHECK(first.drivers[0].car.x != before.x);
    CHECK(first.drivers[0].car.y == 0 && second.drivers[0].car.y == 50);
    CHECK(first.drivers[0].stepTick == first.tick - 1);
    CHECK(first.drivers[0].crashed == 1);
    /* A landing result survives an intermediate 50 Hz tick, and the next
     * physics step replaces it. Consumers need only remember stepTick. */
    RaceSim landing = alone;
    CHECK(RetireRaceDriver(&landing, 1));
    PlayerCarRuntime *landingCar = &landing.drivers[0].car;
    landingCar->verticalMotionState = CAR_VERTICAL_FALLING;
    landingCar->verticalMotionTimer = 18;
    landingCar->verticalMotionRate = 32;
    landingCar->y = 0;
    CHECK(StepRaceSim(&landing));
    CHECK(landing.drivers[0].stepTick == landing.tick);
    CHECK(landing.drivers[0].step.landingFrames == 19);
    const DriverStep landed = landing.drivers[0].step;
    const u32 landedTick = landing.drivers[0].stepTick;
    CHECK(StepRaceSim(&landing));
    CHECK(landing.drivers[0].stepTick == landedTick);
    CHECK(memcmp(&landing.drivers[0].step, &landed, sizeof(landed)) == 0);
    CHECK(StepRaceSim(&landing));
    CHECK(landing.drivers[0].stepTick == landedTick + 2);
    CHECK(landing.drivers[0].step.landingFrames == 0);
    RaceSim restored = first;
    CHECK(StepRaceSim(&first) && StepRaceSim(&second) && StepRaceSim(&restored));
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    DriverInput edge = input;
    edge.shiftUp = 1;
    CHECK(SetRaceInput(&first, 0, &edge));
    CHECK(SetRaceInput(&first, 0, &input));
    CHECK(first.drivers[0].input.shiftUp == 1);
    restored = first; /* Snapshot includes the pending edge, before physics. */
    CHECK(StepRaceSim(&first)); /* Odd clock tick preserves the edge. */
    CHECK(StepRaceSim(&second) && StepRaceSim(&restored));
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    CHECK(first.drivers[0].input.shiftUp == 1);
    CHECK(StepRaceSim(&first));
    CHECK(StepRaceSim(&second) && StepRaceSim(&restored));
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    CHECK(first.drivers[0].input.shiftUp == 0);
    /* Resuming the copied context with subsequent inputs reproduces both
     * drivers, collisions, lap clocks and RNG, even with another room stepped
     * between executions. No renderer or presentation state is restored. */
    for (int tick = 0; tick < 20; tick++) {
        DriverInput next = input;
        next.throttle = tick % 3 == 0 ? 0 : 256;
        next.brake = tick % 3 == 0 ? 128 : 0;
        next.shiftUp = tick == 2;
        next.shiftDown = tick == 12;
        CHECK(SetRaceInput(&first, 0, &next) && SetRaceInput(&restored, 0, &next));
        CHECK(StepRaceSim(&first) && StepRaceSim(&second) && StepRaceSim(&restored));
        CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    }
    edge.throttle = 257;
    restored = first;
    CHECK(!SetRaceInput(&first, 0, &edge));
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    /* Lifecycle fixture at the final accumulated-distance boundary. */
    CHECK(RetireRaceDriver(&first, 1));
    first.drivers[0].car.lap = 1;
    first.drivers[0].car.progressA = 6000;
    first.drivers[0].lapStarted = 2;
    if (first.elapsed % 2 == 0) CHECK(StepRaceSim(&first));
    CHECK(StepRaceSim(&first));
    CHECK(first.phase == SIM_FINISHED && first.finishCount == 1);
    CHECK(first.drivers[0].status == SIM_DRIVER_FINISHED && first.drivers[0].place == 1);
    CHECK(first.drivers[1].status == SIM_RETIRED);
    CHECK(first.drivers[0].finishTick == first.elapsed);
    CHECK(first.drivers[0].lapTicks[0] == first.elapsed - 2);
    restored = first;
    CHECK(!StepRaceSim(&first));
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    /* A failed segment search deactivates the car. The race owns retirement,
     * including the case where the last remaining participant leaves track. */
    RaceSim lost = alone;
    CHECK(RetireRaceDriver(&lost, 1));
    MoveCarTrackProgress(AsRivalCar(&lost.drivers[0].car), &lost.route, -1, lost.reverse);
    CHECK(lost.drivers[0].car.activeFlag == -1);
    const PlayerCarRuntime lostCar = lost.drivers[0].car;
    for (int i = 0; i < SIM_PHYSICS_INTERVAL; i++) {
        if (lost.phase != SIM_FINISHED) CHECK(StepRaceSim(&lost));
    }
    CHECK(lost.phase == SIM_FINISHED && lost.finishCount == 0);
    CHECK(lost.drivers[0].status == SIM_RETIRED);
    CHECK(lost.drivers[0].finishTick == 0 && lost.drivers[0].place == 0);
    CHECK(memcmp(&lostCar, &lost.drivers[0].car, sizeof(lostCar)) == 0);
    CHECK(!SetRaceInput(&lost, 0, &input));
    /* Disconnects during countdown do not wait for an empty grid to start.
     * A remaining driver still gets the normal countdown and drivetrain. */
    RaceSim countdown;
    CHECK(InitRaceSim(&countdown, &route, NULL, 1, 0));
    for (int i = 0; i < 2; i++) {
        CHECK(AddRaceDriver(&countdown, i, &spec, &hull, roadCorners,
            &threshold, &position, 0, 1, 23, 123 + i));
        CHECK(SetRaceInput(&countdown, i, &input));
    }
    CHECK(StartRaceSim(&countdown, 50));
    CHECK(RetireRaceDriver(&countdown, 0));
    const SimDriver retired = countdown.drivers[0];
    CHECK(StepRaceSim(&countdown) && StepRaceSim(&countdown));
    CHECK(countdown.phase == SIM_COUNTDOWN && countdown.countdown == 48);
    CHECK(countdown.drivers[1].car.drive.engineRpm > 0);
    CHECK(memcmp(&retired, &countdown.drivers[0], sizeof(retired)) == 0);
    CHECK(RetireRaceDriver(&countdown, 1));
    const u32 remaining = countdown.countdown;
    CHECK(StepRaceSim(&countdown));
    CHECK(countdown.phase == SIM_FINISHED && countdown.countdown == remaining);
    CHECK(countdown.elapsed == 0 && countdown.finishCount == 0);
    restored = countdown;
    CHECK(!StepRaceSim(&countdown));
    CHECK(memcmp(&restored, &countdown, sizeof(countdown)) == 0);
    /* Configured automatic shift points travel through real seat setup and
     * physics, without a menu/global spec or injected shift button. */
    GameCarSpec automaticSpec = spec;
    RageCarCatalog catalog;
    char error[128];
    CHECK(CarCatalogParse("[[cars]]\nid = \"test\"\nmodel = 0\ngrade = 0\n"
        "automatic_acceleration_scale = 900\n"
        "shift_points = [0,1000,800,2000,1600,3000,2400,4000,3200,5000,4000,6000]\n",
        &catalog, error, sizeof(error)));
    const RageCarCatalogEntry configured = catalog.entries[0];
    ApplyCarSpec(&configured, &automaticSpec);
    CHECK(automaticSpec.automaticAccelerationScale == 900);
    CHECK(automaticSpec.redline == spec.redline && automaticSpec.revLimit == spec.revLimit);
    CHECK(memcmp(automaticSpec.torqueCurve, spec.torqueCurve, sizeof(spec.torqueCurve)) == 0);
    const GameCarSpec overridden = automaticSpec;
    ApplyCarSpec(NULL, &automaticSpec);
    ApplyCarSpec(&configured, NULL);
    CHECK(memcmp(&automaticSpec, &overridden, sizeof(overridden)) == 0);
    RaceSim automatic, manual;
    CHECK(InitRaceSim(&automatic, &route, NULL, 1, 0));
    CHECK(InitRaceSim(&manual, &route, NULL, 1, 0));
    CHECK(AddRaceDriver(&automatic, 0, &automaticSpec, &hull, roadCorners,
        &threshold, &position, 0, 0, 23, 123));
    CHECK(AddRaceDriver(&manual, 0, &automaticSpec, &hull, roadCorners,
        &threshold, &position, 0, 1, 23, 123));
    CHECK(automatic.drivers[0].car.drive.manual == 0);
    /* Boundary fixture: both cars arrive just above the first upshift speed. */
    automatic.drivers[0].car.speed = manual.drivers[0].car.speed = 1001;
    CHECK(SetRaceInput(&automatic, 0, &input) && SetRaceInput(&manual, 0, &input));
    CHECK(StartRaceSim(&automatic, 0) && StartRaceSim(&manual, 0));
    for (int tick = 0; tick < SIM_PHYSICS_INTERVAL; tick++) {
        CHECK(StepRaceSim(&automatic) && StepRaceSim(&manual));
    }
    CHECK(automatic.drivers[0].car.drive.gear == 2);
    CHECK(manual.drivers[0].car.drive.gear == 1);
    CHECK(automatic.drivers[0].car.drive.autoShiftCooldown > 0);
    CHECK(memcmp(&automatic.drivers[0].spec, &manual.drivers[0].spec, sizeof(GameCarSpec)) == 0);
    return 0;
}
