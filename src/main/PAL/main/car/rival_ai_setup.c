#include "game/car.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/track.h"

enum {
    MAX_BOOST_ACCELERATION_THRESHOLD = 10,
    MAX_BOOST_ACCELERATION = 15,
    MIN_RIVAL_SPEED = 60,
};

static s16 DecodeClampedConfigValue(u16 encoded,
                                    s16 minimum,
                                    s16 maximum) {
    s16 value = WrapSigned16(encoded);

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
                    const RaceGridSlot *grid) {
    s32 configIndex = grid[gridPosition].value;
    const TrackRivalAiConfig *config;
    GameCarAiBlock *ai = GetCarAiBlock(car);

    if ((u32)configIndex >= TRACK_RIVAL_COUNT) {
        configIndex = 0;
    }
    config = &g_TrackEventData->rivalAiConfigs[
        g_RaceSeries][configIndex];

    car->targetSpeed =
        (DecodeClampedConfigValue((u16)config->speed, 0, INT16_MAX) * 1168) /
        160;
    car->accelerationStep =
        DecodeClampedConfigValue(config->accelerationStep, 0, INT16_MAX);
    car->boostAccelerationThreshold =
        DecodeClampedConfigValue(config->boostAccelerationThreshold, 0,
                                 MAX_BOOST_ACCELERATION_THRESHOLD);
    ai->collisionBoostDuration =
        DecodeClampedConfigValue(config->collisionBoostDuration, 0,
                                 INT16_MAX);
    ai->boostAcceleration =
        DecodeClampedConfigValue(config->boostAcceleration, 0,
                                 MAX_BOOST_ACCELERATION);
    ai->boostTimer = 0;
    ai->minimumSpeed =
        DecodeClampedConfigValue(config->minimumSpeed, MIN_RIVAL_SPEED,
                                 INT16_MAX);
    ai->engineRpmLow =
        DecodeClampedConfigValue(config->initialEngineRpm, 0, INT16_MAX);
    ai->accelerationLimit = ai->targetSpeed * 6 / 100;
    ai->gridTargetProgress = g_TrackLength / 12;
    if (gridPosition >= 4) {
        ai->gridTargetProgress +=
            (g_TrackLength / 40) * (gridPosition - 4);
    }
}
