#include "game/car_internal.h"
#include "game/rival.h"
#include "game/race.h"

void SeedCarAiSpeedKeys(void) {
    if (g_TrackEventData == NULL) return;
    const TrackAiSpeedKey *table = g_TrackEventData->aiSpeedKeys[g_RaceSeries != 0];
    for (s32 slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) {
        SeedRivalSpeedKey(&g_Cars[slot], table);
    }
}

void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex) {
    if (g_TrackEventData == NULL) return;
    const s32 reverse = g_RaceSeries != 0;
    StepRivalTargetSpeed(car, carIndex, g_TrackEventData->aiSpeedKeys[reverse], reverse);
}

static void AccelerateRivals(int racing) {
    for (s32 slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) {
        StepRivalAcceleration(&g_Cars[slot], racing);
    }
}

void AccelerateRaceRivals(void) { AccelerateRivals(1); }
void AccelerateAttractRivals(void) { AccelerateRivals(0); }

void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex) {
    if (g_TrackEventData == NULL) return;
    StepRivalLine(car, carIndex, g_TrackEventData->racingLineHints[g_RaceSeries != 0]);
}

void MoveRivalCars(void) {
    for (s32 slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) MoveRival(&g_Cars[slot], slot);
}

void UpdateRivalBodyMotion(void) {
    for (s32 slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) {
        FinishRival(&g_Cars[slot], g_TrackEventData, g_TrackLength, g_RaceSeries != 0);
    }
}

