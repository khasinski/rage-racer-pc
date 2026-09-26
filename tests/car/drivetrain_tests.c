/*
 * The parts of the drivetrain a race does not reach.
 *
 * The drivetrain used to be one 800-line function. It is split into named
 * stages now, but smoke runs still only get the car into first or second gear
 * on a flat piece of track. That leaves the tall-gear grade penalty and the
 * mid-shift interpolation for these focused cases.
 *
 * The numbers are what the shipped code produces; they are here to catch a
 * branch changing, so a deliberate change means updating them on purpose.
 */

#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track.h"
#include "game/race.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

void UpdateCarDrivetrain(PlayerCarRuntime *carArg);

/* The tables the drivetrain reads. */
GameCarSpec *g_CarSpec;
CarPerformance g_CarPerformance;
const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
const GameTrackArcCenter *g_TrackArcCenters;
s16 g_RacePhase;
u8 g_PadType;
u32 g_RandomSeed;
LaunchSpeedThreshold g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];

int StepCarMotion(PlayerCarRuntime *car, const GameCarSpec *spec,
                  const TrackRoute *route, const LaunchSpeedThreshold *threshold,
                  u32 *random) {
    (void)car; (void)spec; (void)route; (void)threshold; (void)random;
    return 0;
}
void SetIndexedEffectVoice(s32 voice, s32 pitch, s32 level) {
    (void)voice; (void)pitch; (void)level;
}

/* Where the drivetrain hands off once it has worked out the forces. What
 * those do with the result is their own business, not this test's. */
static int s_drivingCalls;
static int s_launchCalls;
static int s_airborneCalls;
static int s_standingStartCalls;

void PlayCarDrivingVoice(const PlayerCarRuntime *car, const GameCarSpec *spec) {
    (void)spec;
    (void)car;
    s_drivingCalls++;
}
void PlayCarLaunchVoice(const PlayerCarRuntime *car) {
    (void)car;
    s_launchCalls++;
}
void PlayCarAirborneVoice(const PlayerCarRuntime *car) {
    (void)car;
    s_airborneCalls++;
}
void PlayCarStandingStartVoice(const PlayerCarRuntime *car) {
    (void)car;
    s_standingStartCalls++;
}

s32 Atan2(s32 x, s32 y) { (void)x; (void)y; return 0; }
s32 GetAngleDistance(s32 a, s32 b) { (void)a; (void)b; return 0; }
s32 rsin(s32 angle) { (void)angle; return 0; }
s32 rcos(s32 angle) { (void)angle; return 0x1000; }

static GameCarSpec s_spec;
static GameTrackPoint s_points[4];
static GameTrackArcCenter s_arcs[4];
static PlayerCarRuntime s_car;
static int s_failures;

static void Check(int condition, const char *what, s32 got, s32 wanted) {
    if (condition) return;
    printf("FAIL %s: got %d, expected %d\n", what, got, wanted);
    s_failures++;
}

/*
 * A car whose torque curve is a straight line, so anything the interpolation
 * returns can be read off by hand. One band per gear covering the whole rev
 * range, and a loss curve of the same shape.
 */
static void BuildSpec(void) {
    int i;

    memset(&s_spec, 0, sizeof(s_spec));
    memset(g_CarPerformance.curves, 0, sizeof(g_CarPerformance.curves));

    s_spec.topGear = 6;
    s_spec.redline = 8000;
    s_spec.revLimit = 9000;
    s_spec.automaticAccelerationScale = 1000;
    s_spec.referenceTurnRadius = 100;
    for (i = 0; i < 6; i++) {
        s_spec.gearLoad[i] = 100 + i * 20;
    }
    for (i = 0; i < 7; i++) {
        s_spec.gearRatio[i] = 1000;
    }
    for (i = 0; i < 6; i++) {
        s_spec.shiftPoints[i].upshiftSpeed = 1000 * (i + 1);
        s_spec.shiftPoints[i].downshiftSpeed = 500 * (i + 1);
    }

    /*
     * One thousand rpm per slot, and each rev band covering the two slots
     * around it, which is the layout the walk expects: the band's start index
     * is the previous band's end minus one.
     */
    for (i = 0; i < 16; i++) {
        s_spec.torqueBand.values[i] = i * 1000;
    }
    for (i = 0; i < 8; i++) {
        g_CarPerformance.torqueBands[i] = (s16)(i + 2);
        g_CarPerformance.lossBands[i] = (s16)(i + 2);
    }
    for (i = 0; i < 9; i++) {
        s_spec.torqueLossRpm[i] = i * 1000;
    }
    for (i = 0; i < 10; i++) {
        s_spec.torqueLossValue[i] = i * 10;
    }
    for (i = 0; i <= CAR_FORWARD_GEAR_COUNT; i++) {
        int slot;
        for (slot = 0; slot < 16; slot++) {
            /* Each gear pulls differently, so reading the wrong gear's
             * curve is visible. */
            g_CarPerformance.curves[i].values[slot] = slot * 1000 * (i + 1);
        }
    }
    g_CarSpec = &s_spec;
}

