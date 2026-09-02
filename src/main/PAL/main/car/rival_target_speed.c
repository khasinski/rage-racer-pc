#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

enum {
    FRONT_RIVAL_COUNT = 4,
};

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
    s32 marker;
    s32 lowProgress;
    s32 highProgress;
    s32 lowSpeed;
    s32 highSpeed;
    s32 pitch = 0;

    if (g_TrackEventData == NULL || carIndex < 0 ||
        carIndex >= RACE_CAR_SLOT_COUNT) {
        return;
    }

    position = car->trackProgress >> 4;
    marker = car->routeMarkerIndex;
    if (position < 0x20 || marker < 0 ||
        marker >= TRACK_AI_SPEED_KEY_COUNT - 1) {
        car->routeMarkerIndex = 0;
        marker = 0;
    }

    table = g_TrackEventData->aiSpeedKeys[ReadStableRaceSeries() != 0];
    lowKey = &table[marker];
    highKey = &table[marker + 1];
    lowProgress = lowKey->progress;
    highProgress = highKey->progress;
    if (carIndex < FRONT_RIVAL_COUNT) {
        lowSpeed = lowKey->slotTargetSpeeds[carIndex];
        highSpeed = highKey->slotTargetSpeeds[carIndex];
    } else {
        const s32 taper = 0x55 - carIndex;

        lowSpeed = (lowKey->slotTargetSpeeds[3] * taper) / 100;
        highSpeed = (highKey->slotTargetSpeeds[3] * taper) / 100;
    }

    if (position >= lowProgress && position <= highProgress) {
        s32 range = highProgress - lowProgress;
        s32 blended;

        pitch = lowKey->pitch;
        if (range <= 0) {
            range = 1;
        }
        blended = lowSpeed +
                  ((highSpeed - lowSpeed) * (position - lowProgress)) / range;
        car->accelerationLimit = (((blended * 1168) / 160) * 6) / 100;
    } else {
        car->routeMarkerActive = 1;
        car->routeMarkerIndex += highProgress < position ? 1 : -1;
        if (position < 0x20) {
            car->routeMarkerIndex = 0;
        }
    }

    if (car->routeMarkerActive != 0) {
        UpdateCarSlideAngle(car, (s16)pitch);
    }
}
