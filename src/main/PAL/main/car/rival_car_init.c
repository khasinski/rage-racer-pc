#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

static s16 ClampS16(s16 value, s16 minimum, s16 maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

void InitRivalCarAi(GameCarRuntime *car,
                    s32 gridPosition,
                    RaceGridSlot *grid) {
    s32 configIndex = grid[gridPosition].value;
    const TrackRivalAiConfig *config;
    GameCarAiBlock *ai = GetCarAiBlock(car);

    if ((u32)configIndex >= 12) configIndex = 0;
    config = &g_TrackEventData->rivalAiConfigs[g_RaceSeries][configIndex];

    car->targetSpeed = (config->speed * 1168) / 160;
    car->accelerationStep = config->accelerationStep;
    car->boostAccelerationThreshold =
        ClampS16((s16)config->boostAccelerationThreshold, 0, 10);
    ai->collisionBoostDuration =
        ClampS16((s16)config->collisionBoostDuration, 0, INT16_MAX);
    ai->boostAcceleration =
        ClampS16((s16)config->boostAcceleration, 0, 15);
    ai->boostTimer = 0;
    ai->minimumSpeed =
        ClampS16((s16)config->minimumSpeed, 60, INT16_MAX);
    ai->engineRpmLow =
        ClampS16((s16)config->initialEngineRpm, 0, INT16_MAX);
    ai->accelerationLimit = ai->targetSpeed * 6 / 100;
    ai->gridTargetProgress = g_TrackLength / 12;
    if (gridPosition >= 4) {
        ai->gridTargetProgress +=
            (g_TrackLength / 40) * (gridPosition - 4);
    }
}
