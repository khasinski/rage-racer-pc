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

void TraceCarStates(void) { s_traceCalls++; }

void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    s_trafficCalls++;
}

s32 CollideRivalCars(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    s_collisionCalls++;
    return 0;
}

void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    s_targetSpeedCalls++;
}

void ApplyCarRacingLineHint(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    s_hintCalls++;
}

void ClampCarLateralOffset(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
    s_clampCalls++;
}

void SteerCarAlongRoute(GameCarRuntime *car) {
    (void)car;
    s_steerCalls++;
}

void MoveRivalCars(void) { s_moveCalls++; }

void AccumulateLapProgress(GameCarRuntime *car) {
    (void)car;
    s_progressCalls++;
}

void ApplyCarKnockback(GameCarRuntime *car) {
    (void)car;
    s_knockbackCalls++;
}

s32 UpdateCarTrackState(GameCarRuntime *car, s32 point,
                        CarTrackLimits *limits) {
    (void)car;
    (void)point;
    if (limits->leftInset != -0x3C || limits->rightInset != 0x3C) {
        return -1;
    }
    s_trackStateCalls++;
    return 0;
}

void UpdateRivalBodyMotion(void) { s_bodyMotionCalls++; }

s32 GetAngleDelta(s32 from, s32 to) { return to - from; }

void RankContenders(void) {}
void UpdateRivalRubberBand(void) {}
void SlowRivalAhead(GameCarRuntime *car, s32 index) {
    (void)car;
    (void)index;
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
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

    UpdateAttractCars();

    CHECK_EQ(car->progressA, 123);
    CHECK_EQ(car->reservedF8, 0);
    CHECK_EQ(car->collisionFlag, 0);
    CHECK_EQ(car->bodyYaw, 10);
    CHECK_EQ(car->acceleration, 15);
    CHECK_EQ(car->speed, 109);
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
    CHECK_EQ(s_progressCalls, 1);
    CHECK_EQ(s_knockbackCalls, 1);
    CHECK_EQ(s_trackStateCalls, 1);
    CHECK_EQ(s_bodyMotionCalls, 1);

    g_TrackLength = 100;
    car->progressA = 123;
    UpdateAttractCars();
    CHECK_EQ(car->progressA, 23);

    puts("attract car update preserves pass coverage and empty-track progress");
    return 0;
}
