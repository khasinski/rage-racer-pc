/*
 * Where a car is on the track, swept.
 *
 * UpdateCarTrackState is the function that turns a car's world position into
 * its place on the course: how far round it is, how far off the centre line,
 * which way the track runs under it, and whether it has hit a boundary. Lap
 * counting, the AI's racing line, the camera and collision all read what it
 * writes, and nothing tested it.
 *
 * It is pure given a track and a car, so this walks a grid of cars over a
 * course built here, both a straight stretch and a corner in each direction,
 * and folds everything the call could have written into one number. Comparing
 * that number across a change says whether the change moved a car.
 */

#include "common.h"
#include "game/car.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 UpdateCarTrackState(GameCarRuntime *obj, s32 trackPointIndex,
                        const CarTrackLimits *limits);

/* These six come from the port's own state, which this test links; declaring
 * them here as well left two definitions of each, which only a linker that
 * merges tentative definitions would accept. */
PlayerCarRuntime g_PlayerCar;
/* The working set the track code hands its intermediate values through. */
ObjectMatrixWork g_ObjectMatrixWork;
CarTrackWork g_CarTrackWork;
GameRenderState g_RenderState;

/* The boundary response calls out to the knockback code; what that does with
 * the hit is its own business, but that it was called is part of this one's
 * behaviour. */
static int s_knockbacks;
static s32 s_lastKnockX;
static s32 s_lastKnockZ;
static s32 s_lastKnockMode;

void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    (void)car;
    s_knockbacks++;
    s_lastKnockX = x;
    s_lastKnockZ = z;
    s_lastKnockMode = mode;
}

/* The diagnostics hooks are off in a test run. */
int DiagnosticsEnabled(const char *channel) { (void)channel; return 0; }
const char *DiagnosticsValue(const char *key) { (void)key; return NULL; }
void Trace(const char *channel, const char *format, ...) {
    (void)channel;
    (void)format;
}

/* Reached only from SetCameraRotMatrix in the matrix module, which builds the
 * mirror view rather than anything this test looks at. */
MATRIX *MulMatrix0(MATRIX *a, MATRIX *b, MATRIX *out) {
    (void)a;
    (void)b;
    return out;
}
void GameRenderWorldSetCamera(s32 x, s32 y, s32 z, s32 pitch, s32 yaw,
                              s32 roll) {
    (void)x; (void)y; (void)z; (void)pitch; (void)yaw; (void)roll;
}

/*
 * A ring of eight points: four straight, then a right-hand corner, then a
 * left-hand one, so both signs of the arc branch are walked.
 */
static GameTrackPoint s_points[8];
static GameTrackArcCenter s_arcs[2];

static void BuildTrack(void) {
    int i;

    memset(s_points, 0, sizeof(s_points));
    memset(s_arcs, 0, sizeof(s_arcs));

    for (i = 0; i < 8; i++) {
        s_points[i].x = i * 0x1000;
        s_points[i].z = 0x800;
        s_points[i].y = (s16)(i * 8);
        s_points[i].angle = (s16)(i * 0x40);
        s_points[i].surfacePitch = (s16)(i * 4);
        s_points[i].crossSlope = (s16)(i - 4);
        s_points[i].leftHalfWidth = 0x400;
        s_points[i].rightHalfWidth = 0x400;
        s_points[i].segmentLength = 0x1000;
        s_points[i].arcRef = 0;
    }

    /* Point 4 corners one way and point 5 the other; bits 0..1 of arcRef pick
     * the model and bits 4..15 index the centre table. */
    s_arcs[0].x = 0x4000;
    s_arcs[0].z = 0x4800;
    s_arcs[1].x = 0x5000;
    s_arcs[1].z = -0x3800;
    s_points[4].arcRef = TRACK_CURVE_PRIMARY;
    s_points[5].arcRef = (u16)((1 << 4) | TRACK_CURVE_MIRRORED);

    g_TrackPoints = s_points;
    g_TrackPointCount = 8;
    g_TrackArcCenters = s_arcs;
    g_TrackLength = 8 * 0x1000;
}

static unsigned long s_digest = 2166136261UL;

static void Fold(FILE *out, const char *label, s32 result,
                 const GameCarRuntime *car) {
    char line[512];
    const char *p;

    snprintf(line, sizeof(line),
             "%s -> %d pos=(%d,%d,%d) yaw=%d pitch=%d roll=%d heading=%d "
             "lateral=%d norm=%d progress=%d/%d prev=%d a=%d b=%d frac=%d "
             "section=%d speed=%d vel=(%d,%d) motion=%d/%d "
             "knock=%d/%d/%d/%d\n",
             label, result, car->x, car->y, car->z, car->bodyYaw,
             car->bodyPitch, car->bodyRoll, car->trackHeading.value,
             car->trackLateralOffset, car->normalizedLateralOffset,
             car->trackProgress, g_TrackLength, car->previousTrackProgress,
             car->progressA, car->progressB, car->segmentFraction,
             car->trackSection, car->speed, car->velocityX, car->velocityZ,
             car->motionActive, car->motionTimer, s_knockbacks, s_lastKnockX,
             s_lastKnockZ, s_lastKnockMode);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) {
        fputs(line, out);
    }
}

