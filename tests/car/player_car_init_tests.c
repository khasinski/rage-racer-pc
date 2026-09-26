#include "game/driver.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameCarSpec *g_CarSpec;
CarPerformance g_CarPerformance;

CarEntry *g_CarTable;
s32 g_PlayerCarIndex;
s16 g_GrandPrixSeries;
s32 g_RaceSeries;
s16 g_RacePhase;
s32 g_EngineRpmJitter;
s32 g_EngineRpm;
s32 g_TachoShiftLightOn;
s16 g_WrongWayTimer;
s16 g_PlayerAutoSteer;
const TrackEventData *g_TrackEventData;
const GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_TrackLength;
const GameTrackArcCenter *g_TrackArcCenters;

static TrackEventData s_eventData;
static GameTrackPoint s_points[2];
static int s_tachoCalls;
static int s_findCalls;
static int s_seedCalls;
static int s_trackCalls;
static int s_offsetCalls;
static int s_performanceCalls;
static int s_failures;
static s32 s_findResult;

void BuildTachometerFace(const CarTachometerSpec *spec) { (void)spec; s_tachoCalls++; }

s32 FindCarTrackSegment(const GameCarRuntime *car, const TrackRoute *route, s32 pointIndex) {
    (void)car;
    (void)route;
    (void)pointIndex;
    s_findCalls++;
    return s_findResult;
}

void SeedCarTrackProgress(GameCarRuntime *car, const TrackRoute *route, s32 startIndex, s32 seedSelector, int reverse) {
    (void)route;
    (void)startIndex;
    (void)reverse;
    (void)seedSelector;
    car->progressA = 123;
    s_seedCalls++;
}

s32 StepCarTrackState(GameCarRuntime *car, const TrackRoute *route, s32 pointIndex,
                        const CarTrackLimits *limits, int reverse, int knockback) {
    (void)pointIndex;
    (void)route;
    (void)reverse;
    (void)knockback;
    (void)limits;
    car->y = 40;
    car->trackProgress = 500;
    s_trackCalls++;
    return 0;
}

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    car->motionX = 5;
    car->motionY = 0;
    car->motionZ = -7;
    s_offsetCalls++;
}

s32 CarFacesBackwards(const PlayerCarRuntime *car, const TrackRoute *route) {
    (void)car;
    (void)route;
    return 1;
}

void PrepareCarPerformance(GameCarDrive *drive, GameCarSpec *spec,
                           CarPerformance *performance) {
    (void)spec;
    (void)performance;
    if (drive->motionState != CAR_MOTION_STANDING_START ||
        drive->gear != 1 || drive->drivetrainCoupled != 1) {
        s_failures++;
    }
    drive->speedScale = 777;
    s_performanceCalls++;
}

