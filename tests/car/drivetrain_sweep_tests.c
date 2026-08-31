/*
 * The whole drivetrain, swept.
 *
 * drivetrain_tests answers named questions about named branches, and stubs the
 * trigonometry to zero so those answers can be read off by hand. That leaves
 * the slide, the heading error and the pedal latch unreachable by
 * construction: a mutation probe over the file found seven of ten injected
 * mistakes surviving, and a driven race caught only one more.
 *
 * This walks the states instead, with a real sine table and a real angle
 * difference, and folds everything the call could have written into one
 * number.
 */

#include "common.h"
#include "game/car.h"
#include "game/track.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

void UpdateCarDrivetrain(PlayerCarRuntime *carArg);

/* The tables the drivetrain reads. */
GameCarSpec *g_CarSpec;
GearCurveRow g_GearTorqueCurve[8];
s16 g_TorqueBandEnd[8];
s16 g_TorqueLossBandEnd[8];
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
static s32 s_lastDrag;
static int s_drivingCalls;
static int s_launchCalls;
static int s_airborneCalls;
static int s_standingStartCalls;

void UpdateCarDriving(PlayerCarRuntime *car, s32 drag) {
    (void)car;
    s_lastDrag = drag;
    s_drivingCalls++;
}
void UpdateCarLaunch(PlayerCarRuntime *car, s32 drag) {
    (void)car;
    s_lastDrag = drag;
    s_launchCalls++;
}
void UpdateCarAirborne(PlayerCarRuntime *car, s32 drag) {
    (void)car;
    s_lastDrag = drag;
    s_airborneCalls++;
}
void UpdateCarStandingStart(PlayerCarRuntime *car, s32 drag) {
    (void)car;
    s_lastDrag = drag;
    s_standingStartCalls++;
}

/* The console's quarter-turn sine table, and the two angle helpers written
 * out, because a drivetrain that is told every angle is zero never slides. */
s32 rsin(s32 angle) {
    static const s32 quarter[17] = {0,     0x18F,  0x31F,  0x4AD, 0x63A, 0x7C4,
                                    0x94C, 0xACF,  0xC4E,  0xDC7, 0xF3A, 0x10A6,
                                    0x120A, 0x1365, 0x14B7, 0x15FF, 0x173C};
    s32 index = ((angle & 0xFFF) * 16) / 0x400;
    s32 sign = 1;

    if (index >= 32) {
        index -= 32;
        sign = -1;
    }
    if (index >= 16) {
        index = 32 - index;
    }
    return sign * quarter[index];
}
s32 rcos(s32 angle) { return rsin(angle + 0x400); }

s32 Atan2(s32 x, s32 y) {
    /* Enough of an arctangent to put the answer in the right octant. */
    s32 ax = x < 0 ? -x : x;
    s32 az = y < 0 ? -y : y;
    s32 base = (ax > az) ? (az * 0x200) / (ax + 1) : 0x400 - (ax * 0x200) / (az + 1);

    if (x < 0) base = 0x800 - base;
    if (y < 0) base = -base;
    return base & 0xFFF;
}

s32 GetAngleDistance(s32 a, s32 b) {
    s32 delta = (a - b) & 0xFFF;

    return delta >= 0x800 ? delta - 0x1000 : delta;
}

static GameCarSpec s_spec;
static GameTrackPoint s_points[4];
static GameTrackArcCenter s_arcs[4];
static PlayerCarRuntime s_car;


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


static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_records;

static void Fold(unsigned char byte) {
    s_digest = ((s_digest ^ byte) * 16777619UL) & 0xFFFFFFFFUL;
}

static void Record(const char *name, const s32 *values, int count) {
    const char *p;
    int i;

    for (p = name; *p != '\0'; p++) {
        Fold((unsigned char)*p);
    }
    for (i = 0; i < count; i++) {
        u32 value = (u32)values[i];

        Fold((unsigned char)value);
        Fold((unsigned char)(value >> 8));
        Fold((unsigned char)(value >> 16));
        Fold((unsigned char)(value >> 24));
    }
    if (s_out != NULL) {
        fputs(name, s_out);
        for (i = 0; i < count; i++) {
            fprintf(s_out, " %d", values[i]);
        }
        fputc('\n', s_out);
    }
    s_records++;
}

/* Everything the drivetrain can move, plus which of the four motion handlers
 * it decided to hand the car to and with what drag. */
