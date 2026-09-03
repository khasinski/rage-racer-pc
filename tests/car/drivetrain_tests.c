/*
 * The parts of the drivetrain a race does not reach.
 *
 * UpdateCarDrivetrain is one 800-line function, and the smoke runs only ever
 * get the car into first or second gear on a flat piece of track. That leaves
 * the tall-gear grade penalty and the mid-shift interpolation unexercised by
 * anything, which is exactly where the four gotos this replaced used to jump.
 * Here the function is handed a car spec and a car placed where those branches
 * run.
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
GearCurveRow g_GearTorqueCurve[8];
s16 g_TorqueBandEnd[CAR_TORQUE_BAND_COUNT];
s16 g_TorqueLossBandEnd[CAR_TORQUE_BAND_COUNT];
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
GameTrackArcCenter *g_TrackArcCenters;
s16 g_RacePhase;
s32 g_RoadGrade;
s16 g_DragScale;
s16 g_GripLossTimer;
s32 g_DriveBoostTimer;
s32 g_StandingStartSpin;
s32 g_ShiftTargetRpm;
s32 g_ShiftTargetSpeed;
u8 g_PadType;

/* Where the drivetrain hands off once it has worked out the forces. What
 * those do with the result is their own business, not this test's. */
static int s_drivingCalls;
static int s_launchCalls;
static int s_airborneCalls;
static int s_standingStartCalls;

void UpdateCarDriving(PlayerCarRuntime *car) {
    (void)car;
    s_drivingCalls++;
}
void UpdateCarLaunch(PlayerCarRuntime *car) {
    (void)car;
    s_launchCalls++;
}
void UpdateCarAirborne(PlayerCarRuntime *car) {
    (void)car;
    s_airborneCalls++;
}
void UpdateCarStandingStart(PlayerCarRuntime *car) {
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
    memset(g_GearTorqueCurve, 0, sizeof(g_GearTorqueCurve));

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
        g_TorqueBandEnd[i] = (s16)(i + 2);
        g_TorqueLossBandEnd[i] = (s16)(i + 2);
    }
    for (i = 0; i < 9; i++) {
        s_spec.torqueLossRpm[i] = i * 1000;
    }
    for (i = 0; i < 10; i++) {
        s_spec.torqueLossValue[i] = i * 10;
    }
    for (i = 0; i < 8; i++) {
        int slot;
        for (slot = 0; slot < 16; slot++) {
            /* Each gear pulls differently, so reading the wrong gear's
             * curve is visible. */
            g_GearTorqueCurve[i].values[slot] = slot * 1000 * (i + 1);
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
    g_RoadGrade = 0;
    g_PadType = 0;
    g_ShiftTargetRpm = 0;
    g_ShiftTargetSpeed = 0;
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
    g_ShiftTargetRpm = 5000;
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
    g_ShiftTargetRpm = 5000;
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
    g_ShiftTargetRpm = 5000;
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
    g_ShiftTargetRpm = 0;
    UpdateCarDrivetrain(&s_car);
    Check(g_ShiftTargetRpm == (((5000 * 0xA0) / 1168) * 0x2710) / 1000,
          "the shift target follows road speed", g_ShiftTargetRpm,
          (((5000 * 0xA0) / 1168) * 0x2710) / 1000);

    /* With the box caught up it is left where it was. */
    PlaceCar();
    s_car.drive.motionState = 2;
    s_car.drive.jumpTimer = 10;
    s_car.speed = 5000;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 3;
    g_ShiftTargetRpm = 1234;
    UpdateCarDrivetrain(&s_car);
    Check(g_ShiftTargetRpm == 1234, "a caught-up box keeps its target",
          g_ShiftTargetRpm, 1234);
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
    g_RoadGrade = 0;
    UpdateCarDrivetrain(&s_car);
    level = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 3;
    s_car.drive.gearDisp = 2;
    g_RoadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    third = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 4;
    s_car.drive.gearDisp = 3;
    g_RoadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    fourth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 5;
    s_car.drive.gearDisp = 4;
    g_RoadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    fifth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    g_RoadGrade = -1200;
    UpdateCarDrivetrain(&s_car);
    sixth = s_car.drive.engineLoad;

    PlaceCar();
    s_car.acceleration = 1000;
    s_car.drive.manual = 1;
    s_car.drive.gear = 6;
    s_car.drive.gearDisp = 5;
    g_RoadGrade = 1200;
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
    g_TorqueBandEnd[2] = 4;
    g_TorqueBandEnd[3] = 4;
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
        ReadCarEngineTorque(&s_car.drive, &s_spec,
                            g_GearTorqueCurve[1].values,
                            &netTorque, &bandScale);
        Check(netTorque == 123, "negative RPM uses the first torque band",
              netTorque, 123);
        Check(bandScale == 0, "negative RPM has no engine braking",
              bandScale, 0);
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
    g_DragScale = 0;
    UpdateCarDrivetrain(&s_car);
    Check(g_DragScale == 1000, "zero drag scale is repaired",
          g_DragScale, 1000);

    BuildSpec();
    PlaceCar();
    s_spec.gearRatio[1] = 0;
    s_car.drive.motionState = CAR_MOTION_AIRBORNE;
    s_car.drive.jumpTimer = 10;
    s_car.drive.gearDisp = 2;
    UpdateCarDrivetrain(&s_car);
    Check(g_ShiftTargetRpm == (((2000 * 0xA0) / 1168) * 0x2710),
          "zero shift ratio uses a unit divisor", g_ShiftTargetRpm,
          (((2000 * 0xA0) / 1168) * 0x2710));
}

static void MissingTrackTests(void) {
    CarDrivetrainLoads loads;

    BuildSpec();
    PlaceCar();
    g_TrackPoints = NULL;
    g_TrackPointCount = 0;
    s_car.drive.steeringGrip = 20;
    s_car.drive.steeringGripResponse = 1000;
    UpdateCarSteeringGrip(&s_car, &s_spec, 100);
    Check(s_car.drive.steeringGrip == 60,
          "missing track keeps neutral steering grip",
          s_car.drive.steeringGrip, 60);

    s_car.drive.motionState = CAR_MOTION_TAKEOFF;
    s_car.drive.trackCurveMode = 1;
    s_car.drive.trackCurveBias = 7;
    UpdateCarSteeringGrip(&s_car, &s_spec, 0);
    Check(s_car.drive.trackCurveBias == 7,
          "missing track does not change curve bias",
          s_car.drive.trackCurveBias, 7);

    g_RoadGrade = 123;
    loads = CalculateCarDrivetrainLoads(&s_car, &s_spec, 0, 0, 0);
    (void)loads;
    Check(g_RoadGrade == 0, "missing track clears road grade",
          g_RoadGrade, 0);
}

static void ExtremeLoadArithmeticTests(void) {
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
    g_DriveBoostTimer = INT_MAX;
    g_DragScale = 1;

    loads = CalculateCarDrivetrainLoads(
        &s_car, &s_spec, INT_MAX, INT_MAX, INT_MAX);
    (void)loads;
    Check(g_DragScale == 1000, "extreme loads reset drag scale",
          g_DragScale, 1000);
    Check(g_DriveBoostTimer == INT_MAX - 1,
          "extreme loads advance boost timer", g_DriveBoostTimer,
          INT_MAX - 1);
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
    GearBoundsTests();
    MissingTrackTests();
    ExtremeLoadArithmeticTests();

    if (s_failures != 0) {
        printf("%d drivetrain checks failed\n", s_failures);
        return 1;
    }
    printf("the drivetrain's tall gears and mid-shift behave as they shipped\n");
    return 0;
}
