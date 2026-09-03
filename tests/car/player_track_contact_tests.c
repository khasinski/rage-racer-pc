#include "game/car.h"
#include "game/car_internal.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;

static GameTrackPoint s_points[2];
static s32 s_trackResult;
static s32 s_rotation;
static int s_measureCalls;
static int s_knockbackCalls;
static int s_trackCalls;
static int s_traceCalls;
static int s_failures;

/* The track frame comes from the GTE rotation, whose Y convention is the
 * transpose of BuildRotMatrixY. The helper must keep calling this one. */
#undef RotMatrix
MATRIX *RotMatrix(SVECTOR *rotation, MATRIX *matrix) {
    memset(matrix, 0, sizeof(*matrix));
    s_rotation = rotation->vy;
    return matrix;
}

void MeasurePlayerTrackLimits(const Matrix *toTrack,
                              CarTrackLimits *limits) {
    (void)toTrack;
    memset(limits, 0, sizeof(*limits));
    s_measureCalls++;
}

void ApplyCarKnockback(GameCarRuntime *car) {
    (void)car;
    s_knockbackCalls++;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 pointIndex,
                        const CarTrackLimits *limits) {
    (void)car;
    (void)pointIndex;
    (void)limits;
    s_trackCalls++;
    return s_trackResult;
}

void TraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    (void)phase;
    (void)car;
    s_traceCalls++;
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(s_points, 0, sizeof(s_points));
    g_TrackPoints = s_points;
    g_TrackPointCount = 2;
    s_points[0].angle = 0x100;
    car->bodyYaw = 0xC80;
    car->speed = 100;
    s_trackResult = 0;
    s_rotation = 0;
    s_measureCalls = 0;
    s_knockbackCalls = 0;
    s_trackCalls = 0;
    s_traceCalls = 0;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    s_trackResult = 4;
    CHECK(ResolvePlayerTrackContact(&car) == 4);
    CHECK(s_rotation == 0x180);
    CHECK(s_measureCalls == 1 && s_trackCalls == 1 && s_traceCalls == 2);
    CHECK(s_knockbackCalls == 0);

    Reset(&car);
    car.motionActive = 1;
    car.motionTimer = 1;
    s_trackResult = 1;
    CHECK(ResolvePlayerTrackContact(&car) == 1);
    CHECK(s_knockbackCalls == 1);

    Reset(&car);
    car.motionActive = 1;
    car.motionTimer = 0x8000;
    CHECK(ResolvePlayerTrackContact(&car) == 0);
    CHECK(s_knockbackCalls == 1);

    Reset(&car);
    car.motionTimer = 1;
    CHECK(ResolvePlayerTrackContact(&car) == 0);
    CHECK(s_knockbackCalls == 0);

    Reset(&car);
    car.speed = 63;
    s_trackResult = 2;
    CHECK(ResolvePlayerTrackContact(&car) == 0);
    s_trackResult = 3;
    CHECK(ResolvePlayerTrackContact(&car) == 0);

    Reset(&car);
    car.speed = 63;
    s_trackResult = 1;
    CHECK(ResolvePlayerTrackContact(&car) == 1);
    s_trackResult = 4;
    CHECK(ResolvePlayerTrackContact(&car) == 4);

    Reset(&car);
    car.speed = 64;
    s_trackResult = 2;
    CHECK(ResolvePlayerTrackContact(&car) == 2);

    Reset(&car);
    g_TrackPoints = NULL;
    CHECK(ResolvePlayerTrackContact(&car) == 0);
    CHECK(s_measureCalls == 0 && s_trackCalls == 0 && s_traceCalls == 0);

    Reset(&car);
    g_TrackPointCount = 0;
    CHECK(ResolvePlayerTrackContact(&car) == 0);
    CHECK(s_measureCalls == 0 && s_trackCalls == 0 && s_traceCalls == 0);

    Reset(&car);
    car.bodyYaw = INT_MAX;
    s_points[0].angle = INT16_MAX;
    ResolvePlayerTrackContact(&car);
    CHECK(s_rotation == 0x3FE);

    if (s_failures != 0) {
        printf("%d player track contact checks failed\n", s_failures);
        return 1;
    }
    puts("player track contact preserves skid filtering and knockback");
    return 0;
}
