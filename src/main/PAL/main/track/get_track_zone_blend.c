#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

#include <limits.h>

enum {
    TRACK_ZONE_FADE_DISTANCE = 0x100,
    TRACK_ZONE_PHASE_FADE_IN = 1,
    TRACK_ZONE_PHASE_FADE_OUT = 2,
    TRACK_ZONE_PHASE_FULL = 3,
    TRACK_ZONE_CODE_DARK_ONLY = 0,
    TRACK_ZONE_CODE_NO_BLEND = 2,
    TRACK_ZONE_CODE_EXIT_ONLY_BLEND = -3,
    TRACK_ZONE_DARK_LEVEL = 3,
};

static s32 ZoneBlend(const TrackZone *zone, s32 position, s32 *zonePhase) {
    int64_t distanceFromStart = (int64_t)position - zone->start;
    int64_t distanceToEnd = (int64_t)zone->end - position;

    if (distanceFromStart < TRACK_ZONE_FADE_DISTANCE) {
        *zonePhase = TRACK_ZONE_PHASE_FADE_IN;
        return (s32)distanceFromStart;
    }
    if (distanceToEnd < TRACK_ZONE_FADE_DISTANCE) {
        *zonePhase = TRACK_ZONE_PHASE_FADE_OUT;
        return (s32)distanceToEnd;
    }
    *zonePhase = TRACK_ZONE_PHASE_FULL;
    return TRACK_ZONE_FADE_DISTANCE;
}

s32 GetTrackZoneBlend(s32 position) {
    const TrackZone *zones;
    s32 index;

    g_TrackZoneCode = 0;
    g_ReverbZoneDepth = 0;
    g_TrackZoneDark = 0;
    if (g_TrackEventData == NULL) {
        return 0;
    }

    position = TrackPositionForSeries(position, g_TrackLength, g_RaceSeries);
    zones = g_TrackEventData->zones;

    for (index = 0; index < TRACK_ZONE_COUNT && zones[index].start != -1;
         index++) {
        const TrackZone *current = &zones[index];
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

        if (code == TRACK_ZONE_CODE_DARK_ONLY) {
            g_TrackZoneDark = TRACK_ZONE_DARK_LEVEL;
        } else if (code == TRACK_ZONE_CODE_NO_BLEND ||
                   code == TRACK_ZONE_CODE_EXIT_ONLY_BLEND) {
            g_TrackZoneCode = 1;
            if (code == TRACK_ZONE_CODE_NO_BLEND) {
                return 0;
            }
            if (phase == TRACK_ZONE_PHASE_FADE_OUT) {
                return TRACK_ZONE_FADE_DISTANCE;
            }
        } else if (code < 0) {
            s32 positiveCode = -(s32)code;

            g_TrackZoneCode = (s16)(positiveCode > INT16_MAX
                                        ? INT16_MAX
                                        : positiveCode);
            return TRACK_ZONE_FADE_DISTANCE;
        }
        return blend;
    }
    return 0;
}
