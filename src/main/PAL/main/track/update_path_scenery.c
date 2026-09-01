#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"
#include "psyq/gte.h"

static void AdvancePositionKeyframe(void) {
    PathSceneryPositionKey *keyframe;
    s16 index;
    int axis;

    if (g_PathSceneryClock.posFrame != g_PathSceneryCursors.posSpan) {
        g_PathSceneryCursors.posPhase.value++;
        return;
    }

    g_PathSceneryCursors.posPhase.value = 0;
    index = (s16)((u16)g_PathSceneryCursors.posIndex + 1u);
    keyframe = &g_PathSceneryPosKeys[index];
    if (keyframe->fields.span == -1) {
        index = (s16)keyframe->fields.loopIndex;
        g_PathSceneryClock.posFrame = index > 0
            ? g_PathSceneryPosKeys[index - 1].fields.span
            : 0;
        keyframe = &g_PathSceneryPosKeys[index];
    }

    g_PathSceneryCursors.posIndex = index;
    g_PathSceneryCursors.posRate.value =
        NormalizePathSceneryRate(keyframe->fields.rate);
    g_PathSceneryCursors.posSpan = keyframe->fields.span;
    for (axis = 0; axis < 3; axis++) {
        g_PathSceneryHalfDelta[axis] =
            (s16)((keyframe[1].position.w[axis] -
                   keyframe[0].position.w[axis]) / 2);
    }
}

static void AdvanceRotationKeyframe(void) {
    PathSceneryRotationKey *keyframe;
    s16 index;

    if (g_PathSceneryClock.rotFrame != g_PathSceneryCursors.rotSpan) {
        g_PathSceneryCursors.rotPhase.value++;
        return;
    }

    g_PathSceneryCursors.rotPhase.value = 0;
    index = (s16)((u16)g_PathSceneryCursors.rotIndex + 1u);
    keyframe = &g_PathSceneryRotKeys[index];
    if (keyframe->fields.span == -1) {
        index = (s16)keyframe->fields.loopIndex;
        g_PathSceneryClock.rotFrame = index > 0
            ? g_PathSceneryRotKeys[index - 1].fields.span
            : 0;
        keyframe = &g_PathSceneryRotKeys[index];
    }

    g_PathSceneryCursors.rotIndex = index;
    g_PathSceneryCursors.rotRate.value =
        NormalizePathSceneryRate(keyframe->fields.rate);
    g_PathSceneryCursors.rotSpan = keyframe->fields.span;
    g_PathSceneryRotHalfDelta[0] =
        (s16)((keyframe[1].fields.x - keyframe[0].fields.x) / 2);
    g_PathSceneryRotHalfDelta[1] =
        (s16)((keyframe[1].fields.y - keyframe[0].fields.y) / 2);
    g_PathSceneryRotHalfDelta[2] =
        (s16)((keyframe[1].fields.z - keyframe[0].fields.z) / 2);
}

static s32 EasePathValue(s32 start, s32 end, s32 halfDelta,
                         s16 phase, s16 rate) {
    s32 angle;

    if (phase > rate) {
        return end;
    }

    angle = (phase << 11) / rate;
    if (phase <= rate / 2) {
        return end - halfDelta * rcos(angle) / 4096 - halfDelta;
    }
    return start + halfDelta * rsin(angle - 0x400) / 4096 + halfDelta;
}

static void UpdatePathPosition(void) {
    PathSceneryPositionKey *keyframe =
        &g_PathSceneryPosKeys[g_PathSceneryCursors.posIndex];
    const s16 phase = g_PathSceneryCursors.posPhase.signedValue;
    const s16 rate = g_PathSceneryCursors.posRate.signedValue;
    int axis;

    for (axis = 0; axis < 3; axis++) {
        g_PathSceneryTransform.position.w[axis] =
            EasePathValue(keyframe[0].position.w[axis],
                          keyframe[1].position.w[axis],
                          g_PathSceneryHalfDelta[axis], phase, rate);
    }
    if (phase > rate) {
        g_PathSceneryTransform.position = keyframe[1].position;
    }
}

static void UpdatePathRotation(void) {
    PathSceneryRotationKey *keyframe =
        &g_PathSceneryRotKeys[g_PathSceneryCursors.rotIndex];
    const s16 phase = g_PathSceneryCursors.rotPhase.signedValue;
    const s16 rate = g_PathSceneryCursors.rotRate.signedValue;

    g_PathSceneryTransform.rotation.vx = (s16)EasePathValue(
        keyframe[0].fields.x, keyframe[1].fields.x,
        g_PathSceneryRotHalfDelta[0], phase, rate);
    g_PathSceneryTransform.rotation.vy = (s16)EasePathValue(
        keyframe[0].fields.y, keyframe[1].fields.y,
        g_PathSceneryRotHalfDelta[1], phase, rate);
    g_PathSceneryTransform.rotation.vz = (s16)EasePathValue(
        keyframe[0].fields.z, keyframe[1].fields.z,
        g_PathSceneryRotHalfDelta[2], phase, rate);
    if (phase > rate) {
        g_PathSceneryTransform.rotation = keyframe[1].rotation;
    }
}

static void UpdatePathSceneryAudio(void) {
    const s32 dx =
        g_PlayerCar.x - g_PathSceneryTransform.position.w[0];
    const s32 dy =
        g_PlayerCar.y - g_PathSceneryTransform.position.w[1];
    const s32 dz =
        g_PlayerCar.z - g_PathSceneryTransform.position.w[2];
    s32 pitch = 0;
    s32 volume = 0;

    if (dx < 0x1000 && dz < 0x1000 && dx >= -0xFFF && dz >= -0xFFF) {
        const s32 distance =
            SquareRoot12(dx * dx / 4 + dy * dy / 8 + dz * dz / 4) >> 10;
        s32 volumeDelta;

        volume = 0x64 - distance;
        if (volume > 0x64) {
            volume = 0x64;
        } else if (volume < 0) {
            volume = 0;
        }

        volumeDelta = volume - g_PathSceneryVolume;
        if (volumeDelta < -0x14) {
            volumeDelta = -0x14;
        } else if (volumeDelta > 0x14) {
            volumeDelta = 0x14;
        }
        pitch = (volumeDelta / 2 + 0x3C) << 7;
        g_PathSceneryVolume = volume;
    } else {
        g_PathSceneryVolume = 0;
    }

    if (g_RacePhase >= 3) {
        pitch = 0;
        volume = 0;
    }
    SetPitchedSoundCue(0, pitch, volume);
}

void UpdatePathScenerySound(void) {
    AdvancePositionKeyframe();
    UpdatePathPosition();
    AdvanceRotationKeyframe();
    UpdatePathRotation();

    g_PathSceneryClock.posFrame =
        (s16)((u16)g_PathSceneryClock.posFrame + 1u);
    g_PathSceneryClock.rotFrame =
        (s16)((u16)g_PathSceneryClock.rotFrame + 1u);
    UpdatePathSceneryAudio();
}
