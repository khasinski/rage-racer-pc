#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

enum {
    RACING_LINE_HINT_COUNT = 30,
    FRONT_RIVAL_COUNT = 4,
};

/*
 * Each racing-line hint describes a stretch of track and the lateral range
 * in which a rival may be nudged. Passing the current stretch advances to the
 * next entry and wraps at the list's -1 sentinel. Only the front four rivals
 * are nudged, and only while no nearby car is blocking them.
 */
void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex) {
    const s32 series = ReadStableRaceSeries() != 0;
    s32 position = car->trackProgress >> 4;
    const TrackRacingLineHint *hints;
    const TrackRacingLineHint *hint;

    if (g_TrackEventData == NULL || carIndex < 0 ||
        carIndex >= RACE_CAR_SLOT_COUNT) {
        return;
    }

    if (position < 0x20 || car->routeIndex < 0 ||
        car->routeIndex >= RACING_LINE_HINT_COUNT) {
        car->routeIndex = 0;
        position = 0;
    }
    hints = g_TrackEventData->racingLineHints[series];
    hint = &hints[car->routeIndex];

    if (hint->end < position) {
        car->routeIndex++;
        if (car->routeIndex >= RACING_LINE_HINT_COUNT ||
            hints[car->routeIndex].start == -1) {
            car->routeIndex = 0;
        }
        car->racingLineHintState = 0;
        return;
    }
    if (position < hint->start) {
        car->racingLineHintState = 0;
        return;
    }
    if (carIndex < FRONT_RIVAL_COUNT && car->nearbyCarCount == 0) {
        const s32 offset = car->aiLateralOffset;

        if (hint->minHeight < offset && offset < hint->maxHeight) {
            car->aiLateralOffset = offset + hint->heightAdjustment;
        }
    }
}
