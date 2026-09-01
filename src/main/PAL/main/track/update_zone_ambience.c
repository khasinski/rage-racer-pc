#include "game/audio.h"
#include "game/race.h"
#include "game/track.h"

enum {
    AMBIENCE_ZONE_COUNT = 4,
    AMBIENCE_MAX_VOLUME = 0x60,
    AMBIENCE_FADE_DISTANCE = 800,
};

static s32 AmbienceVolumeAtPosition(s32 position, s32 maximumVolume) {
    const TrackAmbienceZone *zone = g_TrackEventData->ambienceZones;
    s32 index;

    for (index = 0; index < AMBIENCE_ZONE_COUNT && zone->start != -1;
         index++, zone++) {
        if (position < zone->start || position > zone->end) {
            continue;
        }
        if (position < zone->start + AMBIENCE_FADE_DISTANCE &&
            (zone->flags & 1) != 0) {
            return maximumVolume * (position - zone->start) /
                   AMBIENCE_FADE_DISTANCE;
        }
        if (position > zone->end - AMBIENCE_FADE_DISTANCE &&
            (zone->flags & 2) != 0) {
            return maximumVolume * (zone->end - position) /
                   AMBIENCE_FADE_DISTANCE;
        }
        return maximumVolume;
    }
    return 0;
}

void UpdateZoneAmbience(s32 trackPosition) {
    s32 sceneryTier = g_GrandPrixClass % 5;
    s32 maximumVolume = sceneryTier >= 1 ? AMBIENCE_MAX_VOLUME : 0;
    s32 cue = sceneryTier >= 3 ? 1 : 0;
    s32 volume;

    if (g_RaceSeries != 0) {
        trackPosition = g_TrackLength - trackPosition;
    }
    volume = (s16)AmbienceVolumeAtPosition(trackPosition, maximumVolume);
    SetStereoSoundCue(cue, volume, volume);
}
