#include "game/race.h"
#include "game/audio.h"
#include "game/track_internal.h"
#include "game/player_car_internal.h"
#include "psyq/gte.h"

void DrawScriptedScenery(s32 flags) {
    switch (g_GrandPrixClass % 5) {
    case 0:
        if (flags != 0) {
            UpdateRouteScenery();
        }
        DrawRouteScenery();
        break;
    case 1:
    case 2:
        if (flags != 0) {
            UpdateRouteScenery();
            UpdateFlybyScenery();
        }
        DrawRouteScenery();
        DrawFlybyScenery();
        break;
    case 3:
    case 4:
        if (flags != 0) {
            UpdateRouteScenery();
            UpdateFlybyScenery();
            UpdatePathScenerySound();
        }
        DrawRouteScenery();
        DrawFlybyScenery();
        DrawPathScenery();
        break;
    case 5:
        break;
    }
}

/*
 * Ticks the course's permanently looping prop and its positional sound. Two
 * keyframe tracks (position at g_PathSceneryPosKeys, rotation at
 * g_PathSceneryRotKeys) are eased sinusoidally between waypoints. The sound is
 * cue zero with distance attenuation and a Doppler-like pitch shift.
 */
void UpdatePathScenerySound(void) {
    PathSceneryPositionKey *positionKeys;
    PathSceneryRotationKey *rotationKeys;
    s16 rate;
    s16 idx;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 vol;
    s32 pitch;
    s32 slew;
    PathSceneryPositionKey *sinRec;
    s32 product;
    s32 oldVolume;

    if (g_PathSceneryClock.posFrame == g_PathSceneryCursors.posSpan) {
        PathSceneryPositionKey *rec;
        PathSceneryPositionKey *stepRec;
        idx = (u16)g_PathSceneryCursors.posIndex;
        positionKeys = g_PathSceneryPosKeys;
        g_PathSceneryCursors.posPhase.value = 0;
        idx++;
        g_PathSceneryCursors.posIndex = idx;
        stepRec = &positionKeys[idx];
        if (stepRec->fields.span == -1) {
            idx = stepRec->fields.loopIndex;
            g_PathSceneryClock.posFrame = 0;
            g_PathSceneryCursors.posIndex = idx;
            if (idx > 0) {
                g_PathSceneryClock.posFrame =
                    positionKeys[idx - 1].fields.span;
            }
        }
        rate = g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex].fields.rate;
        if (rate < 0) {
            rate = -rate;
            g_PathSceneryCursors.posRate.value = rate;
        } else {
            if (rate == 0) {
                rate = 1;
            }
            g_PathSceneryCursors.posRate.value = rate;
        }
        rec = &g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex];
        g_PathSceneryCursors.posSpan =
            rec->fields.span;
        g_PathSceneryHalfDelta[0] =
            (rec[1].fields.x - rec[0].fields.x) / 2;
        g_PathSceneryHalfDelta[1] =
            (rec[1].fields.y - rec[0].fields.y) / 2;
        g_PathSceneryHalfDelta[2] =
            (rec[1].fields.z - rec[0].fields.z) / 2;
    } else {
        g_PathSceneryCursors.posPhase.value++;
    }

    if (g_PathSceneryCursors.posPhase.signedValue <= g_PathSceneryCursors.posRate.signedValue) {
        if (g_PathSceneryCursors.posPhase.signedValue <=
            g_PathSceneryCursors.posRate.signedValue / 2) {
            g_PathSceneryTransform.position.w[0] =
                g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex + 1].fields.x -
                g_PathSceneryHalfDelta[0] *
                    rcos((g_PathSceneryCursors.posPhase.signedValue << 11) /
                         g_PathSceneryCursors.posRate.signedValue) /
                    4096 -
                g_PathSceneryHalfDelta[0];
            g_PathSceneryTransform.position.w[1] =
                g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex + 1].fields.y -
                g_PathSceneryHalfDelta[1] *
                    rcos((g_PathSceneryCursors.posPhase.signedValue << 11) /
                         g_PathSceneryCursors.posRate.signedValue) /
                    4096 -
                g_PathSceneryHalfDelta[1];
            g_PathSceneryTransform.position.w[2] =
                g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex + 1].fields.z -
                g_PathSceneryHalfDelta[2] *
                    rcos((g_PathSceneryCursors.posPhase.signedValue << 11) /
                         g_PathSceneryCursors.posRate.signedValue) /
                    4096 -
                g_PathSceneryHalfDelta[2];
        } else {
            product =
                g_PathSceneryHalfDelta[0] *
                rsin((g_PathSceneryCursors.posPhase.signedValue << 11) /
                         g_PathSceneryCursors.posRate.signedValue -
                     0x400);
            sinRec = &g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex];
            if (product < 0) {
                product += 0xFFF;
            }
            g_PathSceneryTransform.position.w[0] =
                (product >> 12) + sinRec->fields.x +
                g_PathSceneryHalfDelta[0];

            product =
                g_PathSceneryHalfDelta[1] *
                rsin((g_PathSceneryCursors.posPhase.signedValue << 11) /
                         g_PathSceneryCursors.posRate.signedValue -
                     0x400);
            sinRec = &g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex];
            if (product < 0) {
                product += 0xFFF;
            }
            g_PathSceneryTransform.position.w[1] =
                (product >> 12) + sinRec->fields.y +
                g_PathSceneryHalfDelta[1];

            g_PathSceneryTransform.position.w[2] =
                g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex].fields.z +
                g_PathSceneryHalfDelta[2] *
                    rsin((g_PathSceneryCursors.posPhase.signedValue << 11) /
                             g_PathSceneryCursors.posRate.signedValue -
                         0x400) /
                    4096 +
                g_PathSceneryHalfDelta[2];
        }
    } else {
        g_PathSceneryTransform.position =
            g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex + 1].position;
    }

    if (g_PathSceneryClock.rotFrame == g_PathSceneryCursors.rotSpan) {
        PathSceneryRotationKey *rec;
        PathSceneryRotationKey *stepRec;
        idx = (u16)g_PathSceneryCursors.rotIndex;
        rotationKeys = g_PathSceneryRotKeys;
        g_PathSceneryCursors.rotPhase.value = 0;
        idx++;
        g_PathSceneryCursors.rotIndex = idx;
        stepRec = &rotationKeys[idx];
        if (stepRec->fields.span == -1) {
            idx = stepRec->fields.loopIndex;
            g_PathSceneryClock.rotFrame = 0;
            g_PathSceneryCursors.rotIndex = idx;
            if (idx > 0) {
                g_PathSceneryClock.rotFrame =
                    rotationKeys[idx - 1].fields.span;
            }
        }
        rate = g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex].fields.rate;
        if (rate < 0) {
            rate = -rate;
            g_PathSceneryCursors.rotRate.value = rate;
        } else {
            if (rate == 0) {
                rate = 1;
            }
            g_PathSceneryCursors.rotRate.value = rate;
        }
        rec = &g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex];
        g_PathSceneryCursors.rotRate.value =
            rec->fields.rate;
        g_PathSceneryCursors.rotSpan =
            rec->fields.span;
        g_PathSceneryRotHalfDelta[0] =
            (rec[1].fields.x - rec[0].fields.x) / 2;
        g_PathSceneryRotHalfDelta[1] =
            (rec[1].fields.y - rec[0].fields.y) / 2;
        g_PathSceneryRotHalfDelta[2] =
            (rec[1].fields.z - rec[0].fields.z) / 2;
    } else {
        g_PathSceneryCursors.rotPhase.value++;
    }

    if (g_PathSceneryCursors.rotPhase.signedValue <= g_PathSceneryCursors.rotRate.signedValue) {
        if (g_PathSceneryCursors.rotPhase.signedValue <=
            g_PathSceneryCursors.rotRate.signedValue / 2) {
            g_PathSceneryTransform.rotation.vx =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex + 1].fields.x -
                g_PathSceneryRotHalfDelta[0] *
                    rcos((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                         g_PathSceneryCursors.rotRate.signedValue) /
                    4096 -
                g_PathSceneryRotHalfDelta[0];
            g_PathSceneryTransform.rotation.vy =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex + 1].fields.y -
                g_PathSceneryRotHalfDelta[1] *
                    rcos((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                         g_PathSceneryCursors.rotRate.signedValue) /
                    4096 -
                g_PathSceneryRotHalfDelta[1];
            g_PathSceneryTransform.rotation.vz =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex + 1].fields.z -
                g_PathSceneryRotHalfDelta[2] *
                    rcos((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                         g_PathSceneryCursors.rotRate.signedValue) /
                    4096 -
                g_PathSceneryRotHalfDelta[2];
        } else {
            g_PathSceneryTransform.rotation.vx =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex].fields.x +
                g_PathSceneryRotHalfDelta[0] *
                    rsin((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                             g_PathSceneryCursors.rotRate.signedValue -
                         0x400) /
                    4096 +
                g_PathSceneryRotHalfDelta[0];
            g_PathSceneryTransform.rotation.vy =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex].fields.y +
                g_PathSceneryRotHalfDelta[1] *
                    rsin((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                             g_PathSceneryCursors.rotRate.signedValue -
                         0x400) /
                    4096 +
                g_PathSceneryRotHalfDelta[1];
            g_PathSceneryTransform.rotation.vz =
                g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex].fields.z +
                g_PathSceneryRotHalfDelta[2] *
                    rsin((g_PathSceneryCursors.rotPhase.signedValue << 11) /
                             g_PathSceneryCursors.rotRate.signedValue -
                         0x400) /
                    4096 +
                g_PathSceneryRotHalfDelta[2];
        }
    } else {
        g_PathSceneryTransform.rotation =
            g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex + 1].rotation;
    }

    dx = g_PlayerCar.x - g_PathSceneryTransform.position.w[0];
    g_PathSceneryClock.posFrame =
        (s16)((u16)g_PathSceneryClock.posFrame + 1u);
    g_PathSceneryClock.rotFrame =
        (s16)((u16)g_PathSceneryClock.rotFrame + 1u);
    dy = g_PlayerCar.y - g_PathSceneryTransform.position.w[1];
    dz = g_PlayerCar.z - g_PathSceneryTransform.position.w[2];

    if (dx < 0x1000 && dz < 0x1000 && dx >= -0xFFF && dz >= -0xFFF) {
        slew =
            SquareRoot12(dx * dx / 4 + dy * dy / 8 + dz * dz / 4) >> 10;
        vol = 0x64 - slew;
        if (vol >= 0x65) {
            vol = 0x64;
        }
        if (vol < 0) {
            vol = 0;
        }
        oldVolume = g_PathSceneryVolume;
        slew = vol - oldVolume;
        if (slew < -0x14) {
            slew = -0x14;
        }
        if (slew >= 0x15) {
            slew = 0x14;
        }
        oldVolume = slew / 2 + 0x3C;
        g_PathSceneryVolume = vol;
        pitch = oldVolume << 7;
    } else {
        vol = 0;
        g_PathSceneryVolume = 0;
        pitch = 0;
    }

    if (g_RacePhase >= 3) {
        pitch = 0;
        vol = 0;
    }
    SetPitchedSoundCue(0, pitch, vol);
}
