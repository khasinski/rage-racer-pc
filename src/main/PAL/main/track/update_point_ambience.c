#include "game/audio.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

/*
 * Place the one point-source ambience the camera is near: pick the zone the
 * car has reached, work out how loud it should be from how far into the zone
 * it is, then pan that between the two channels by where the source sits
 * relative to where the camera is looking.
 */
void UpdatePointAmbience(s32 trackPosition) {
    TrackEventData *events;
    TrackPointAmbienceZone *zone;
    s32 level;
    s32 sourceX;
    s32 sourceZ;
    s32 cue;
    s32 leftVolume;
    s32 rightVolume;
    s32 index;
    s32 fade;
    s16 attenuated;
    s32 bearing;
    s32 pan;
    s32 sine;

    events = g_TrackEventData;
    zone = events->pointAmbienceZones;
    if (g_RaceSeries != 0) {
        trackPosition = g_TrackLength - trackPosition;
    }

    leftVolume = 0;
    rightVolume = 0;
    level = 0;
    sourceX = 0;
    sourceZ = 0;
    cue = 0;

    /* Two zones at most, and a start of -1 ends the list. */
    for (index = 0; index < 2; index++, zone++) {
        if (zone->start == -1) {
            break;
        }
        if (trackPosition < zone->start || trackPosition > zone->end) {
            continue;
        }
        /* Full level in the middle, ramped over the two fade distances. */
        fade = (s16)zone->fadeInDistance;
        if (trackPosition < zone->start + fade) {
            level = ((trackPosition - zone->start) * 48) / fade;
        } else {
            fade = (s16)zone->fadeOutDistance;
            if (zone->end - fade < trackPosition) {
                level = ((zone->end - trackPosition) * 48) / fade;
            } else {
                level = 0x30;
            }
        }
        sourceX = zone->sourceX;
        sourceZ = zone->sourceZ;
        cue = zone->cue;
        break;
    }

    if ((s16)level != 0) {
        /*
         * Retail keeps attenuation in 16 bits. A sufficiently distant source
         * can wrap the subtraction positive; the following clamp catches it.
         */
        sourceX -= g_RenderState.viewX;
        sourceZ -= g_RenderState.viewZ;
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
        leftVolume = level + (attenuated * sine) / 4096 + 0x20;
        rightVolume = level + (-attenuated * sine) / 4096 + 0x20;
        if (cue < 0) {
            cue = -cue;
        }
    }

    if (g_MirrorMode != 0) {
        SetStereoSoundCue(cue == 1 ? 2 : 3, (s16)leftVolume, (s16)rightVolume);
    } else {
        SetStereoSoundCue(cue == 1 ? 2 : 3, (s16)rightVolume, (s16)leftVolume);
    }
}
