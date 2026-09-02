#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

enum {
    RIVAL_CONFIG_COUNT = 12,
    MAX_BOOST_ACCELERATION_THRESHOLD = 10,
    MAX_BOOST_ACCELERATION = 15,
    MIN_RIVAL_SPEED = 60,
};

static s16 ClampSignedConfigValue(u16 encoded, s16 minimum, s16 maximum) {
    s16 value = (s16)encoded;

    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

void InitRivalCarAi(GameCarRuntime *car,
                    s32 gridPosition,
                    RaceGridSlot *grid) {
    s32 configIndex = grid[gridPosition].value;
    const TrackRivalAiConfig *config;
    GameCarAiBlock *ai = GetCarAiBlock(car);

    if ((u32)configIndex >= RIVAL_CONFIG_COUNT) {
        configIndex = 0;
    }
    config = &g_TrackEventData->rivalAiConfigs[
        ReadStableRaceSeries()][configIndex];

    car->targetSpeed = (config->speed * 1168) / 160;
    car->accelerationStep = config->accelerationStep;
    car->boostAccelerationThreshold =
        ClampSignedConfigValue(config->boostAccelerationThreshold, 0,
                               MAX_BOOST_ACCELERATION_THRESHOLD);
    ai->collisionBoostDuration =
        ClampSignedConfigValue(config->collisionBoostDuration, 0, INT16_MAX);
    ai->boostAcceleration =
        ClampSignedConfigValue(config->boostAcceleration, 0,
                               MAX_BOOST_ACCELERATION);
    ai->boostTimer = 0;
    ai->minimumSpeed =
        ClampSignedConfigValue(config->minimumSpeed, MIN_RIVAL_SPEED,
                               INT16_MAX);
    ai->engineRpmLow =
        ClampSignedConfigValue(config->initialEngineRpm, 0, INT16_MAX);
    ai->accelerationLimit = ai->targetSpeed * 6 / 100;
    ai->gridTargetProgress = g_TrackLength / 12;
    if (gridPosition >= 4) {
        ai->gridTargetProgress +=
            (g_TrackLength / 40) * (gridPosition - 4);
    }
}