static void PlaceCar(void) {
    memset(&s_points, 0, sizeof(s_points));
    memset(&s_arcs, 0, sizeof(s_arcs));
    g_TrackPoints = s_points;
    g_TrackArcCenters = s_arcs;
    g_TrackPointCount = 4;

    memset(&s_car, 0, sizeof(s_car));
    s_car.speed = 2000;
    s_car.drive.gear = 1;
    s_car.drive.gearDisp = 1;
    s_car.drive.engineRpm = 3000;
    s_car.drive.acceleratorInput.value = 0xFF;
    s_car.drive.drivetrainCoupled = 1;

    g_RacePhase = 2;
    s_car.drive.roadGrade = 0;
    g_PadType = 0;
    s_car.drive.shiftTargetRpm = 0;
    s_car.drive.shiftTargetSpeed = 0;
    s_drivingCalls = 0;
    s_launchCalls = 0;
    s_airborneCalls = 0;
    s_standingStartCalls = 0;
}

/*
 * The curve bias: the car carries the curve it thinks it is on and the track
 * point carries the curve it is really on. Agreeing winds the bias up twice as
 * fast as disagreeing unwinds it, and a car on no curve leaves it alone.
 */
static void CurveBiasTests(void) {
    BuildSpec();

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = 0;
    s_points[0].arcRef = 1;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.trackCurveBias == 2, "agreeing winds the bias up",
          s_car.drive.trackCurveBias, 2);

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = 10;
    s_points[0].arcRef = 2;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.trackCurveBias == 9, "disagreeing unwinds it",
          s_car.drive.trackCurveBias, 9);

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 0;
    s_car.drive.trackCurveBias = 10;
    s_points[0].arcRef = 2;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.trackCurveBias == 10, "no curve leaves it alone",
          s_car.drive.trackCurveBias, 10);

    /* And it is held inside its limits at both ends. */
    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = 0x1E;
    s_points[0].arcRef = 1;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.trackCurveBias == 0x1E, "bias clamped at the top",
          s_car.drive.trackCurveBias, 0x1E);

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = -0x1E;
    s_points[0].arcRef = 2;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.trackCurveBias == -0x1E, "bias clamped at the bottom",
          s_car.drive.trackCurveBias, -0x1E);
}

/*
 * Mid-shift: the timer counts down and the engine speed is dragged from where
 * it is towards where the new gear will put it, in proportion to how much of
 * the shift is left. Once the timer reaches zero the ordinary throttle path
 * takes the engine speed back over, so only a shift still in progress can be
 * read off directly.
 */
static void ShiftInterpolationTests(void) {
    s32 early, late;

    BuildSpec();

    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 10;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 3; /* not shifting: the target is left alone */
    s_car.drive.shiftRpmDelta = 200;
    s_car.drive.shiftTargetRpm = 5000;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.jumpTimer == 9, "the shift timer runs down",
          s_car.drive.jumpTimer, 9);
    Check(s_car.drive.engineRpm == 5000 + (200 * 9) / 20,
          "engine speed drags towards the target", s_car.drive.engineRpm,
          5000 + (200 * 9) / 20);
    early = s_car.drive.engineRpm;

    /* Further into the shift, closer to the target. */
    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 3;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 3;
    s_car.drive.shiftRpmDelta = 200;
    s_car.drive.shiftTargetRpm = 5000;
    UpdateCarDrivetrain(&s_car);
    late = s_car.drive.engineRpm;
    if (!(late < early && late > 5000)) {
        printf("FAIL the shift does not converge: early=%d late=%d target=%d\n",
               early, late, 5000);
        s_failures++;
    }

    /* A timer already at the end never goes negative. */
    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 0;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 3;
    s_car.drive.shiftTargetRpm = 5000;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.jumpTimer == 0, "the timer does not go negative",
          s_car.drive.jumpTimer, 0);

    /* While the displayed gear still lags the real one, the target is worked
     * out afresh from road speed and the new gear's ratio. */
    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 10;
    s_car.speed = 5000;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 2;
    s_car.drive.shiftTargetRpm = 0;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.shiftTargetRpm == (((5000 * 0xA0) / 1168) * 0x2710) / 1000,
          "the shift target follows road speed", s_car.drive.shiftTargetRpm,
          (((5000 * 0xA0) / 1168) * 0x2710) / 1000);

    /* With the box caught up it is left where it was. */
    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 10;
    s_car.speed = 5000;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 3;
    s_car.drive.shiftTargetRpm = 1234;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.shiftTargetRpm == 1234, "a caught-up box keeps its target",
          s_car.drive.shiftTargetRpm, 1234);
}

