#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/track.h"

enum {
    TRAILING_RIVAL_BASE_PERCENT = 85,
    TARGET_SPEED_SCALE = 1168,
    TARGET_SPEED_SOURCE_SCALE = 160,
    ACCELERATION_LIMIT_PERCENT = 6,
    PERCENT_SCALE = 100,
    AI_TABLE_LAP_START_PROGRESS = 0x20,
};

static s32 RivalTargetSpeed(const TrackAiSpeedKey *key, s32 carIndex) {
    if (carIndex < RIVAL_CONTENDER_COUNT) {
        return key->slotTargetSpeeds[carIndex];
    }
    return key->slotTargetSpeeds[RIVAL_CONTENDER_COUNT - 1] *
           (TRAILING_RIVAL_BASE_PERCENT - carIndex) / PERCENT_SCALE;
}

static s32 TargetSpeedAccelerationLimit(s32 targetSpeed) {
    s32 scaledSpeed = WrapSigned32(
        (int64_t)targetSpeed * TARGET_SPEED_SCALE) /
        TARGET_SPEED_SOURCE_SCALE;

    return WrapSigned32(
        (int64_t)scaledSpeed * ACCELERATION_LIMIT_PERCENT) / PERCENT_SCALE;
}

/*
 * Interpolate the course speed limit between the rival's current marker pair.
 * Cars outside that pair move their marker toward the current position and
 * get no new limit this frame. Rivals behind the front four share fourth
 * place's target, tapered by their grid slot.
 */
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex) {
    const TrackAiSpeedKey *lowKey;
    const TrackAiSpeedKey *highKey;
    const TrackAiSpeedKey *table;
    s32 position;
    s32 keyIndex;
    s32 lowProgress;
    s32 highProgress;
    s32 lowSpeed;
    s32 highSpeed;
    s32 pitch = 0;

    if (car == NULL || g_TrackEventData == NULL || carIndex < 0 ||
        carIndex >= RACE_CAR_SLOT_COUNT) {
        return;
    }

    position = car->trackProgress >> 4;
    keyIndex = car->speedKeyIndex;
    if (keyIndex < 0 || keyIndex >= TRACK_AI_SPEED_KEY_COUNT - 1) {
        /* Retail would read past the table here; the first pair stands in. */
        car->speedKeyIndex = 0;
        keyIndex = 0;
    } else if (position < AI_TABLE_LAP_START_PROGRESS) {
        /* Retail resets the marker for the next frame but still interpolates
         * this frame with the pair it read before the reset. Using the first
         * pair a frame early changes every rival's speed at the lap start. */
        car->speedKeyIndex = 0;
    }

    table = g_TrackEventData->aiSpeedKeys[g_RaceSeries != 0];
    lowKey = &table[keyIndex];
    highKey = &table[keyIndex + 1];
    lowProgress = lowKey->progress;
    highProgress = highKey->progress;
    lowSpeed = RivalTargetSpeed(lowKey, carIndex);
    highSpeed = RivalTargetSpeed(highKey, carIndex);

    if (position >= lowProgress && position <= highProgress) {
        s32 range = highProgress - lowProgress;
        s32 blended;

        pitch = lowKey->pitch;
        if (range <= 0) {
            range = 1;
        }
        blended = WrapSigned32(
            (int64_t)(highSpeed - lowSpeed) *
            (position - lowProgress));
        blended = WrapSigned32((int64_t)lowSpeed + blended / range);
        car->accelerationLimit = WrapSigned16(
            TargetSpeedAccelerationLimit(blended));
    } else {
        car->slideActive = 1;
        car->speedKeyIndex += highProgress < position ? 1 : -1;
        if (position < AI_TABLE_LAP_START_PROGRESS) {
            car->speedKeyIndex = 0;
        }
    }

    if (car->slideActive != 0) {
        UpdateCarSlideAngle(car, (s16)pitch);
    }
}
