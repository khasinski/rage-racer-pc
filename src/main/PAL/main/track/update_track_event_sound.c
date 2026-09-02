#include "game/audio.h"
#include "game/angle.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

enum {
    EVENT_SOUND_RIGHT_SIDE = 1,
    EVENT_SOUND_LEFT_SIDE = 2,
    LATERAL_LEAN_DEAD_ZONE = 0x100,
    EVENT_SOUND_SPEED_SCALE = 12775,
};

static s32 FindEventSoundFlags(s16 trackSection) {
    const TrackEventSoundZone *zone;
    const TrackEventSoundZone *end;

    if (g_TrackEventData == NULL) {
        return 0;
    }
    zone = g_TrackEventData->eventSoundZones;
    end = zone + TRACK_EVENT_SOUND_ZONE_COUNT;

    for (; zone < end && zone->start != -1; zone++) {
        if (trackSection >= zone->start && trackSection <= zone->end) {
            return zone->flags;
        }
    }
    return 0;
}

static s32 DecayTowardZero(s32 value, s32 step) {
    if (value < 0) {
        value += step;
        return value > 0 ? 0 : value;
    }
    value -= step;
    return value < 0 ? 0 : value;
}

/* The retail MIPS sequence rounds signed fixed-point products toward zero. */
static s32 MultiplyCosine(s32 value, s32 angle) {
    s32 product = value * rcos(angle);

    if (product < 0) {
        product += 0xFFF;
    }
    return product >> 12;
}

static void CalculateEventSoundVolumes(s32 flags, s32 *left, s32 *right) {
    s32 lean;
    s32 angle;

    *left = 0;
    *right = 0;
    if (flags == 0) {
        return;
    }

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return;
    }

    lean = DecayTowardZero(g_PlayerCar.normalizedLateralOffset,
                           LATERAL_LEAN_DEAD_ZONE);
    if (lean == 0) {
        return;
    }

    lean = (lean * g_PlayerCar.speed) / EVENT_SOUND_SPEED_SCALE;
    angle = (g_RenderState.viewAngleY - ANGLE_THREE_QUARTER_TURN +
             TrackPoint(g_PlayerCar.trackPointIndex)->angle) & ANGLE_MASK;

    if (lean < 0 && (flags & EVENT_SOUND_LEFT_SIDE) != 0) {
        *left = -(lean + MultiplyCosine(lean, angle));
        *right = -(lean + MultiplyCosine(-lean, angle));
    } else if (lean > 0 && (flags & EVENT_SOUND_RIGHT_SIDE) != 0) {
        *right = lean + MultiplyCosine(lean, angle);
        *left = lean + MultiplyCosine(-lean, angle);
    }
}

void UpdateTrackEventSound(s16 trackSection) {
    s32 left;
    s32 right;
    s32 outputLeft;
    s32 outputRight;

    CalculateEventSoundVolumes(FindEventSoundFlags(trackSection), &left, &right);
    outputLeft = g_MirrorMode != 0 ? right : left;
    outputRight = g_MirrorMode != 0 ? left : right;
    SetPanVoiceTargetVolume(outputLeft, outputRight);
}