/*
 * The tall-gear grade penalty: climbing with a manual box that is still
 * catching up takes load off the engine, and the taller the gear the more of
 * it. Below fourth nothing is taken off at all.
 */
static void GradePenaltyTests(void) {
    s32 third, fourth, fifth, sixth, level, downhill;

    BuildSpec();

    /* The whole block sits behind the manual-gearbox flag. */
    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    s_car.drive.roadGrade = 0;
    UpdateCarDrivetrain(&s_car);
    level = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 2;
    s_car.drive.roadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    third = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 4;
    s_car.drive.gearDisp = 3;
    s_car.drive.roadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    fourth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 5;
    s_car.drive.gearDisp = 4;
    s_car.drive.roadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    fifth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    s_car.drive.roadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    sixth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    s_car.drive.roadGrade = 1200;
    UpdateCarDrivetrain(&s_car);
    downhill = s_car.drive.engineLoad;

    Check(third == level, "third takes no penalty at all", third, level);
    Check(downhill == level, "downhill takes nothing off", downhill, level);
    if (!(fourth < level && fifth < fourth && sixth < fifth)) {
        printf("FAIL grade penalty does not grow with the gear: "
               "level=%d 4th=%d 5th=%d 6th=%d\n", level, fourth, fifth, sixth);
        s_failures++;
    }
}

/* The car is handed on to exactly one motion handler, and the torque walk
 * gives a different answer at different engine speeds rather than falling out
 * of its loop with nothing. */
static void TorqueBandTests(void) {
    s32 slow, fast;

    BuildSpec();

    PlaceCar();
    s_car.drive.engineRpm = 1000;
    UpdateCarDrivetrain(&s_car);
    Check(s_drivingCalls + s_launchCalls + s_airborneCalls +
                  s_standingStartCalls ==
              1,
          "the drivetrain hands the car on exactly once",
          s_drivingCalls + s_launchCalls + s_airborneCalls +
              s_standingStartCalls,
          1);
    slow = s_car.acceleration;

    PlaceCar();
    s_car.drive.engineRpm = 7000;
    UpdateCarDrivetrain(&s_car);
    fast = s_car.acceleration;

    if (slow == fast) {
        printf("FAIL the torque walk gave the same answer at 1000 and 7000 "
               "rpm: %d\n", slow);
        s_failures++;
    }

    /*
     * The torque a given engine speed produces, pinned. The walk is two
     * interpolations and a scaling, and each gear has its own curve, so a
     * change to any of those moves these numbers.
     */
    {
        static const struct {
            s32 rpm;
            s32 wanted;
        } cases[] = {{5000, 4}, {5500, 13}, {6000, 13}};
        size_t i;

        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
            char what[64];

            PlaceCar();
            s_car.speed = 8000;
            s_car.drive.drivetrainTorque = -200000;
            s_car.drive.engineRpm = cases[i].rpm;
            UpdateCarDrivetrain(&s_car);
            sprintf(what, "torque at %d rpm", cases[i].rpm);
            Check(s_car.acceleration == cases[i].wanted, what,
                  s_car.acceleration, cases[i].wanted);
        }
    }

    /* An empty band deliberately falls back to wheel torque minus drivetrain
     * load instead of inventing a zero interpolation. */
    BuildSpec();
    g_CarPerformance.torqueBands[2] = 4;
    g_CarPerformance.torqueBands[3] = 4;
    PlaceCar();
    s_car.drive.engineRpm = 3500;
    s_car.drive.drivetrainTorque = -200000;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.acceleration == 0, "empty torque band fallback",
          s_car.acceleration, 0);
    Check(s_car.drive.engineRpm == 0, "empty torque band rpm fallback",
          s_car.drive.engineRpm, 0);

    {
        s32 netTorque = 123;
        s32 bandScale = -1;

        BuildSpec();
        PlaceCar();
        s_car.drive.engineRpm = -1000;
        ReadCarEngineTorque(&s_car.drive, &s_spec, &g_CarPerformance,
                            g_CarPerformance.curves[1].values,
                            &netTorque, &bandScale);
        Check(netTorque == 123, "negative RPM uses the first torque band",
              netTorque, 123);
        Check(bandScale == 0, "negative RPM has no engine braking",
              bandScale, 0);
    }

}

