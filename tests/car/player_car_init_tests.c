#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

s16 g_GrandPrixSeries;
s32 g_RaceSeries;
s16 g_RacePhase;
s32 g_AutoShiftCooldown;
s16 g_TrackZoneDark;
s32 g_ShiftSoundLevel;
s32 g_RoadGrade;
s32 g_ShiftTargetRpm;
s32 g_EngineRpmJitter;
s32 g_EngineRpm;
s32 g_EngineRpmSnapshot;
s32 g_StandingStartSpin;
s32 g_DriveBoostTimer;
u16 g_HudGlyphClut;
s16 g_DragScale;
s16 g_SteerHoldFrames;
s16 g_GripLossTimer;
s16 g_WrongWayTimer;
s16 g_PlayerAutoSteer;
TrackEventData *g_TrackEventData;
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;

static TrackEventData s_eventData;
static GameTrackPoint s_points[2];
static int s_tachoCalls;
static int s_findCalls;
static int s_seedCalls;
static int s_trackCalls;
static int s_offsetCalls;
static int s_performanceCalls;
static int s_failures;

void BuildTachoNeedleQuad(void) { s_tachoCalls++; }

s32 FindTrackSegment(GameCarRuntime *car, s32 pointIndex) {
    (void)car;
    (void)pointIndex;
    s_findCalls++;
    return 1;
}

void SeedCarLapProgress(GameCarRuntime *car, s32 progress) {
    (void)progress;
    car->progressA = 123;
    s_seedCalls++;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 pointIndex,
                        const CarTrackLimits *limits) {
    (void)pointIndex;
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

s32 IsCarFacingBackwards(const PlayerCarRuntime *car) {
    (void)car;
    return 1;
}

void PrepareCarPerformance(GameCarDrive *drive) {
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
    s_points[1].angle = 0x200;
    g_TrackEventData = &s_eventData;
    g_TrackPoints = s_points;
    g_TrackPointCount = 2;
    g_GrandPrixSeries = 3;
    g_AutoShiftCooldown = 99;
    g_TrackZoneDark = 3;
    g_ShiftSoundLevel = 99;
    g_RoadGrade = 99;
    g_ShiftTargetRpm = 99;
    g_EngineRpmJitter = 99;
    g_EngineRpm = 99;
    g_EngineRpmSnapshot = 99;
    g_StandingStartSpin = 99;
    g_DriveBoostTimer = 99;
    g_HudGlyphClut = 0;
    g_DragScale = 0;
    g_SteerHoldFrames = 99;
    g_GripLossTimer = 99;
    g_WrongWayTimer = 99;
    g_PlayerAutoSteer = 99;
    s_tachoCalls = 0;
    s_findCalls = 0;
    s_seedCalls = 0;
    s_trackCalls = 0;
    s_offsetCalls = 0;
    s_performanceCalls = 0;
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
    CHECK(car.positionW == 0 && car.bodyRotationW == 0 && car.field_4C == 0);
    CHECK(car.lapTimes.words[11] == 0);

    CHECK(g_AutoShiftCooldown == 0 && g_TrackZoneDark == 0);
    CHECK(g_ShiftSoundLevel == 0 && g_RoadGrade == 0 && g_ShiftTargetRpm == 0);
    CHECK(g_EngineRpmJitter == 0 && g_EngineRpm == 0 &&
          g_EngineRpmSnapshot == 0);
    CHECK(g_StandingStartSpin == 0 && g_DriveBoostTimer == 0);
    CHECK(g_HudGlyphClut == 0x7800 && g_DragScale == 1000);
    CHECK(g_SteerHoldFrames == 0 && g_GripLossTimer == 0);
    CHECK(g_WrongWayTimer == 0 && g_PlayerAutoSteer == 0);

    ResetFixtures();
    memset(&car, 0x55, sizeof(car));
    car.drive.manual = 0;
    car.drive.launchThresholdIndex = 4;
    InitPlayerCar(&car);
    CHECK(car.drive.manual == 0 && car.drive.launchThresholdIndex == 4);
    CHECK(g_HudGlyphClut == 0x78CF);

    if (s_failures != 0) {
        printf("%d player initialization checks failed\n", s_failures);
        return 1;
    }
    puts("player initialization fully resets runtime state");
    return 0;
}
