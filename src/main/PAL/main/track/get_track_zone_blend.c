#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

enum {
    TRACK_ZONE_COUNT = 20,
    TRACK_ZONE_FADE_DISTANCE = 0x100,
};

static s32 ZoneBlend(const TrackZone *zone, s32 position, s32 *zonePhase) {
    if (position < zone->start + TRACK_ZONE_FADE_DISTANCE) {
        *zonePhase = 1;
        return position - zone->start;
    }
    if (position > zone->end - TRACK_ZONE_FADE_DISTANCE) {
        *zonePhase = 2;
        return zone->end - position;
    }
    *zonePhase = 3;
    return TRACK_ZONE_FADE_DISTANCE;
}

s32 GetTrackZoneBlend(s32 position) {
    const TrackZone *zone = g_TrackEventData->zones;
    s32 index;

    if (g_RaceSeries != 0) {
        position = g_TrackLength - position;
    }

    g_TrackZoneCode = 0;
    g_ReverbZoneDepth = 0;
    g_TrackZoneDark = 0;

    for (index = 0; index < TRACK_ZONE_COUNT && zone[index].start != -1;
         index++) {
        const TrackZone *current = &zone[index];
        s32 phase;
        s32 blend;
        s32 code;

        if (position <= current->start || position >= current->end) {
            continue;
        }

        blend = ZoneBlend(current, position, &phase);
        code = current->code;
        g_TrackZoneCode = (s16)code;
        g_ReverbZoneDepth = current->value;

        if (code == 0) {
            g_TrackZoneDark = 3;
        } else if (code == 2 || code == -3) {
            g_TrackZoneCode = 1;
            if (code == 2) {
                return 0;
            }
            if (phase == 2) {
                return TRACK_ZONE_FADE_DISTANCE;
            }
        } else if (code < 0) {
            g_TrackZoneCode = (s16)-code;
            return TRACK_ZONE_FADE_DISTANCE;
        }
        return blend;
    }
    return 0;
}