/*
 * The retail car packs use both 985/990 and the small values 3/6 at +0x102.
 * This pins what the current PC implementation actually does with them.  It
 * is deliberately a behavioural test, not evidence that +0x102's recovered
 * unit is correct.  Retail stores 3 and 6 in several manual-only packs, so
 * they must remain literal per-thousand values should a mod enable automatic
 * for one of those cars.
 */
static s32 DriveWithAutomaticScale(s16 scale, s16 manual) {
    BuildSpec();
    PlaceCar();
    s_car.speed = 8000;
    s_car.drive.engineRpm = 5000;
    s_car.drive.drivetrainTorque = -200000;
    s_car.drive.manual = manual;
    s_spec.automaticAccelerationScale = scale;
    UpdateCarDrivetrain(&s_car);
    return s_car.acceleration;
}

static void AutomaticScaleRetailValueTests(void) {
    s32 manual = DriveWithAutomaticScale(1000, 1);
    s32 scale990 = DriveWithAutomaticScale(990, 0);
    s32 scale985 = DriveWithAutomaticScale(985, 0);
    s32 retail6 = DriveWithAutomaticScale(6, 0);
    s32 retail3 = DriveWithAutomaticScale(3, 0);

    Check(manual > 0, "manual reference acceleration is positive", manual, 1);
    Check(scale990 > 0 && scale985 > 0,
          "ordinary automatic retail scales retain acceleration", scale990,
          1);
    Check(retail6 == manual * 6 / 1000,
          "retail value 6 follows the current per-thousand path", retail6,
          manual * 6 / 1000);
    Check(retail3 == manual * 3 / 1000,
          "retail value 3 follows the current per-thousand path", retail3,
          manual * 3 / 1000);
    if (!(retail6 <= scale985 / 50 && retail3 <= scale985 / 50)) {
        printf("FAIL small retail scales unexpectedly retain pull: "
               "manual=%d 990=%d 985=%d 6=%d 3=%d\n", manual, scale990,
               scale985, retail6, retail3);
        s_failures++;
    }
}

static void GearBoundsTests(void) {
    BuildSpec();
    PlaceCar();
    s_car.drive.gear = 0;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.gear == 1, "gear below first is repaired",
          s_car.drive.gear, 1);

    BuildSpec();
    PlaceCar();
    s_car.drive.gear = 7;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.gear == 6, "gear above sixth is repaired",
          s_car.drive.gear, 6);

    BuildSpec();
    PlaceCar();
    s_car.drive.dragScale = 0;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.dragScale == 1000, "zero drag scale is repaired",
          s_car.drive.dragScale, 1000);

    BuildSpec();
    PlaceCar();
    s_spec.gearRatio[1] = 0;
    s_car.drive.motionState = CAR_MOTION_AIRBORNE;
    s_car.drive.jumpTimer = 10;
    s_car.drive.gearDisp = 2;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.shiftTargetRpm == (((2000 * 0xA0) / 1168) * 0x2710),
          "zero shift ratio uses a unit divisor", s_car.drive.shiftTargetRpm,
          (((2000 * 0xA0) / 1168) * 0x2710));
}

