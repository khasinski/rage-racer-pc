#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

enum { TRACK_EVENT_SOUND_ZONE_COUNT = 30 };

static s32 FindEventSoundFlags(s16 trackSection) {
    const TrackEventSoundZone *zone = g_TrackEventData->eventSoundZones;
    const TrackEventSoundZone *end = zone + TRACK_EVENT_SOUND_ZONE_COUNT;

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

    lean = DecayTowardZero(g_PlayerField3C, 0x100);
    if (lean == 0) {
        return;
    }

    lean = (lean * g_PlayerCar.speed) / 12775;
    angle = (g_RenderState.viewAngleY - 0xC00 +
             TrackPoint(g_PlayerCar.trackPointIndex)->angle) & 0xFFF;

    if (lean < 0 && (flags & 2) != 0) {
        *left = -(lean + MultiplyCosine(lean, angle));
        *right = -(lean + MultiplyCosine(-lean, angle));
    } else if (lean > 0 && (flags & 1) != 0) {
        *right = lean + MultiplyCosine(lean, angle);
        *left = lean + MultiplyCosine(-lean, angle);
    }
}

void UpdateTrackEventSound(s16 trackSection) {
    s32 left;
    s32 right;

    CalculateEventSoundVolumes(FindEventSoundFlags(trackSection), &left, &right);
    if (g_MirrorMode) {
        SetPanVoiceTargetVolume(right, left);
    } else {
        SetPanVoiceTargetVolume(left, right);
    }
}
