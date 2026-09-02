#include "game/audio.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

enum {
    POINT_AMBIENCE_ZONE_COUNT = 2,
    POINT_AMBIENCE_MAX_LEVEL = 0x30,
    POINT_AMBIENCE_VOLUME_BIAS = 0x20,
    POINT_AMBIENCE_CUE_ONE_OUTPUT = 2,
    POINT_AMBIENCE_OTHER_CUE_OUTPUT = 3,
};

static const TrackPointAmbienceZone *FindPointAmbienceZone(
    s32 trackPosition) {
    const TrackPointAmbienceZone *zones =
        g_TrackEventData->pointAmbienceZones;
    s32 index;

    for (index = 0; index < POINT_AMBIENCE_ZONE_COUNT; index++) {
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

    if (fadeIn > 0 && trackPosition < zone->start + fadeIn) {
        return (trackPosition - zone->start) * POINT_AMBIENCE_MAX_LEVEL /
               fadeIn;
    }
    if (fadeOut > 0 && trackPosition > zone->end - fadeOut) {
        return (zone->end - trackPosition) * POINT_AMBIENCE_MAX_LEVEL /
               fadeOut;
    }
    return POINT_AMBIENCE_MAX_LEVEL;
}

static s32 PointAmbienceOutputCue(s32 authoredCue) {
    return authoredCue == 1 ? POINT_AMBIENCE_CUE_ONE_OUTPUT
                            : POINT_AMBIENCE_OTHER_CUE_OUTPUT;
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

    if (g_RaceSeries != 0) {
        trackPosition = g_TrackLength - trackPosition;
    }

    leftVolume = 0;
    rightVolume = 0;
    zone = FindPointAmbienceZone(trackPosition);
    level = zone != NULL ? PointAmbienceLevel(zone, trackPosition) : 0;
    authoredCue = zone != NULL ? zone->cue : 0;

    if ((s16)level != 0 && zone != NULL) {
        s32 sourceX = zone->sourceX - g_RenderState.viewX;
        s32 sourceZ = zone->sourceZ - g_RenderState.viewZ;
        s16 attenuated;
        s32 bearing;
        s32 pan;
        s32 sine;

        /*
         * Retail keeps attenuation in 16 bits. A sufficiently distant source
         * can wrap the subtraction positive; the following clamp catches it.
         */
        attenuated = (s16)(level - (SquareRoot12((sourceX * sourceX) / 4 +
                                                 (sourceZ * sourceZ) / 4) >> 11));
        if ((s16)level < attenuated) {
            attenuated = (s16)level;
        }
        if (attenuated < 0) {
            attenuated = 0;
        }
        bearing = Atan2(sourceX, sourceZ);
        pan = (g_RenderState.viewAngleY - 0xC00 + bearing) & 0xFFF;
        sine = rsin(pan);
        leftVolume = level + (attenuated * sine) / 4096 +
                     POINT_AMBIENCE_VOLUME_BIAS;
        rightVolume = level + (-attenuated * sine) / 4096 +
                      POINT_AMBIENCE_VOLUME_BIAS;
        if (authoredCue < 0) {
            authoredCue = -authoredCue;
        }
    }

    outputLeft = g_MirrorMode != 0 ? leftVolume : rightVolume;
    outputRight = g_MirrorMode != 0 ? rightVolume : leftVolume;
    SetStereoSoundCue(PointAmbienceOutputCue(authoredCue), (s16)outputLeft,
                      (s16)outputRight);
}
