#include "game/driver.h"
#include "game/angle.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <string.h>

enum {
    PLAYER_RENDER_MODEL_INDEX = 0x17,
    RACE_DIRECTION_BIT = 1,
};

/* The CUSTOMIZE screen stores the AT/MT choice in the car table; the race car
 * is a separate object from the showroom preview, so read it from there
 * rather than trusting whatever the runtime last held. */
static s16 PlayerTransmission(const PlayerCarRuntime *car) {
    if (g_CarTable != NULL && (u32)g_PlayerCarIndex < GAME_CAR_COUNT) {
        return g_CarTable[g_PlayerCarIndex].transmission != 0;
    }
    return car->drive.manual;
}

static void ResetPlayerDrivingGlobals(void) {
    g_EngineRpmJitter = 0;
    g_EngineRpm = 0;
    g_TachoShiftLightOn = 0;
    g_WrongWayTimer = 0;
    g_PlayerAutoSteer = 0;
}

void InitPlayerCar(PlayerCarRuntime *car) {
    g_RacePhase = RACE_PHASE_ACTIVE;
    g_RaceSeries = g_GrandPrixSeries & RACE_DIRECTION_BIT;
    BuildTachometerFace(&g_CarSpec->tachometer);

    const TrackRoute route = {.points = g_TrackPoints, .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount, .length = g_TrackLength};
    const DriverStart start = {.route = &route,
        .position = g_TrackEventData != NULL ? &g_TrackEventData->rivalStarts[g_RaceSeries][0] : NULL,
        .walkStart = g_TrackEventData != NULL ? g_TrackEventData->trackWalkStart : 0,
        .reverse = g_RaceSeries, .manual = PlayerTransmission(car),
        .launchThresholdIndex = car->drive.launchThresholdIndex,
        .modelIndex = PLAYER_RENDER_MODEL_INDEX};
    InitDriver(car, g_CarSpec, &g_CarPerformance, &start);
    ResetPlayerDrivingGlobals();
}
