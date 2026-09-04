#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/track.h"

enum {
    AI_TABLE_LAP_START_PROGRESS = 0x20,
};

/*
 * Each racing-line hint describes a stretch of track and the lateral range
 * in which a rival may be nudged. Passing the current stretch advances to the
 * next entry and wraps at the list's -1 sentinel. Only the front four rivals
 * are nudged, and only while no nearby car is blocking them.
 */
void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex) {
    const s32 series = g_RaceSeries != 0;
    s32 position;
    const TrackRacingLineHint *hints;
    const TrackRacingLineHint *hint;

    if (car == NULL || g_TrackEventData == NULL || carIndex < 0 ||
        carIndex >= RACE_CAR_SLOT_COUNT) {
        return;
    }

    position = car->trackProgress >> 4;
    hints = g_TrackEventData->racingLineHints[series];
    if (car->racingLineHintIndex < 0 ||
        car->racingLineHintIndex >= TRACK_RACING_LINE_HINT_COUNT) {
        /* Retail would read outside the table; the first hint stands in. */
        car->racingLineHintIndex = 0;
    }
    /* Retail picks this frame's hint before the lap-start reset, so the
     * reset only takes effect from the next frame. Reading hint zero a
     * frame early nudged every rival's steering at the line. */
    hint = &hints[car->racingLineHintIndex];
    if (position < AI_TABLE_LAP_START_PROGRESS) {
        car->racingLineHintIndex = 0;
        position = 0;
    }

    if (hint->end < position) {
        car->racingLineHintIndex++;
        if (car->racingLineHintIndex >= TRACK_RACING_LINE_HINT_COUNT ||
            hints[car->racingLineHintIndex].start == -1) {
            car->racingLineHintIndex = 0;
        }
        return;
    }
    if (position < hint->start) {
        return;
    }
    if (carIndex < RIVAL_CONTENDER_COUNT && car->nearbyCarCount == 0) {
        const s32 offset = car->aiLateralOffset;

        if (hint->minHeight < offset && offset < hint->maxHeight) {
            car->aiLateralOffset = WrapSigned16(
                offset + hint->heightAdjustment);
        }
    }
}