static int CheckPlayerBoundaryKnockback(const CarTrackLimits *limits,
                                        s32 lateralOffset,
                                        s32 expectedMode,
                                        const char *edgeName) {
    GameCarRuntime *car = AsRivalCar(&g_PlayerCar);
    s32 startZ;
    s32 result;

    BuildTrack();
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    car->x = s_points[0].x;
    car->z = s_points[0].z + lateralOffset;
    startZ = car->z;
    s_knockbacks = 0;
    s_lastKnockMode = 0;

    result = UpdateCarTrackState(car, 0, limits);
    if (result != expectedMode || s_knockbacks != 1 ||
        s_lastKnockMode != expectedMode || car->z == startZ) {
        printf("FAIL player %s boundary: result=%d calls=%d "
               "mode=%d z=%d/%d\n", edgeName, result, s_knockbacks,
               s_lastKnockMode, startZ, car->z);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    /*
     * What the function did before it was touched. A change anywhere in it
     * moves this; run the test with a file name to write the sweep out and
     * diff two runs to see which cars moved.
     */
    static const unsigned long expected = 1686797923UL;
    static const s32 lateralOffsets[] = {0, 0x200, 0x400, 0x600, -0x200,
                                         -0x400, -0x600};
    static const s32 headings[] = {0, 0x200, 0x400, 0x800, 0xC00};
    static const s32 speeds[] = {0, 0x100, 0x800};
    FILE *out = NULL;
    CarTrackLimits limits;
    GameCarRuntime car;
    s32 steps = 0;
    int point, li, hi, si, series, alongIndex;
    char label[96];

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    BuildTrack();

    for (series = 0; series < 2; series++) {
        for (point = 0; point < 8; point++) {
            for (alongIndex = 0; alongIndex < 3; alongIndex++) {
                for (li = 0;
                     li < (int)(sizeof(lateralOffsets) / sizeof(s32)); li++) {
                    for (hi = 0; hi < (int)(sizeof(headings) / sizeof(s32));
                         hi++) {
                        for (si = 0; si < (int)(sizeof(speeds) / sizeof(s32));
                             si++) {
                            s32 along = alongIndex * 0x600;
                            s32 result;

                            memset(&car, 0, sizeof(car));
                            /* Put the car `along` down the segment and
                             * `lateral` to one side of it. */
                            car.x = s_points[point].x + along;
                            car.z = s_points[point].z + lateralOffsets[li];
                            car.y = s_points[point].y;
                            car.bodyYaw = headings[hi];
                            car.speed = speeds[si];
                            car.trackProgress = point * 0x1000;
                            car.previousTrackProgress = car.trackProgress;
                            car.trackSection = (s16)point;

                            limits.leftInset = 0x40;
                            limits.rightInset = 0x40;
                            limits.leftKnockbackMode = 2;
                            limits.rightKnockbackMode = 3;

                            g_RaceSeries = series;
                            g_SceneTimer = 100;
                            s_knockbacks = 0;
                            s_lastKnockX = 0;
                            s_lastKnockZ = 0;
                            s_lastKnockMode = 0;
                            memset(&g_CarTrackWork, 0,
                                   sizeof(g_CarTrackWork));

                            result = UpdateCarTrackState(&car, point, &limits);

                            sprintf(label, "r%d/p%d/a%d/l%d/h%d/s%d", series,
                                    point, along, lateralOffsets[li],
                                    headings[hi], speeds[si]);
                            Fold(out, label, result, &car);
                            steps++;
                        }
                    }
                }
            }
        }
    }

    if (out != NULL) {
        fclose(out);
    }

    if (s_digest != expected) {
        printf("FAIL cars land somewhere else on the track: "
               "%d placements digest to %lu, expected %lu\n",
               steps, s_digest, expected);
        return 1;
    }

    if (CheckPlayerBoundaryKnockback(&limits, -0x600,
                                     limits.leftKnockbackMode,
                                     "left") != 0 ||
        CheckPlayerBoundaryKnockback(&limits, 0x600,
                                     limits.rightKnockbackMode,
                                     "right") != 0) {
        return 1;
    }

    memset(&car, 0, sizeof(car));
    car.x = 123;
    g_TrackPointCount = 0;
    if (UpdateCarTrackState(&car, 0, &limits) != 0 || car.x != 123) {
        puts("FAIL empty track placement changed the car");
        return 1;
    }

    BuildTrack();
    s_points[0].leftHalfWidth = 0;
    s_points[0].rightHalfWidth = 0;
    s_points[1].leftHalfWidth = 0;
    s_points[1].rightHalfWidth = 0;
    memset(&car, 0, sizeof(car));
    car.x = s_points[0].x;
    car.z = s_points[0].z;
    UpdateCarTrackState(&car, 0, &limits);
    if (car.normalizedLateralOffset != 0) {
        puts("FAIL zero-width track produced a normalized offset");
        return 1;
    }
    printf("all %d placements put the car where they always did\n", steps);
    return 0;
}
