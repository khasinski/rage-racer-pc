/*
 * The three pieces of AI the track's own tables drive.
 *
 * GetCarCrestTrigger, UpdateCarAiTargetSpeed and ApplyCarRacingLineHint each
 * read a list the course carries and step a car through it. All three used to
 * be written as jumps into the middle of one another's bodies, which is the
 * kind of shape that a rewrite gets subtly wrong at the ends: the first entry,
 * the last one, the sentinel, the exact boundary. This drives each of them
 * over a table it can see in full, so those ends are checked rather than
 * assumed.
 */

#include "common.h"
#include "game/car.h"
#include "game/track.h"
#include "game/race.h"

#include <stdio.h>
#include <string.h>

/* The state the three of them read. g_Cars is only here because the file
 * defines other functions that touch it. */
GameCarRuntime g_Cars[11];
TrackEventData *g_TrackEventData;
s32 g_RaceSeries;
s32 g_TrackLength;
CarCollisionPoint g_CarCollisionCorners[4];

/* Reached only by the collision code in the same file, which this does not
 * drive. */
s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 pt) {
    (void)p0; (void)p1; (void)p2; (void)p3; (void)pt;
    return 0;
}
void SetCarKnockback(GameCarRuntime *car, s32 x, s32 z, s32 mode) {
    (void)car; (void)x; (void)z; (void)mode;
}
void TransformCollisionVector(const s16 *input, s32 *output) {
    (void)input; (void)output;
}
MATRIX *RotMatrix(SVECTOR *rotation, MATRIX *matrix) {
    (void)rotation;
    return matrix;
}
void SetRotMatrix(MATRIX *matrix) { (void)matrix; }
s32 rsin(s32 angle) { (void)angle; return 0; }

static TrackEventData s_events;
static int s_failures;

static void Check(int condition, const char *what, s32 got, s32 wanted) {
    if (condition) return;
    printf("FAIL %s: got %d, expected %d\n", what, got, wanted);
    s_failures++;
}

static void Reset(void) {
    memset(&s_events, 0, sizeof(s_events));
    g_TrackEventData = &s_events;
    g_RaceSeries = 0;
    g_TrackLength = 0x8000;
}

/* Enough speed that the crest scan runs at all. */
static void PlaceCar(GameCarRuntime *car, s32 from, s32 to) {
    memset(car, 0, sizeof(*car));
    car->speed = 0x400;
    car->previousTrackProgress = from;
    car->trackProgress = to;
}

static void CrestTests(void) {
    GameCarRuntime car;
    TrackCrestEvent *row = &s_events.crestEvents[0][0];

    Reset();
    row[0].progress = 0x100;
    row[0].motionValue = 11;
    row[1].progress = 0x200;
    row[1].motionValue = 22;
    row[2].motionValue = -1; /* the list ends here */

    /* Driving onto a crest reports it; the crest's own position counts as
     * crossed, the position the car came from does not. */
    PlaceCar(&car, 0x0FF, 0x100);
    Check(GetCarCrestTrigger(&car) == 11, "crest reached exactly",
          GetCarCrestTrigger(&car), 11);

    PlaceCar(&car, 0x100, 0x101);
    Check(GetCarCrestTrigger(&car) == 0, "crest already behind",
          GetCarCrestTrigger(&car), 0);

    /* The second entry is reachable, so the scan does not stop at the first. */
    PlaceCar(&car, 0x1F0, 0x210);
    Check(GetCarCrestTrigger(&car) == 22, "second crest",
          GetCarCrestTrigger(&car), 22);

    /* Nothing past the sentinel is ever read, even when it would match. */
    row[2].progress = 0x300;
    PlaceCar(&car, 0x2F0, 0x310);
    Check(GetCarCrestTrigger(&car) == 0, "past the end of the list",
          GetCarCrestTrigger(&car), 0);
    row[2].progress = 0;

    /* Too slow to trigger anything. */
    PlaceCar(&car, 0x0FF, 0x100);
    car.speed = 0x31F;
    Check(GetCarCrestTrigger(&car) == 0, "below the speed floor",
          GetCarCrestTrigger(&car), 0);

    /* Driving backwards over the same crest still reports it: the pair is
     * sorted before it is compared. */
    PlaceCar(&car, 0x100, 0x0FF);
    Check(GetCarCrestTrigger(&car) == 11, "crossed backwards",
          GetCarCrestTrigger(&car), 11);

    /* A jump of a whole lap is the counter wrapping, not a stretch of track,
     * and must not fire every crest between the two ends. */
    PlaceCar(&car, 0x7FFF, 0x10);
    Check(GetCarCrestTrigger(&car) == 0, "lap wrap fires nothing",
          GetCarCrestTrigger(&car), 0);

    /* A reversed race measures from the other end of the track. */
    g_RaceSeries = 1;
    PlaceCar(&car, g_TrackLength - 0x0FF, g_TrackLength - 0x100);
    Check(GetCarCrestTrigger(&car) == 11, "reversed race",
          GetCarCrestTrigger(&car), 11);
    g_RaceSeries = 0;
}