static void ResetFixtures(void) {
    memset(&s_eventData, 0, sizeof(s_eventData));
    memset(s_points, 0, sizeof(s_points));
    s_eventData.rivalStarts[1][0].x = 1000;
    s_eventData.rivalStarts[1][0].z = 2000;
    s_eventData.rivalStarts[1][0].trackPointIndex = 0;
    s_points[0].x = 300;
    s_points[0].z = 400;
    s_points[1].angle = 0x200;
    g_TrackEventData = &s_eventData;
    g_TrackPoints = s_points;
    g_TrackPointCount = 2;
    g_TrackLength = 1000;
    g_GrandPrixSeries = 3;
    g_EngineRpmJitter = 99;
    g_EngineRpm = 99;
    g_TachoShiftLightOn = 1;
    g_WrongWayTimer = 99;
    g_PlayerAutoSteer = 99;
    s_tachoCalls = 0;
    s_findCalls = 0;
    s_seedCalls = 0;
    s_trackCalls = 0;
    s_offsetCalls = 0;
    s_performanceCalls = 0;
    s_findResult = 1;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    ResetFixtures();
    memset(&car, 0x7F, sizeof(car));
    car.drive.manual = 1;
    car.drive.launchThresholdIndex = 3;
    InitPlayerCar(&car);

    CHECK(g_RacePhase == 2 && g_RaceSeries == 1);
    CHECK(s_tachoCalls == 1 && s_findCalls == 1 && s_seedCalls == 1);
    CHECK(s_trackCalls == 1 && s_offsetCalls == 1 && s_performanceCalls == 1);
    CHECK(car.x == 1005 && car.y == 40 && car.z == 1993);
    CHECK(car.trackPointIndex == 1 && car.previousTrackPointIndex == 1);
    CHECK(car.bodyYaw == 0x200 && car.headingAngle == 0x200);
    CHECK(car.modelYaw == car.bodyYaw && car.modelY == 40);
    CHECK(car.progressA == 123 && car.previousTrackProgress == 500);
    CHECK(car.facingBackwards == 1 && car.modelIndex == 0x17);
    CHECK(car.drive.manual == 1 && car.drive.launchThresholdIndex == 3);
    CHECK(car.drive.motionState == CAR_MOTION_STANDING_START);
    CHECK(car.drive.gear == 1 && car.drive.gearDisp == 1);
    CHECK(car.drive.drivetrainCoupled == 1 && car.drive.racePosition == 1);
    CHECK(car.drive.hudLapHighlightRow == -1 && car.drive.speedScale == 777);
    CHECK(car.drive.acceleratorInput.value == 0 && car.drive.brakeInput == 0);
    CHECK(car.drive.spinRate == 0 && car.drive.yawOffset == 0);
    CHECK(car.positionW == 0 && car.bodyRotationW == 0 && car.reserved4C == 0);
    CHECK(car.lapTimes.words[11] == 0);

    CHECK(car.drive.autoShiftCooldown == 0);
    CHECK(car.drive.shiftSoundLevel == 0 && car.drive.roadGrade == 0 && car.drive.shiftTargetRpm == 0);
    CHECK(car.drive.shiftTargetSpeed == 0);
    CHECK(g_EngineRpmJitter == 0 && g_EngineRpm == 0 && g_TachoShiftLightOn == 0);
    CHECK(car.drive.standingStartSpin == 0 && car.drive.driveBoostTimer == 0);
    CHECK(car.drive.dragScale == 1000);
    CHECK(car.drive.steerHoldFrames == 0 && car.drive.gripLossTimer == 0);
    CHECK(g_WrongWayTimer == 0 && g_PlayerAutoSteer == 0);

    ResetFixtures();
    memset(&car, 0x55, sizeof(car));
    car.drive.manual = 0;
    car.drive.launchThresholdIndex = 4;
    InitPlayerCar(&car);
    CHECK(car.drive.manual == 0 && car.drive.launchThresholdIndex == 4);

    ResetFixtures();
    memset(&car, 0x55, sizeof(car));
    car.drive.manual = 1;
    car.drive.launchThresholdIndex = 2;
    s_findResult = -1;
    InitPlayerCar(&car);
    CHECK(car.trackPointIndex == 0);
    CHECK(car.bodyYaw == ANGLE_QUARTER_TURN);
    CHECK(car.x == 305 && car.z == 393);

    ResetFixtures();
    memset(&car, 0x55, sizeof(car));
    car.drive.manual = 1;
    car.drive.launchThresholdIndex = 2;
    g_TrackEventData = NULL;
    InitPlayerCar(&car);
    CHECK(s_findCalls == 0 && s_seedCalls == 0 && s_trackCalls == 0);
    CHECK(s_offsetCalls == 0 && s_performanceCalls == 1);
    CHECK(car.x == 0 && car.y == 0 && car.z == 0);

    ResetFixtures();
    memset(&car, 0x55, sizeof(car));
    car.drive.manual = 1;
    car.drive.launchThresholdIndex = 2;
    g_TrackPoints = NULL;
    InitPlayerCar(&car);
    CHECK(s_findCalls == 0 && s_seedCalls == 0 && s_trackCalls == 0);
    CHECK(car.x == 0 && car.y == 0 && car.z == 0);

    ResetFixtures();
    memset(&car, 0, sizeof(car));
    s_eventData.rivalStarts[1][0].x = INT_MAX;
    s_eventData.rivalStarts[1][0].z = INT_MIN;
    InitPlayerCar(&car);
    CHECK(car.x == INT_MIN + 4);
    CHECK(car.z == INT_MAX - 6);

    /* The race car takes its gearbox from the car table entry the player
     * chose, not from whatever the runtime held before the race. */
    {
        static CarEntry cars[GAME_CAR_COUNT];

        ResetFixtures();
        memset(cars, 0, sizeof(cars));
        g_CarTable = cars;
        g_PlayerCarIndex = 2;
        cars[2].transmission = 1;
        memset(&car, 0, sizeof(car));
        InitPlayerCar(&car);
        CHECK(car.drive.manual == 1);

        ResetFixtures();
        cars[2].transmission = 0;
        memset(&car, 0, sizeof(car));
        car.drive.manual = 1;
        InitPlayerCar(&car);
        CHECK(car.drive.manual == 0);

        ResetFixtures();
        g_PlayerCarIndex = GAME_CAR_COUNT;
        memset(&car, 0, sizeof(car));
        car.drive.manual = 1;
        InitPlayerCar(&car);
        CHECK(car.drive.manual == 1);
        g_CarTable = NULL;
        g_PlayerCarIndex = 0;
    }

    if (s_failures != 0) {
        printf("%d player initialization checks failed\n", s_failures);
        return 1;
    }
    puts("player initialization fully resets runtime state");
    return 0;
}
