#include "common.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "psyq/gte.h"

/* The looping prop's live orientation: three 12-bit angles copied wholesale out
 * of the current rotation keyframe by InitPathScenery, which sees the same
 * eight bytes as one Blk8. */
void DrawPathScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    s32 drawId;
    s32 frameValue;
    Matrix *mtx1Ptr;
    mtx1Ptr = &mtx1;

    BuildRotMatrixY(&mtx0, 0x800 - g_PathSceneryTransform.rotation.vy);
    BuildRotMatrixX(mtx1Ptr, g_PathSceneryTransform.rotation.vx);
    MulMatrix2(&mtx0, mtx1Ptr);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, mtx1Ptr);
    BuildRotMatrixZ(&mtx0, g_PathSceneryTransform.rotation.vz);
    MulMatrix2(mtx1Ptr, &mtx0);

    SelectModelBank(1);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &g_PathSceneryTransform.position, &mtx0);
    frameValue = g_ModelBankCount;
    SCRATCH_ENV_MODE4 = 0;
    drawId = 1;
    if (frameValue >= 0x24) {
        drawId = 0x23;
    }
    SubmitModel(SCRATCHPAD, drawId);

    {
        s32 base;
        s32 acc;
        s32 tmp;

        base = g_SceneTimer;
        acc = base * 4;
        acc += base;
        tmp = acc * 32;
        acc += tmp;
        acc <<= 1;
        acc += base;
        BuildRotMatrixY(mtx1Ptr, acc & 0xFFF);
    }
    MulMatrix2(&mtx0, mtx1Ptr);
    SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &g_PathSceneryTransform.position, mtx1Ptr);
    frameValue = g_ModelBankCount;
    g_ScratchRenderMode = 0;
    drawId = 1;
    if (frameValue >= 0x25) {
        drawId = 0x24;
    }
    SubmitModel(SCRATCHPAD, drawId);
}


void UpdateTrackEventSound(s16 arg) {
    TrackEventSoundZone *p;
    TrackEventSoundZone *cur;
    TrackEventSoundZone *end;
    s32 data;
    s16 lo;
    s32 s0, s1, s2, s3;
    s32 val;
    s32 t;
    s32 a0v, a1v;
    TrackEventSoundZoneAddress cursorAddress;
    TrackEventSoundZoneAddress endAddress;

    data = 0;
    p = g_TrackEventData->eventSoundZones;
    end = g_TrackEventData->eventSoundZones + 30;
    cur = p;
    do {
        lo = cur->start;
        if (arg < lo) {
        } else {
        if (cur->end >= arg) {
            data = p->flags;
            break;
        }
        }
        if (lo == -1) {
            break;
        }
        p = cur + 1;
        cur = p;
        cursorAddress.pointer = p;
        endAddress.pointer = end;
    } while (cursorAddress.value < endAddress.value);

    if (!(data == 0)) {
    s0 = g_PlayerField3C;
    if (!(s0 >= 0)) {
    s0 += 0x100;
    if (s0 <= 0) {
        goto track_event_motion_done;
    }
    s0 = 0;
    } else {
    s0 -= 0x100;
    if (s0 >= 0) {
    } else {
    s0 = 0;
    }
    }
track_event_motion_done:
    if (s0 != 0) {
        s0 = (s0 * g_PlayerCar.speed) / 12775;
        t = SCRATCH_VIEW_ANGLE_Y - 0xC00;
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


void UpdatePointAmbience(s32 arg) {
    TrackEventData *base;
    TrackPointAmbienceZone *startp;
    TrackPointAmbienceZone *seg;
    s32 v1;
    s32 t0;
    u16 a1raw;
    u16 t1raw;
    s32 sentinel;
    s32 s0v;
    s16 s1;
    s32 s2;
    s32 s3;
    s32 s4;
    s32 s5;
    s32 s6;
    s32 a2v;
    s32 a1s;
    s32 v0;
    s32 angle, sinv;

    base = g_TrackEventData;
    startp = base->pointAmbienceZones;
    if (g_RaceSeries != 0) {
        arg = g_TrackLength - arg;
    }

    s5 = 0;
    a2v = 0;
    s2 = 0;
    s3 = 0;
    s4 = 0;
    s6 = 0;
    s0v = 0;
    sentinel = -1;
    seg = startp;
loop:
    v1 = seg->start;
    t0 = seg->end;
    if (!(v1 == sentinel)) {
    a1raw = seg->fadeInDistance;
    t1raw = seg->fadeOutDistance;
    if (!(arg < v1)) {
    if (!(t0 < arg)) {
    v0 = a1raw << 16;
    a1s = v0 >> 16;
    if (arg < v1 + a1s) {
        v0 = arg - v1;
        v1 = (v0 * 48) / a1s;
        s2 = v1;
    } else {
        v0 = t1raw << 16;
        a1s = v0 >> 16;
        if ((t0 - a1s) < arg) {
            v0 = t0 - arg;
            v1 = (v0 * 48) / a1s;
            s2 = v1;
        } else {
            s2 = 0x30;
        }
    }
    s3 = seg->leftVolume;
    s4 = seg->rightVolume;
    s6 = seg->phase;
    goto track_segment_found;
    }
    }
    s0v++;
    seg++;
    if (s0v < 2) {
        goto loop;
    }

    }
track_segment_found:
    v0 = s2 << 16;
    s0v = v0 >> 16;
    if (s0v != 0) {
        s3 -= SCRATCH_VIEW_X;
        s4 -= SCRATCH_VIEW_Z;
        v0 = SquareRoot12((s3 * s3) / 4 + (s4 * s4) / 4);
        v0 = s2 - (v0 >> 11);
        s1 = v0;
        v0 = s0v < (s16)v0;
        if (v0) {
            s1 = s2;
        }
        if ((s16)s1 < 0) {
            s1 = 0;
        }
        angle = Atan2(s3, s4);
        v1 = SCRATCH_VIEW_ANGLE_Y;
        v1 -= 0xC00;
        v1 += angle;
        s0v = v1 & 0xFFF;
        sinv = rsin(s0v);
        v1 = s1 << 16;
        s1 = v1 >> 16;
        v0 = s2 + (s1 * sinv) / 4096;
        s5 = v0 + 0x20;
        sinv = rsin(s0v);
        v1 = -s1;
        v0 = s2 + (v1 * sinv) / 4096;
        a2v = v0 + 0x20;
        if (s6 < 0) {
            s6 = -s6;
        }
    }

    if (s6 == 1) {
        if (g_MirrorMode != 0) {
            SetStereoSoundCue(2, (s16)s5, (s16)a2v);
        } else {
            SetStereoSoundCue(2, (s16)a2v, (s16)s5);
        }
    } else {
        if (g_MirrorMode != 0) {
            SetStereoSoundCue(3, (s16)s5, (s16)a2v);
        } else {
            SetStereoSoundCue(3, (s16)a2v, (s16)s5);
        }
    }
}
