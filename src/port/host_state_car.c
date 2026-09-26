/*
 * Retail state belonging to the cars: the player's and the rivals' physical
 * condition, the drivetrain, the tyres and their grip, the collision and
 * knockback bookkeeping, and the rival AI's view of the race.
 *
 * The camera's subject is here too, since the car code sets it. Order is
 * retail's address order.
 */

#include <stddef.h>

#include "common.h"
#include "game/car.h"
#include "game/car_runtime_state.h"
#include "game/track.h"

LaunchSpeedThreshold g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT] = {
        {960, 320}, {960, 320}, {960, 320}, {960, 320}, {960, 320}
    };
const GameTrackPoint *g_TrackPoints;
s16 g_RivalCueEnabled;
s32 g_TrackPointCount;
s16 g_PlayerAutoSteer;
s32 g_EngineRpm;
const RaceIntroCameraScript *g_RaceIntroCameraScript;
FinishCamera g_FinishCamera;
s32 g_RaceSeries;
s32 g_TachoShiftLightOn;
GameCarRuntime *g_RankedCars[RIVAL_CONTENDER_COUNT];
s32 g_TrackLength;
const TrackEventData *g_TrackEventData;
s32 g_EngineRpmJitter;
GameCarSpec *g_CarSpec;
s32 g_RivalCueFlags;
s32 g_ClosestRivalRank;

CarPerformance g_CarPerformance;