static void RecordDrive(const char *label) {
    GameCarDrive *p = &s_car.drive;
    s32 after[26];

    after[0] = p->gear;
    after[1] = p->gearDisp;
    after[2] = p->clutch;
    after[3] = p->engineRpm;
    after[4] = p->engineLoad;
    after[5] = p->drivetrainTorque;
    after[6] = p->drivetrainCoupled;
    after[7] = p->acceleratorLatch;
    after[8] = p->brakeLatch;
    after[9] = p->steerPos;
    after[10] = p->steeringGrip;
    after[11] = p->steeringGripResponse;
    after[12] = p->steeringLoadAngle;
    after[13] = p->motionState;
    after[14] = p->jumpTimer;
    after[15] = p->shiftRpmDelta;
    after[16] = p->shiftSpeedDelta;
    after[17] = s_car.speed;
    after[18] = s_car.acceleration;
    after[19] = s_car.headingAngle;
    after[20] = p->trackCurveBias;
    after[21] = p->trackCurveMode;
    after[22] = s_lastDrag;
    after[23] = s_drivingCalls;
    after[24] = s_launchCalls;
    after[25] = s_airborneCalls + s_standingStartCalls;
    Record(label, after, 26);
}

/* Four track points in a line with a corner in the middle, so the curve bias
 * and the heading error have something to be measured against. */
static void BuildTrack(void) {
    int i;

    memset(s_points, 0, sizeof(s_points));
    memset(s_arcs, 0, sizeof(s_arcs));
    for (i = 0; i < 4; i++) {
        s_points[i].x = i * 0x1000;
        s_points[i].z = 0x800;
        s_points[i].angle = (s16)(i * 0x180);
        s_points[i].segmentLength = 0x1000;
        s_points[i].leftHalfWidth = 0x400;
        s_points[i].rightHalfWidth = 0x400;
    }
    s_arcs[1].x = 0x2000;
    s_arcs[1].z = 0x4000;
    s_points[1].arcRef = (u16)((1 << 4) | 1);
    g_TrackPoints = s_points;
    g_TrackPointCount = 4;
    g_TrackArcCenters = s_arcs;
}

