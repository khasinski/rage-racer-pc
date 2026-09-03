/*
 * What the three jump handlers leave behind.
 *
 * A car in the air runs one of three handlers: the takeoff frame that turns a
 * launch spin into yaw, the airborne frames that carry it, and the standing
 * start. They read and write a lot of state and call out to steering and sound
 * effects, so this is a characterisation test: it does
 * not say the behaviour is right, only that it is what it was. Everything the
 * handlers reach for is supplied here so a sweep is repeatable.
 */

#include "common.h"
#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_state.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

void UpdateCarLaunch(PlayerCarRuntime *car);
void UpdateCarAirborne(PlayerCarRuntime *car);
void UpdateCarStandingStart(PlayerCarRuntime *car);


/* A ring of points, so an index maps to a position without a course. */
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    s32 angle = (pointIndex * 4096) / (g_TrackPointCount > 0 ? g_TrackPointCount : 1);
    angle = (angle + weight / 16) & 0xFFF;
    out[0] = (rsin(angle) * 1000) >> 12;
    out[1] = 0;
    out[2] = (rcos(angle) * 1000) >> 12;
}

s32 SmoothTrackAngle(s32 pointIndex, s32 weight) {
    s32 angle = (pointIndex * 4096) / (g_TrackPointCount > 0 ? g_TrackPointCount : 1);
    return (angle + weight / 16) & 0xFFF;
}

/*
 * The effect voice is recorded rather than ignored, because which sound a
 * spinning car asks for is part of what these handlers do. The camera and
 * matrix hooks below belong to the Atan2 implementation linked by steering.
 */
GameRenderState g_RenderState;
static s32 s_voiceIndex, s_voicePhase, s_voiceVolume;

void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) {
    s_voiceIndex = index;
    s_voicePhase = phase;
    s_voiceVolume = volume;
}

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z, int32_t pitch,
                              int32_t yaw, int32_t roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2) {
    (void)m0;
    (void)m1;
    return m2;
}

s32 Random15(void) {
    static s32 state;
    state = (state * 1103515245 + 12345) & 0x7FFF;
    return state;
}

static unsigned long s_digest = 2166136261UL;

static void FoldText(FILE *out, const char *line) {
    const char *p;
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) fputs(line, out);
}

static void Fold(FILE *out, const char *label, const PlayerCarRuntime *car) {
    const GameCarDrive *drive = &car->drive;
    char line[512];

    snprintf(line, sizeof(line),
             "%s -> speed=%d yaw=%d heading=%d accel=%d spin=%d energy=%d "
             "lift=%d state=%d jump=%d accelPos=%d brakePos=%d load=%d "
             "shift=%d launchSpeed=%d yawOffset=%d torque=%d voice=%d/%d/%d "
             "shiftSound=%d bounce=%d/%d standingSpin=%d\n",
             label, car->speed, car->bodyYaw, car->headingAngle,
             car->acceleration, drive->spinRate, drive->launchEnergy,
             drive->bodyLiftOffset, (int)drive->motionState, drive->jumpTimer,
             drive->accelPos, drive->brakePos, drive->engineLoad,
             drive->shiftRpmDelta, drive->launchSpeed, drive->yawOffset,
             drive->drivetrainTorque, s_voiceIndex, s_voicePhase,
             s_voiceVolume, g_ShiftSoundLevel,
             drive->standingStartBounceX, drive->standingStartBounceY,
             g_StandingStartSpin);
    FoldText(out, line);
}

/* Everything a handler reads that the sweep does not vary, set to values that
 * keep the divisions honest: no gear ratio may be zero. */
static void PrepareCar(PlayerCarRuntime *car, GameCarSpec *spec) {
    memset(car, 0, sizeof(*car));
    memset(spec, 0x11, sizeof(*spec));
    spec->steerResponse = 20;
    car->x = 100;
    car->z = 900;
    car->drive.speedScale = 0x400;
    car->drive.targetHeading = 0x200;
    car->drive.steeringLoadAngle = 0x100;
    car->drive.acceleratorInput.value = 0x80;
    car->drive.engineRpm = 3000;
    car->acceleration = 500;
}

