#include "common.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
GameCarRuntime *g_RankedCars[4];
s32 g_TrackLength;
s32 g_AnimTimer;
s32 g_ClosestRivalRank;

static int s_traceCalls;
static int s_trafficCalls;
static int s_collisionCalls;
static int s_targetSpeedCalls;
static int s_hintCalls;
static int s_clampCalls;
static int s_steerCalls;
static int s_moveCalls;
static int s_progressCalls;
static int s_knockbackCalls;
static int s_trackStateCalls;
static int s_bodyMotionCalls;
static int s_attractAccelerationCalls;
static int s_placementCalls;
static int s_raceAccelerationCalls;
static int s_rankCalls;
static int s_rubberBandCalls;
static s32 s_slowedRanks[3];
static int s_slowedCount;
static char s_events[128];
static size_t s_eventCount;

static void RecordEvent(char event) {
    if (s_eventCount != 0 && s_events[s_eventCount - 1] == event) {
        return;
    }
    if (s_eventCount + 1 < sizeof(s_events)) {
        s_events[s_eventCount++] = event;
        s_events[s_eventCount] = '\0';
    }
}

void TraceCarStates(void) {
    RecordEvent('T');
    s_traceCalls++;
}

void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    RecordEvent('A');
    s_trafficCalls++;
}

s32 CollideRivalCars(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    RecordEvent('C');
    s_collisionCalls++;
    return 0;
}

void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex) {
    (void)car;
    (void)carIndex;
    RecordEvent('S');
    s_targetSpeedCalls++;
}

void ApplyCarRacingLineHint(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    RecordEvent('H');
    s_hintCalls++;
}

void ClampCarLateralOffset(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    RecordEvent('L');
    s_clampCalls++;
}

void SteerCarAlongRoute(GameCarRuntime *car) {
    (void)car;
    RecordEvent('R');
    s_steerCalls++;
}

void MoveRivalCars(void) {
    RecordEvent('M');
    s_moveCalls++;
}

void AccumulateLapProgress(GameCarRuntime *car) {
    (void)car;
    s_progressCalls++;
}

void ApplyCarKnockback(GameCarRuntime *car) {
    (void)car;
    s_knockbackCalls++;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 point,
                        const CarTrackLimits *limits) {
    (void)car;
    (void)point;
    if (limits->leftInset != -0x3C || limits->rightInset != 0x3C) {
        return -1;
    }
    s_trackStateCalls++;
    return 0;
}

void UpdateRivalBodyMotion(void) {
    RecordEvent('B');
    s_bodyMotionCalls++;
}
void AccelerateRaceRivals(void) {
    RecordEvent('E');
    s_raceAccelerationCalls++;
}
void AccelerateAttractRivals(void) {
    RecordEvent('E');
    s_attractAccelerationCalls++;
}
void PlaceRivalCarsOnTrack(void) {
    RecordEvent('P');
    s_placementCalls++;
}

s32 GetAngleDelta(s32 from, s32 to) { return to - from; }

void RankContenders(void) {
    RecordEvent('K');
    s_rankCalls++;
}
void UpdateRivalRubberBand(void) {
    RecordEvent('U');
    s_rubberBandCalls++;
}
void SlowRivalAhead(GameCarRuntime *car, s32 index) {
    (void)car;
    RecordEvent('W');
    if (s_slowedCount < 3) {
        s_slowedRanks[s_slowedCount++] = index;
    }
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

#define CHECK_EVENTS(expected) do {                                            \
    if (strcmp(s_events, (expected)) != 0) {                                   \
        fprintf(stderr, "line %d: events = %s, expected %s\n", __LINE__,     \
                s_events, (expected));                                         \
        return 1;                                                              \
    }                                                                          \
} while (0)

int main(void) {
    GameCarRuntime *car;
    s32 index;

    memset(g_Cars, 0, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].activeFlag = -1;
        g_Cars[index].reservedF8 = 99;
        g_Cars[index].collisionFlag = 7;
        g_Cars[index].baseBodyYaw = index * 10;
    }
    car = &g_Cars[0];
    car->activeFlag = 0;
    car->progressA = 123;
    car->speed = 100;
    car->acceleration = 10;
    car->accelerationStep = 5;
    car->accelerationLimit = 20;
    car->targetYaw = 50;
    car->motionTimer = 2;
    g_TrackLength = 0;
    s_eventCount = 0;
    s_events[0] = '\0';

    UpdateAttractCars();

    CHECK_EQ(car->progressA, 123);
    CHECK_EQ(car->reservedF8, 0);
    CHECK_EQ(car->collisionFlag, 0);
    CHECK_EQ(car->bodyYaw, 0);
    CHECK_EQ(g_Cars[1].reservedF8, 0);
    CHECK_EQ(g_Cars[1].bodyYaw, 10);
    CHECK_EQ(s_traceCalls, 1);
    CHECK_EQ(s_trafficCalls, 1);
    CHECK_EQ(s_collisionCalls, RACE_CAR_SLOT_COUNT - 1);
    CHECK_EQ(s_targetSpeedCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_hintCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_clampCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_steerCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_moveCalls, 1);
    CHECK_EQ(s_progressCalls, 0);
    CHECK_EQ(s_knockbackCalls, 0);
    CHECK_EQ(s_trackStateCalls, 0);
    CHECK_EQ(s_bodyMotionCalls, 1);
    CHECK_EQ(s_attractAccelerationCalls, 1);
    CHECK_EQ(s_placementCalls, 1);
    CHECK_EVENTS("TACSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLR"
                 "SHLREMPB");

    g_TrackLength = 100;
    car->progressA = 123;
    UpdateAttractCars();
    CHECK_EQ(car->progressA, 23);

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].activeFlag = 0;
        g_Cars[index].collisionFlag = 6;
        g_Cars[index].baseBodyYaw = 100 + index;
    }
    for (index = 0; index < 4; index++) {
        g_RankedCars[index] = &g_Cars[index];
    }
    g_AnimTimer = 0;
    g_ClosestRivalRank = 99;
    s_trafficCalls = 0;
    s_collisionCalls = 0;
    s_targetSpeedCalls = 0;
    s_hintCalls = 0;
    s_clampCalls = 0;
    s_steerCalls = 0;
    s_slowedCount = 0;
    s_eventCount = 0;
    s_events[0] = '\0';

    UpdateRaceCars();
    CHECK_EQ(s_rankCalls, 1);
    CHECK_EQ(s_trafficCalls, 8);
    CHECK_EQ(s_collisionCalls, RACE_CAR_SLOT_COUNT - 1);
    CHECK_EQ(s_targetSpeedCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_hintCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_clampCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_steerCalls, RACE_CAR_SLOT_COUNT);
    CHECK_EQ(s_rubberBandCalls, 1);
    CHECK_EQ(s_slowedCount, 3);
    CHECK_EQ(s_slowedRanks[0], 3);
    CHECK_EQ(s_slowedRanks[1], 2);
    CHECK_EQ(s_slowedRanks[2], 1);
    CHECK_EQ(s_raceAccelerationCalls, 1);
    CHECK_EQ(g_Cars[5].bodyYaw, 105);
    CHECK_EQ(g_Cars[5].collisionFlag, 0);
    CHECK_EVENTS("KACSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLRSHLR"
                 "UWEMPB");

    puts("attract car update preserves pass coverage and empty-track progress");
    return 0;
}
