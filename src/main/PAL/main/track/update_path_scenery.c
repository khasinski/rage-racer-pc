#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "psyq/gte.h"

#include <stdint.h>

enum {
    PATH_SCENERY_AUDIO_RANGE = 0x1000,
    PATH_SCENERY_MAX_VOLUME = 0x64,
    PATH_SCENERY_MAX_VOLUME_STEP = 0x14,
    PATH_SCENERY_BASE_PITCH = 0x3C,
};

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
            PathSceneryHalfDelta(keyframe[0].position.w[axis],
                                 keyframe[1].position.w[axis]);
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

    angle = (s32)((int64_t)phase * 0x800 / rate);
    if (phase <= rate / 2) {
        return WrapRenderCoordinate32(
            (int64_t)end - (int64_t)halfDelta * rcos(angle) / 4096 -
            halfDelta);
    }
    return WrapRenderCoordinate32(
        (int64_t)start +
        (int64_t)halfDelta * rsin(angle - 0x400) / 4096 + halfDelta);
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

    g_PathSceneryTransform.rotation.vx = WrapRenderCoordinate16(
        EasePathValue(keyframe[0].fields.x, keyframe[1].fields.x,
                      g_PathSceneryRotHalfDelta[0], phase, rate));
    g_PathSceneryTransform.rotation.vy = WrapRenderCoordinate16(
        EasePathValue(keyframe[0].fields.y, keyframe[1].fields.y,
                      g_PathSceneryRotHalfDelta[1], phase, rate));
    g_PathSceneryTransform.rotation.vz = WrapRenderCoordinate16(
        EasePathValue(keyframe[0].fields.z, keyframe[1].fields.z,
                      g_PathSceneryRotHalfDelta[2], phase, rate));
    if (phase > rate) {
        g_PathSceneryTransform.rotation = keyframe[1].rotation;
    }
}

static s32 PathSceneryDistance(int64_t dx, int64_t dy, int64_t dz) {
    const uint64_t x = dx < 0 ? (uint64_t)-dx : (uint64_t)dx;
    const uint64_t y = dy < 0 ? (uint64_t)-dy : (uint64_t)dy;
    const uint64_t z = dz < 0 ? (uint64_t)-dz : (uint64_t)dz;
    const uint64_t squared = x * x / 4 + y * y / 8 + z * z / 4;
    const uint64_t audibleRadius =
        (uint64_t)PATH_SCENERY_MAX_VOLUME * 16;

    if (squared >= audibleRadius * audibleRadius) {
        return PATH_SCENERY_MAX_VOLUME;
    }
    return (s32)(SquareRoot12((long)squared) >> 10);
}

static void UpdatePathSceneryAudio(void) {
    const int64_t dx =
        (int64_t)g_PlayerCar.x - g_PathSceneryTransform.position.w[0];
    const int64_t dy =
        (int64_t)g_PlayerCar.y - g_PathSceneryTransform.position.w[1];
    const int64_t dz =
        (int64_t)g_PlayerCar.z - g_PathSceneryTransform.position.w[2];
    s32 pitch = 0;
    s32 volume = 0;

    if (dx < PATH_SCENERY_AUDIO_RANGE &&
        dz < PATH_SCENERY_AUDIO_RANGE &&
        dx > -PATH_SCENERY_AUDIO_RANGE &&
        dz > -PATH_SCENERY_AUDIO_RANGE) {
        const s32 distance = PathSceneryDistance(dx, dy, dz);
        s32 volumeDelta;

        volume = PATH_SCENERY_MAX_VOLUME - distance;
        if (volume > PATH_SCENERY_MAX_VOLUME) {
            volume = PATH_SCENERY_MAX_VOLUME;
        } else if (volume < 0) {
            volume = 0;
        }

        volumeDelta = volume - g_PathSceneryVolume;
        if (volumeDelta < -PATH_SCENERY_MAX_VOLUME_STEP) {
            volumeDelta = -PATH_SCENERY_MAX_VOLUME_STEP;
        } else if (volumeDelta > PATH_SCENERY_MAX_VOLUME_STEP) {
            volumeDelta = PATH_SCENERY_MAX_VOLUME_STEP;
        }
        pitch = (volumeDelta / 2 + PATH_SCENERY_BASE_PITCH) << 7;
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

void UpdatePathScenery(void) {
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