static void TargetSpeedTests(void) {
    GameCarRuntime car;
    TrackAiSpeedKey *keys = s_events.aiSpeedKeys[0];

    Reset();
    keys[0].progress = 0x10;
    keys[1].progress = 0x20;
    keys[0].slotTargetSpeeds[0] = 100;
    keys[1].slotTargetSpeeds[0] = 200;
    keys[0].slotTargetSpeeds[2] = 300;
    keys[1].slotTargetSpeeds[2] = 500;
    keys[0].slotTargetSpeeds[3] = 400;
    keys[1].slotTargetSpeeds[3] = 400;
    keys[0].pitch = 7;

    /* Halfway between two keys is halfway between their speeds, put through
     * the same scaling the game applies. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x18 << 4;
    car.routeMarkerIndex = 0;
    car.routeMarkerActive = 0;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.accelerationLimit == (((150 * 1168) / 160) * 6) / 100,
          "limit halfway between two keys", car.accelerationLimit,
          (((150 * 1168) / 160) * 6) / 100);

    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x18 << 4;
    UpdateCarAiTargetSpeed(&car, 2);
    Check(car.accelerationLimit == (((400 * 1168) / 160) * 6) / 100,
          "third car uses third target-speed column", car.accelerationLimit,
          (((400 * 1168) / 160) * 6) / 100);

    /* Past the far key the marker steps forward and no limit is set. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x30 << 4;
    car.routeMarkerIndex = 0;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.routeMarkerIndex == 1, "marker steps forward past the pair",
          car.routeMarkerIndex, 1);
    Check(car.accelerationLimit == 0, "no limit while off the pair",
          car.accelerationLimit, 0);

    /* Short of the near key it steps back. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x1000;
    car.routeMarkerIndex = 5;
    keys[5].progress = 0x200;
    keys[6].progress = 0x300;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.routeMarkerIndex == 4, "marker steps back before the pair",
          car.routeMarkerIndex, 4);

    /* Cars behind the front four use fourth place's target, tapered by their
     * grid slot. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x18 << 4;
    car.routeMarkerIndex = 0;
    UpdateCarAiTargetSpeed(&car, 5);
    Check(car.accelerationLimit ==
              (((((400 * (0x55 - 5)) / 100) * 1168) / 160) * 6) / 100,
          "fifth car tapers fourth place's target", car.accelerationLimit,
          (((((400 * (0x55 - 5)) / 100) * 1168) / 160) * 6) / 100);

    /* Two keys at the same position must not divide by zero. */
    memset(&car, 0, sizeof(car));
    keys[0].progress = 0x40;
    keys[1].progress = 0x40;
    car.trackProgress = 0x40 << 4;
    car.routeMarkerIndex = 0;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.accelerationLimit == (((100 * 1168) / 160) * 6) / 100,
          "two keys at one position", car.accelerationLimit,
          (((100 * 1168) / 160) * 6) / 100);

    /* A stale marker at the lap boundary is reset before the table is read,
     * so this frame already uses the first pair. */
    keys[0].progress = 0x10;
    keys[1].progress = 0x20;
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x18 << 4;
    car.routeMarkerIndex = 5;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.routeMarkerIndex == 0, "lap start resets speed marker",
          car.routeMarkerIndex, 0);
    Check(car.accelerationLimit == (((150 * 1168) / 160) * 6) / 100,
          "lap start immediately uses first speed pair", car.accelerationLimit,
          (((150 * 1168) / 160) * 6) / 100);

    /* A negative signed marker is invalid input, not permission to read the
     * key immediately before the table. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x18 << 4;
    car.routeMarkerIndex = -1;
    UpdateCarAiTargetSpeed(&car, 0);
    Check(car.routeMarkerIndex == 0, "negative speed marker resets",
          car.routeMarkerIndex, 0);
    Check(car.accelerationLimit == (((150 * 1168) / 160) * 6) / 100,
          "negative marker uses first speed pair", car.accelerationLimit,
          (((150 * 1168) / 160) * 6) / 100);
}

static void RouteMarkerSeedTests(void) {
    TrackAiSpeedKey *keys = s_events.aiSpeedKeys[0];
    s32 carIndex;

    Reset();
    keys[0].progress = 0x300;
    keys[1].progress = 0x200;
    keys[2].progress = 0x100;
    keys[3].progress = -1;
    memset(g_Cars, 0, sizeof(g_Cars));
    g_Cars[0].trackProgress = 0x350 << 4;
    g_Cars[1].trackProgress = 0x250 << 4;
    g_Cars[2].trackProgress = 0x150 << 4;
    g_Cars[3].trackProgress = 0x050 << 4;

    SeedCarRouteMarkers();

    Check(g_Cars[0].routeMarkerIndex == 0, "seed at first speed key",
          g_Cars[0].routeMarkerIndex, 0);
    Check(g_Cars[1].routeMarkerIndex == 1, "seed at second speed key",
          g_Cars[1].routeMarkerIndex, 1);
    Check(g_Cars[2].routeMarkerIndex == 2, "seed at third speed key",
          g_Cars[2].routeMarkerIndex, 2);
    Check(g_Cars[3].routeMarkerIndex == 0, "seed before all speed keys",
          g_Cars[3].routeMarkerIndex, 0);
    for (carIndex = 0; carIndex < RACE_CAR_SLOT_COUNT; carIndex++) {
        Check(g_Cars[carIndex].routeMarkerActive == 1,
              "seed activates route marker", g_Cars[carIndex].routeMarkerActive,
              1);
    }
}

static void RacingLineTests(void) {
    GameCarRuntime car;
    TrackRacingLineHint *hints = s_events.racingLineHints[0];

    Reset();
    hints[0].start = 0x100;
    hints[0].end = 0x200;
    hints[0].minHeight = 10;
    hints[0].maxHeight = 90;
    hints[0].heightAdjustment = 5;
    hints[1].start = 0x300;
    hints[1].end = 0x400;
    hints[2].start = -1; /* the list ends here */

    /* Inside the stretch, a leading car with clear air drifts by the hint's
     * own step. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x180 << 4;
    car.aiLateralOffset = 50;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.aiLateralOffset == 55, "drift towards the line",
          car.aiLateralOffset, 55);

    /* Traffic alongside holds the line where it is. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x180 << 4;
    car.aiLateralOffset = 50;
    car.nearbyCarCount = 1;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.aiLateralOffset == 50, "traffic holds the line",
          car.aiLateralOffset, 50);

    /* So does being outside the front four. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x180 << 4;
    car.aiLateralOffset = 50;
    ApplyCarRacingLineHint(&car, 4);
    Check(car.aiLateralOffset == 50, "back of the field holds the line",
          car.aiLateralOffset, 50);

    /* An offset already outside the hint's band is left alone. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x180 << 4;
    car.aiLateralOffset = 90;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.aiLateralOffset == 90, "offset at the band's edge",
          car.aiLateralOffset, 90);

    /* The far edge still belongs to this stretch. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x200 << 4;
    car.aiLateralOffset = 50;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.routeIndex == 0, "the stretch's last position is still inside",
          car.routeIndex, 0);
    Check(car.aiLateralOffset == 55, "and still drifts", car.aiLateralOffset,
          55);

    /* So does the near one. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x100 << 4;
    car.aiLateralOffset = 50;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.aiLateralOffset == 55, "the stretch's first position drifts",
          car.aiLateralOffset, 55);

    /* Past the stretch, the next one is taken. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x250 << 4;
    car.racingLineHintState = 9;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.routeIndex == 1, "advance to the next hint", car.routeIndex, 1);
    Check(car.racingLineHintState == 0, "advancing clears the hint state",
          car.racingLineHintState, 0);

    /* Past the last one, the list starts over rather than reading the -1. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x450 << 4;
    car.routeIndex = 1;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.routeIndex == 0, "wrap at the end of the list", car.routeIndex, 0);

    /* Short of the stretch, nothing moves but the state clears. */
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x080 << 4;
    car.aiLateralOffset = 50;
    car.racingLineHintState = 9;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.aiLateralOffset == 50, "before the stretch, no drift",
          car.aiLateralOffset, 50);
    Check(car.racingLineHintState == 0, "before the stretch, state clears",
          car.racingLineHintState, 0);

    /* Resetting the route index at the start of a lap must also select the
     * first hint immediately, rather than retaining the old hint for a frame. */
    hints[0].start = 0;
    hints[0].end = 0x20;
    memset(&car, 0, sizeof(car));
    car.trackProgress = 0x10 << 4;
    car.routeIndex = 1;
    car.aiLateralOffset = 50;
    ApplyCarRacingLineHint(&car, 1);
    Check(car.routeIndex == 0, "lap start resets the racing-line hint",
          car.routeIndex, 0);
    Check(car.aiLateralOffset == 55, "lap start uses the first hint immediately",
          car.aiLateralOffset, 55);
}

int main(void) {
    CrestTests();
    TargetSpeedTests();
    RouteMarkerSeedTests();
    RacingLineTests();

    if (s_failures != 0) {
        printf("%d AI table checks failed\n", s_failures);
        return 1;
    }
    printf("the AI walks the track's crest, speed and racing-line tables\n");
    return 0;
}
