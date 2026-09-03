#include "game/audio.h"
#include "game/race.h"
#include "game/track.h"

enum {
    AMBIENCE_MAX_VOLUME = 0x60,
    AMBIENCE_FADE_DISTANCE = 800,
    AMBIENCE_FADE_IN = 1,
    AMBIENCE_FADE_OUT = 2,
    AMBIENCE_MINIMUM_CLASS_TIER = 1,
    AMBIENCE_ALTERNATE_CUE_CLASS_TIER = 3,
};

static s32 AmbienceVolumeAtPosition(s32 position, s32 maximumVolume) {
    const TrackAmbienceZone *zone;
    s32 index;

    if (g_TrackEventData == NULL) return 0;
    zone = g_TrackEventData->ambienceZones;

    for (index = 0;
         index < TRACK_AMBIENCE_ZONE_COUNT && zone->start != -1;
         index++, zone++) {
        if (position < zone->start || position > zone->end) {
            continue;
        }
        if ((int64_t)position <
                (int64_t)zone->start + AMBIENCE_FADE_DISTANCE &&
            (zone->flags & AMBIENCE_FADE_IN) != 0) {
            return (s32)((int64_t)maximumVolume *
                         ((int64_t)position - zone->start) /
                         AMBIENCE_FADE_DISTANCE);
        }
        if ((int64_t)position >
                (int64_t)zone->end - AMBIENCE_FADE_DISTANCE &&
            (zone->flags & AMBIENCE_FADE_OUT) != 0) {
            return (s32)((int64_t)maximumVolume *
                         ((int64_t)zone->end - position) /
                         AMBIENCE_FADE_DISTANCE);
        }
        return maximumVolume;
    }
    return 0;
}

void UpdateZoneAmbience(s32 trackPosition) {
    s32 sceneryTier = g_GrandPrixClass % GRAND_PRIX_FINAL_CLASS_INDEX;
    s32 maximumVolume = sceneryTier >= AMBIENCE_MINIMUM_CLASS_TIER
                            ? AMBIENCE_MAX_VOLUME
                            : 0;
    s32 cue = sceneryTier >= AMBIENCE_ALTERNATE_CUE_CLASS_TIER ? 1 : 0;
    s32 volume;

    trackPosition = TrackPositionForSeries(trackPosition, g_TrackLength,
                                           g_RaceSeries);
    volume = AmbienceVolumeAtPosition(trackPosition, maximumVolume);
    SetStereoSoundCue(cue, volume, volume);
}