static void MissingTrackTests(void) {
    const DriveContext context = {0};
    CarDrivetrainLoads loads;

    BuildSpec();
    PlaceCar();
    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    s_car.drive.steeringGrip = 20;
    s_car.drive.steeringGripResponse = 1000;
    UpdateCarSteeringGrip(&s_car, &s_spec, &context, 100);
    Check(s_car.drive.steeringGrip == 60,
          "missing track keeps neutral steering grip",
          s_car.drive.steeringGrip, 60);

    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = 7;
    UpdateCarSteeringGrip(&s_car, &s_spec, &context, 0);
    Check(s_car.drive.trackCurveBias == 7,
          "missing track does not change curve bias",
          s_car.drive.trackCurveBias, 7);

    s_car.drive.roadGrade = 123;
    loads = CalculateCarDrivetrainLoads(&s_car, &s_spec, &context, 0, 0, 0);
    (void)loads;
    Check(s_car.drive.roadGrade == 0, "missing track clears road grade",
          s_car.drive.roadGrade, 0);
}

static void ExtremeLoadArithmeticTests(void) {
    const DriveContext context = {.racing = 1, .digitalSteering = 1};
    CarDrivetrainLoads loads;

    BuildSpec();
    PlaceCar();
    s_car.speed = INT_MAX;
    s_car.drive.engineRpm = INT_MAX;
    s_car.drive.acceleratorInput.value = 0x100;
    s_car.drive.drivetrainCoupled = 1;
    s_car.drive.steeringGrip = INT16_MAX;
    s_car.drive.steeringGripResponse = INT_MAX;
    s_spec.speedDragDivisor = 1;
    s_spec.negconSteeringAssistScale = INT16_MAX;
    s_car.drive.driveBoostTimer = INT_MAX;
    s_car.drive.dragScale = 1;

    loads = CalculateCarDrivetrainLoads(
        &s_car, &s_spec, &context, INT_MAX, INT_MAX, INT_MAX);
    (void)loads;
    Check(s_car.drive.dragScale == 1000, "extreme loads reset drag scale",
          s_car.drive.dragScale, 1000);
    Check(s_car.drive.driveBoostTimer == INT_MAX - 1,
          "extreme loads advance boost timer", s_car.drive.driveBoostTimer,
          INT_MAX - 1);
}

static void ExtremeTorqueArithmeticTests(void) {
    s32 netTorque;
    s32 bandScale;

    BuildSpec();
    PlaceCar();
    s_car.drive.engineRpm = INT_MAX;
    netTorque = 0;
    bandScale = -1;
    ReadCarEngineTorque(&s_car.drive, &s_spec, &g_CarPerformance,
                        g_CarPerformance.curves[1].values,
                        &netTorque, &bandScale);
    Check(netTorque == 7200, "rev limiter keeps 32-bit arithmetic",
          netTorque, 7200);
    Check(bandScale == 0, "rev limiter clears engine braking",
          bandScale, 0);

    BuildSpec();
    PlaceCar();
    s_car.drive.engineRpm = 0;
    s_spec.torqueBand.values[0] = INT_MIN;
    s_spec.torqueBand.values[1] = INT_MAX;
    s_spec.torqueLossRpm[0] = INT_MIN;
    s_spec.torqueLossRpm[1] = INT_MAX;
    g_CarPerformance.curves[1].values[0] = INT_MAX;
    g_CarPerformance.curves[1].values[1] = INT_MAX;
    s_spec.torqueLossValue[0] = INT_MAX;
    s_spec.torqueLossValue[1] = INT_MAX;
    g_CarPerformance.torqueBands[0] = 1;
    g_CarPerformance.lossBands[0] = 1;
    netTorque = 123;
    bandScale = -1;
    ReadCarEngineTorque(&s_car.drive, &s_spec, &g_CarPerformance,
                        g_CarPerformance.curves[1].values,
                        &netTorque, &bandScale);
    Check(netTorque == 0, "torque interpolation wraps its products",
          netTorque, 0);
    Check(bandScale == 0, "braking interpolation wraps its products",
          bandScale, 0);

    memset(&s_car.drive, 0, sizeof(s_car.drive));
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.engineRpm = INT_MAX;
    s_car.drive.drivetrainTorque = INT_MIN;
    Check(CalculateCarInitialAcceleration(&s_car.drive, INT_MAX) == -524287,
          "initial acceleration wraps gear torque and load",
          CalculateCarInitialAcceleration(&s_car.drive, INT_MAX), -524287);
}

