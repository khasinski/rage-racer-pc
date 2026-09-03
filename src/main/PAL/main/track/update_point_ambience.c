#include "game/audio.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

#include <stdint.h>

enum {
    POINT_AMBIENCE_MAX_LEVEL = 0x30,
    POINT_AMBIENCE_VOLUME_BIAS = 0x20,
    POINT_AMBIENCE_CUE_ONE_OUTPUT = 2,
    POINT_AMBIENCE_OTHER_CUE_OUTPUT = 3,
};

static const TrackPointAmbienceZone *FindPointAmbienceZone(
    s32 trackPosition) {
    const TrackPointAmbienceZone *zones;
    s32 index;

    if (g_TrackEventData == NULL) return NULL;
    zones = g_TrackEventData->pointAmbienceZones;

    for (index = 0; index < TRACK_POINT_AMBIENCE_ZONE_COUNT; index++) {
        if (zones[index].start == -1) {
            break;
        }
        if (trackPosition >= zones[index].start &&
            trackPosition <= zones[index].end) {
            return &zones[index];
        }
    }
    return NULL;
}

static s32 PointAmbienceLevel(const TrackPointAmbienceZone *zone,
                              s32 trackPosition) {
    const s32 fadeIn = (s16)zone->fadeInDistance;
    const s32 fadeOut = (s16)zone->fadeOutDistance;

    if (fadeIn > 0 &&
        (int64_t)trackPosition < (int64_t)zone->start + fadeIn) {
        return (s32)(((int64_t)trackPosition - zone->start) *
                     POINT_AMBIENCE_MAX_LEVEL / fadeIn);
    }
    if (fadeOut > 0 &&
        (int64_t)trackPosition > (int64_t)zone->end - fadeOut) {
        return (s32)(((int64_t)zone->end - trackPosition) *
                     POINT_AMBIENCE_MAX_LEVEL / fadeOut);
    }
    return POINT_AMBIENCE_MAX_LEVEL;
}

static s32 PointAmbienceOutputCue(s32 authoredCue) {
    return authoredCue == 1 ? POINT_AMBIENCE_CUE_ONE_OUTPUT
                            : POINT_AMBIENCE_OTHER_CUE_OUTPUT;
}

static s32 PointAmbienceAttenuation(s32 level, int64_t dx, int64_t dz) {
    const uint64_t x = dx < 0 ? (uint64_t)-dx : (uint64_t)dx;
    const uint64_t z = dz < 0 ? (uint64_t)-dz : (uint64_t)dz;
    const uint64_t squared = x * x + z * z;
    const uint64_t panRadius = (uint64_t)level * 64;
    s32 distance;

    if (squared >= panRadius * panRadius) return 0;
    distance = (s32)(SquareRoot12((long)(squared / 4)) >> 11);
    return distance < level ? level - distance : 0;
}

/*
 * Place the one point-source ambience the camera is near: pick the zone the
 * car has reached, work out how loud it should be from how far into the zone
 * it is, then pan that between the two channels by where the source sits
 * relative to where the camera is looking.
 */
void UpdatePointAmbience(s32 trackPosition) {
    const TrackPointAmbienceZone *zone;
    s32 level;
    s32 authoredCue;
    s32 leftVolume;
    s32 rightVolume;
    s32 outputLeft;
    s32 outputRight;

    trackPosition = TrackPositionForSeries(trackPosition, g_TrackLength,
                                           g_RaceSeries);

    leftVolume = 0;
    rightVolume = 0;
    zone = FindPointAmbienceZone(trackPosition);
    level = zone != NULL ? PointAmbienceLevel(zone, trackPosition) : 0;
    authoredCue = zone != NULL ? zone->cue : 0;

    if (level != 0 && zone != NULL) {
        const int64_t sourceX =
            (int64_t)zone->sourceX - g_RenderState.viewX;
        const int64_t sourceZ =
            (int64_t)zone->sourceZ - g_RenderState.viewZ;
        const s32 attenuated =
            PointAmbienceAttenuation(level, sourceX, sourceZ);
        s32 sine = 0;

        if (attenuated != 0) {
            const s32 bearing = Atan2((s32)sourceX, (s32)sourceZ);
            const s32 pan = (s32)(
                ((u32)g_RenderState.viewAngleY - 0xC00u + (u32)bearing) &
                0xFFFu);
            sine = rsin(pan);
        }
        leftVolume = level + (attenuated * sine) / 4096 +
                     POINT_AMBIENCE_VOLUME_BIAS;
        rightVolume = level + (-attenuated * sine) / 4096 +
                      POINT_AMBIENCE_VOLUME_BIAS;
        if (authoredCue == -1) {
            authoredCue = 1;
        }
    }

    outputLeft = g_MirrorMode != 0 ? leftVolume : rightVolume;
    outputRight = g_MirrorMode != 0 ? rightVolume : leftVolume;
    SetStereoSoundCue(PointAmbienceOutputCue(authoredCue), (s16)outputLeft,
                      (s16)outputRight);
}