int main(int argc, char **argv) {
    /*
     * What the drivetrain did before it was taken apart. Run the test with a
     * file name to write the sweep out and diff two runs.
     */
    static const unsigned long expected = 2620216087UL;
    static const s32 speeds[] = {0, 0x100, 0x800, 0x4000, 0x20000};
    static const s32 gears[] = {1, 2, 5, 6};
    static const s32 pedals[] = {0, 0x7B, 0x85, 0x100};
    static const s32 headings[] = {0, 0x200, 0x401, 0x800, -0x401};
    static const s32 motions[] = {CAR_MOTION_DRIVING, CAR_MOTION_TAKEOFF,
                                  CAR_MOTION_AIRBORNE, CAR_MOTION_STANDING_START};
    static const s32 grades[] = {0, 0x400, -0x400};
    int si, gi, ai, bi, hi, mi, gr, manual, pad;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }
    BuildSpec();
    BuildTrack();

    for (si = 0; si < 5; si++)
    for (gi = 0; gi < 4; gi++)
    for (ai = 0; ai < 4; ai++)
    for (bi = 0; bi < 4; bi++)
    for (hi = 0; hi < 5; hi++)
    for (mi = 0; mi < 4; mi++)
    for (gr = 0; gr < 3; gr++)
    for (manual = 0; manual < 2; manual++)
    for (pad = 0; pad < 2; pad++) {
        char label[176];
        GameCarDrive *p = &s_car.drive;

        memset(&s_car, 0, sizeof(s_car));
        s_car.speed = speeds[si];
        s_car.headingAngle = headings[hi];
        s_car.bodyYaw = 0x100;
        s_car.trackPointIndex = 1;
        s_car.segmentFraction = 0x800;
        s_car.x = 0x1400;
        s_car.z = 0x900;
        p->gear = (s16)gears[gi];
        p->gearDisp = (s16)gears[gi];
        p->acceleratorInput.sampled = (s16)pedals[ai];
        p->brakeInput = (s16)pedals[bi];
        p->acceleratorLatch = (s16)(ai & 1);
        p->brakeLatch = (s16)(bi & 1);
        p->motionState = (s16)motions[mi];
        p->manual = (s16)manual;
        p->engineRpm = 4000;
        p->engineLoad = 500;
        p->drivetrainTorque = 100000;
        p->steeringGrip = 200;
        p->steerPos = (s16)(headings[hi] / 2);
        p->clutch = (s16)(gi & 1);
        g_RoadGrade = grades[gr];
        g_RacePhase = 2;
        g_DragScale = 0x2BC;
        g_GripLossTimer = 0;
        g_DriveBoostTimer = 0;
        g_StandingStartSpin = 0;
        g_ShiftTargetRpm = 5000;
        g_ShiftTargetSpeed = 3000;
        g_PadType = (u8)(pad ? 0x23 : 0x41);
        g_CarSpec = &s_spec;
        s_lastDrag = 0;
        s_drivingCalls = 0;
        s_launchCalls = 0;
        s_airborneCalls = 0;
        s_standingStartCalls = 0;

        sprintf(label, "speed%d gear%d accel%d brake%d heading%d motion%d "
                "grade%d manual%d pad%d", speeds[si], gears[gi], pedals[ai],
                pedals[bi], headings[hi], motions[mi], grades[gr], manual, pad);
        Record(label, NULL, 0);
        UpdateCarDrivetrain(&s_car);
        RecordDrive("drove");
        steps++;
    }

    /*
     * Five conditions the sweep above cannot arrive at by driving: engine
     * braking, a shift in progress, a heading error sitting exactly on its
     * threshold, a slide hard enough to hit its clamp, and a car whose
     * reference radius leaves no downforce. Each is set up rather than driven
     * to.
     */
    {
        static const s32 torques[] = {100000, -400000};
        static const s32 loads[] = {500, 60000};
        static const s32 radii[] = {100, 1, 0};
        static const s32 exactHeadings[] = {0x100 - 0x401, 0x100 - 0x400,
                                            0x100 - 0x402};
        int ti, li, ri, ei, shifting;

        for (ti = 0; ti < 2; ti++)
        for (li = 0; li < 2; li++)
        for (ri = 0; ri < 3; ri++)
        for (ei = 0; ei < 3; ei++)
        for (shifting = 0; shifting < 2; shifting++) {
            char label[176];
            GameCarDrive *p = &s_car.drive;

            memset(&s_car, 0, sizeof(s_car));
            s_car.speed = 0x4000;
            s_car.headingAngle = exactHeadings[ei];
            s_car.bodyYaw = 0x100;
            s_car.trackPointIndex = 1;
            s_car.segmentFraction = 0x800;
            s_car.x = 0x1400;
            s_car.z = 0x900;
            p->gear = 3;
            p->gearDisp = (s16)(shifting ? 4 : 3);
            p->acceleratorInput.sampled = 0x100;
            p->brakeInput = 0x40;
            p->motionState = (s16)(shifting ? CAR_MOTION_TAKEOFF
                                            : CAR_MOTION_DRIVING);
            p->engineRpm = 6000;
            p->engineLoad = (s16)loads[li];
            p->drivetrainTorque = torques[ti];
            p->steeringGrip = 200;
            /* Full lock, so the slide has something to clamp. */
            p->steerPos = -0x1000;
            p->clutch = 0;
            s_spec.referenceTurnRadius = (s16)radii[ri];
            g_RoadGrade = 0x400;
            g_RacePhase = 2;
            g_DragScale = 0x2BC;
            g_ShiftTargetRpm = 5000;
            g_ShiftTargetSpeed = 3000;
            g_PadType = 0x41;
            g_CarSpec = &s_spec;
            s_lastDrag = 0;
            s_drivingCalls = 0;
            s_launchCalls = 0;
            s_airborneCalls = 0;
            s_standingStartCalls = 0;

            sprintf(label, "edge torque%d load%d radius%d heading%d shifting%d",
                    torques[ti], loads[li], radii[ri], exactHeadings[ei],
                    shifting);
            Record(label, NULL, 0);
            UpdateCarDrivetrain(&s_car);
            RecordDrive("drove");
            steps++;
        }
        s_spec.referenceTurnRadius = 100;
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL the drivetrain behaves differently: %d states making %d "
               "records digest to %lu, expected %lu\n", steps, s_records,
               s_digest, expected);
        return 1;
    }
    printf("the drivetrain takes the same %d states it always did\n", steps);
    return 0;
}
