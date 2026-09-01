#include "game/audio.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track_internal.h"

/* The looping prop's live orientation: three 12-bit angles copied wholesale out
 * of the current rotation keyframe by InitPathScenery, which sees the same
 * eight bytes as one Blk8. */
void DrawPathScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    s32 drawId;
    s32 frameValue;

    BuildRotMatrixY(&mtx0, 0x800 - g_PathSceneryTransform.rotation.vy);
    BuildRotMatrixX(&mtx1, g_PathSceneryTransform.rotation.vx);
    MulMatrix2(&mtx0, &mtx1);
    MulMatrix2((&g_RenderState.matrix), &mtx1);
    BuildRotMatrixZ(&mtx0, g_PathSceneryTransform.rotation.vz);
    MulMatrix2(&mtx1, &mtx0);

    SelectModelBank(1);
    SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(g_PathSceneryTransform.position.w), &mtx0);
    frameValue = g_ModelBankCount;
    g_RenderState.envMode4 = 0;
    drawId = 1;
    if (frameValue >= 0x24) {
        drawId = 0x23;
    }
    SubmitModel((&g_RenderState), drawId);

    {
        s32 angle = (s32)((u32)g_SceneTimer * 331u);

        BuildRotMatrixY(&mtx1, angle & 0xFFF);
    }
    MulMatrix2(&mtx0, &mtx1);
    SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(g_PathSceneryTransform.position.w), &mtx1);
    frameValue = g_ModelBankCount;
    g_ScratchRenderMode = 0;
    drawId = 1;
    if (frameValue >= 0x25) {
        drawId = 0x24;
    }
    SubmitModel((&g_RenderState), drawId);
}


void UpdateTrackEventSound(s16 arg) {
    TrackEventSoundZone *cur;
    TrackEventSoundZone *end;
    s32 data;
    s16 lo;
    s32 s0, s1, s2, s3;
    s32 val;
    s32 t;
    s32 a0v, a1v;

    data = 0;
    end = g_TrackEventData->eventSoundZones + 30;
    for (cur = g_TrackEventData->eventSoundZones; cur < end; cur++) {
        lo = cur->start;
        if (arg >= lo) {
        if (cur->end >= arg) {
            data = cur->flags;
            break;
        }
        }
        if (lo == -1) {
            break;
        }
    }

    if (data != 0) {
    /* Decay the stored lean toward zero by one step, never past it. */
    s0 = g_PlayerField3C;
    if (s0 < 0) {
        s0 += 0x100;
        if (s0 > 0) {
            s0 = 0;
        }
    } else {
        s0 -= 0x100;
        if (s0 < 0) {
            s0 = 0;
        }
    }
    if (s0 != 0) {
        s0 = (s0 * g_PlayerCar.speed) / 12775;
        t = g_RenderState.viewAngleY - 0xC00;
        s3 = (t + TrackPoint(g_PlayerCar.trackPointIndex)->angle) & 0xFFF;
        if (s0 < 0 && (data & 2) > 0) {
            val = s0 * rcos(s3);
            if (val < 0) {
                val += 0xFFF;
            }
            s1 = -(s0 + (val >> 12));
            val = (-s0) * rcos(s3);
            if (val < 0) {
                val += 0xFFF;
            }
            s2 = -(s0 + (val >> 12));
        } else if (s0 > 0 && (data & 1) > 0) {
            val = s0 * rcos(s3);
            if (val < 0) {
                val += 0xFFF;
            }
            s2 = s0 + (val >> 12);
            val = (-s0) * rcos(s3);
            if (val < 0) {
                val += 0xFFF;
            }
            s1 = s0 + (val >> 12);
        } else {
            s2 = 0;
            s1 = 0;
        }
    } else {
        s2 = 0;
        s1 = 0;
    }
    if (g_MirrorMode) {
        a0v = s2;
        a1v = s1;
    } else {
        a0v = s1;
        a1v = s2;
    }
    } else {
    a0v = 0;
    a1v = 0;
    }
    SetPanVoiceTargetVolume(a0v, a1v);
}


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
    s16 attenuated; /* 16-bit on purpose: see below */
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
         * Quieter the further the camera stands from the source. Retail keeps
         * this in 16 bits, so a source far enough away wraps the subtraction
         * back to a large positive number; the clamp against the zone's own
         * level is what catches that, and both have to stay as they are.
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
        /* Where the source lies relative to the way the camera faces, turned
         * into a left/right split around the zone's own level. */
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