static int CheckAirborneYawSymmetry(GameCarSpec *spec) {
    PlayerCarRuntime left;
    PlayerCarRuntime right;
    s32 leftPhase;
    s32 rightPhase;

    PrepareCar(&left, spec);
    PrepareCar(&right, spec);
    left.speed = right.speed = 0x800;
    left.drive.jumpTimer = right.drive.jumpTimer = 10;
    left.drive.yawOffset = -1600;
    right.drive.yawOffset = 1600;

    UpdateCarAirborne(&left);
    leftPhase = s_voicePhase;
    UpdateCarAirborne(&right);
    rightPhase = s_voicePhase;

    if (left.speed != right.speed || leftPhase != rightPhase) {
        printf("airborne yaw is asymmetric: left %d/%d, right %d/%d\n",
               left.speed, leftPhase, right.speed, rightPhase);
        return 1;
    }
    return 0;
}

static int CheckLaunchShiftSoundRange(GameCarSpec *spec) {
    static const s16 rpmDelta[] = {-100, -99, 99, 100};
    static const s16 expectedSound[] = {0, 1, 1, 0};
    size_t test;

    for (test = 0; test < sizeof(rpmDelta) / sizeof(rpmDelta[0]); test++) {
        PlayerCarRuntime car;
        s32 landingRpm;

        PrepareCar(&car, spec);
        car.speed = 1168;
        car.drive.gear = 1;
        car.drive.launchEnergy = 0;
        spec->gearRatio[1] = 10000;
        landingRpm = car.speed * 0xA0 / 1168;
        car.drive.engineRpm = landingRpm - rpmDelta[test];
        g_ShiftSoundLevel = -1;

        UpdateCarLaunch(&car);
        if (g_ShiftSoundLevel != expectedSound[test]) {
            printf("launch RPM delta %d produced shift sound %d, expected %d\n",
                   rpmDelta[test], g_ShiftSoundLevel, expectedSound[test]);
            return 1;
        }
    }
    return 0;
}

static int CheckSixthGearLaunchAssets(GameCarSpec *spec) {
    PlayerCarRuntime car;
    s32 expectedRpm;
    s32 expectedLoad;

    PrepareCar(&car, spec);
    car.speed = 1168;
    car.drive.gear = CAR_FORWARD_GEAR_COUNT;
    car.drive.launchEnergy = 0;
    spec->gearRatio[CAR_FORWARD_GEAR_COUNT] = 0;
    spec->gearRatio[0] = 123;

    expectedRpm = car.speed * 0xA0 / 1168 * 10000;
    expectedLoad = expectedRpm * spec->gearRatio[0] / 0x20000;
    expectedLoad = expectedLoad * 985 / 1000;

    UpdateCarLaunch(&car);

    if (g_ShiftTargetRpm != expectedRpm ||
        car.drive.engineLoad != expectedLoad) {
        printf("sixth-gear launch produced RPM/load %d/%d, expected %d/%d\n",
               g_ShiftTargetRpm, car.drive.engineLoad,
               expectedRpm, expectedLoad);
        return 1;
    }
    return 0;
}