static void ExtremeDrivetrainArithmeticTests(void) {
    BuildSpec();
    PlaceCar();
    s_spec.gearLoad[1] = INT_MAX;
    s_spec.speedDragDivisor = 1;
    s_car.speed = INT_MAX;
    s_car.drive.engineRpm = INT_MAX;
    s_car.drive.drivetrainTorque = INT_MIN;
    s_car.drive.acceleratorInput.value = 0x100;
    s_car.drive.brakeInput = INT16_MAX;
    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.dragScale = 1;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.drive.engineRpm >= 0 &&
              s_car.drive.engineRpm <= 0x3A98,
          "extreme takeoff clamps engine RPM", s_car.drive.engineRpm,
          0x3A98);
    Check(s_launchCalls == 1, "extreme takeoff reaches launch motion",
          s_launchCalls, 1);

    BuildSpec();
    PlaceCar();
    s_spec.gearLoad[1] = INT_MAX;
    s_car.speed = INT_MAX;
    s_car.verticalMotionState = CAR_VERTICAL_RISING;
    s_car.drive.engineRpm = INT_MAX;
    s_car.drive.drivetrainTorque = INT_MIN;
    s_car.drive.motionState = CAR_MOTION_AIRBORNE;
    s_car.drive.jumpTimer = 1;
    UpdateCarDrivetrain(&s_car);
    Check(s_car.acceleration == 0,
          "extreme airborne drivetrain leaves acceleration neutral",
          s_car.acceleration, 0);
    Check(s_airborneCalls == 1, "extreme drivetrain reaches airborne motion",
          s_airborneCalls, 1);
}

static void ExtremeShiftArithmeticTests(void) {
    s32 acceleration;

    BuildSpec();
    Check(CalculateAirborneEngineRpm(&s_spec, 1, INT_MAX) == 0,
          "airborne RPM preserves wrapped wheel speed",
          CalculateAirborneEngineRpm(&s_spec, 1, INT_MAX), 0);

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_DRIVING;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    s_car.drive.manual = 1;
    s_car.speed = INT_MAX;
    s_car.acceleration = INT_MAX;
    s_spec.gearRatio[6] = INT_MAX;
    s_car.drive.roadGrade = INT_MIN;
    acceleration = INT_MAX;
    UpdateCarGearShiftState(&s_car, &s_spec, s_car.drive.roadGrade, &acceleration);
    Check(s_car.drive.clutch == 10, "extreme shift engages the clutch",
          s_car.drive.clutch, 10);
    Check(acceleration == 0, "extreme shift clears initial acceleration",
          acceleration, 0);

    PlaceCar();
    s_car.drive.motionState = CAR_MOTION_DRIVING;
    s_car.drive.gearDisp = s_car.drive.gear;
    s_car.drive.clutch = INT16_MAX;
    s_car.drive.shiftSpeedDelta = INT16_MAX;
    s_car.drive.shiftTargetSpeed = INT_MIN;
    acceleration = 0;
    UpdateCarGearShiftState(&s_car, &s_spec, s_car.drive.roadGrade, &acceleration);
    Check(s_car.drive.clutch == INT16_MAX - 1,
          "extreme shift countdown advances", s_car.drive.clutch,
          INT16_MAX - 1);
}

int main(void) {
    Check(CalculateCarRpmDelta(0, INT16_MIN) == INT16_MIN,
          "RPM delta wraps at the negative limit",
          CalculateCarRpmDelta(0, INT16_MIN), INT16_MIN);
    Check(CalculateCarRpmDelta(INT16_MAX, -1) == INT16_MIN,
          "RPM delta wraps at the positive limit",
          CalculateCarRpmDelta(INT16_MAX, -1), INT16_MIN);
    CurveBiasTests();
    ShiftInterpolationTests();
    GradePenaltyTests();
    TorqueBandTests();
    AutomaticScaleRetailValueTests();
    GearBoundsTests();
    MissingTrackTests();
    ExtremeLoadArithmeticTests();
    ExtremeTorqueArithmeticTests();
    ExtremeDrivetrainArithmeticTests();
    ExtremeShiftArithmeticTests();

    if (s_failures != 0) {
        printf("%d drivetrain checks failed\n", s_failures);
        return 1;
    }
    printf("the drivetrain's tall gears and mid-shift behave as they shipped\n");
    return 0;
}

s32 SinAngle(s32 angle) { return rsin(angle); }
s32 CosAngle(s32 angle) { return rcos(angle); }