static int CheckExtremeLaunchSpin(GameCarSpec *spec) {
    PlayerCarRuntime car;

    PrepareCar(&car, spec);
    car.drive.gear = 1;
    car.drive.spinRate = INT_MIN;
    car.drive.launchEnergy = INT_MIN;
    UpdateCarLaunch(&car);
    if (car.drive.spinRate < -0x3600 || car.drive.spinRate > 0x3600) {
        puts("extreme launch spin escaped its clamp");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    /*
     * What the handlers did before they were touched. Run with a file name to
     * write the sweep out and diff two runs to see which cases moved.
     */
    static const unsigned long expected = 966554366UL;
    static const s32 spins[] = {0, 0x400, -0x400, 0x2000, -0x2000, 0x3700};
    static const s32 energies[] = {-1, 0, 1, 5000, 200000};
    static const s32 speeds[] = {0, 0x100, 0x190, 0x800};
    static const s32 yaws[] = {0, 0x200, 0x800};
    /* 0x580 puts the skid between the thresholds the launch reads. */
    static const s32 headings[] = {0, 0x100, 0x580, 0x900};
    static const s16 verticals[] = {0, 1};
    static const s16 gears[] = {1, 3, 5};
    /* Zero never reaches the wheelspin the standing start counts down. */
    static const s32 standingSpins[] = {0, 20, 200};
    static GameTrackPoint points[16];
    GameCarSpec spec;
    FILE *out = NULL;
    size_t s, e, v, y, h, g, vert, ss;
    int cases = 0;

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            fprintf(stderr, "cannot write %s\n", argv[1]);
            return 2;
        }
    }
    g_TrackPointCount = 16;
    g_TrackPoints = points;
    g_CarSpec = &spec;

    if (CheckAirborneYawSymmetry(&spec) != 0 ||
        CheckLaunchShiftSoundRange(&spec) != 0 ||
        CheckSixthGearLaunchAssets(&spec) != 0 ||
        CheckExtremeLaunchSpin(&spec) != 0)
        return 1;

    for (s = 0; s < sizeof(spins) / sizeof(spins[0]); s++)
    for (e = 0; e < sizeof(energies) / sizeof(energies[0]); e++)
    for (v = 0; v < sizeof(speeds) / sizeof(speeds[0]); v++)
    for (y = 0; y < sizeof(yaws) / sizeof(yaws[0]); y++)
    for (h = 0; h < sizeof(headings) / sizeof(headings[0]); h++)
    for (g = 0; g < sizeof(gears) / sizeof(gears[0]); g++)
    for (vert = 0; vert < sizeof(verticals) / sizeof(verticals[0]); vert++)
    for (ss = 0; ss < sizeof(standingSpins) / sizeof(standingSpins[0]); ss++) {
        static const char *const names[] = {"launch", "airborne", "standing"};
        int which;

        for (which = 0; which < 3; which++) {
            PlayerCarRuntime car;
            char label[192];

            PrepareCar(&car, &spec);
            car.drive.spinRate = spins[s];
            car.drive.launchEnergy = energies[e];
            car.speed = speeds[v];
            car.bodyYaw = (s16)yaws[y];
            car.headingAngle = (s16)headings[h];
            car.drive.gear = gears[g];
            car.verticalMotionState = verticals[vert];
            g_StandingStartSpin = standingSpins[ss];

            snprintf(label, sizeof(label),
                     "%s spin=%d energy=%d speed=%d yaw=%d heading=%d gear=%d "
                     "vert=%d standing=%d",
                     names[which], spins[s], energies[e], speeds[v], yaws[y],
                     headings[h], gears[g], verticals[vert],
                     standingSpins[ss]);
            if (which == 0) UpdateCarLaunch(&car);
            else if (which == 1) UpdateCarAirborne(&car);
            else UpdateCarStandingStart(&car);
            Fold(out, label, &car);
            cases++;
        }
    }

    if (out != NULL) fclose(out);
    if (s_digest != expected) {
        printf("car_motion_handlers: %d cases folded to %lu, expected %lu\n",
               cases, s_digest, expected);
        return 1;
    }

    {
        PlayerCarRuntime car;

        PrepareCar(&car, &spec);
        car.drive.jumpTimer = 10;
        car.drive.yawOffset = INT_MIN;
        UpdateCarAirborne(&car);
        if (car.drive.yawOffset != -67108864) {
            printf("extreme airborne yaw decayed to %d\n",
                   car.drive.yawOffset);
            return 1;
        }
    }
    printf("car_motion_handlers: %d cases unchanged\n", cases);
    return 0;
}
