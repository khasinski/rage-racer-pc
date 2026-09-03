#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "psyq/gte.h"

#include <stdint.h>

enum {
    FLYBY_LIFETIME_FRAMES = 0x1C3,
    FLYBY_MAX_VOLUME = 0x74,
    FLYBY_ENGINE_PITCH = 0x1900,
    FLYBY_DISTANCE_SCALE = 64,
};

static void StartFlybyIfTriggered(void) {
    const s32 series = g_RaceSeries != 0;
    const s32 keyframeIndex =
        g_FlybySceneryData->firstKeyframe[series][0];

    if (g_PlayerCar.lap != g_FlybyScenery.lap ||
        g_PlayerCar.trackSection !=
            g_FlybySceneryData->triggerSection[series][0]) {
        return;
    }

    g_FlybyScenery.soundEnabled = 1;
    g_FlybyScenery.timer = 1;
    g_FlybyScenery.keyframeTime = 0;
    g_FlybyScenery.lap = 0;
    g_FlybyScenery.keyframeIndex = 0;
    g_FlybyScenery.position = g_FlybySceneryData->start[series].position;
    g_FlybyScenery.rotationX = 0;
    g_FlybyScenery.rotationY = 0;
    g_FlybyScenery.rotationZ = 0;
    g_FlybySceneryKeyframe =
        &g_FlybySceneryData->keyframes[keyframeIndex];
}

static const SceneryMotionKeyframe *AdvanceFlybyKeyframe(void) {
    const SceneryMotionKeyframe *keyframe;

    g_FlybyScenery.timer++;
    g_FlybyScenery.keyframeTime++;
    if (g_FlybyScenery.timer >= FLYBY_LIFETIME_FRAMES) {
        g_FlybyScenery.timer = 0;
    }

    keyframe = &g_FlybySceneryKeyframe[g_FlybyScenery.keyframeIndex];
    if (keyframe->duration == g_FlybyScenery.keyframeTime) {
        g_FlybyScenery.keyframeIndex++;
        g_FlybyScenery.keyframeTime = 0;
        keyframe++;
    }
    if (keyframe->duration == SCENERY_MOTION_END) {
        g_FlybyScenery.keyframeIndex = 0;
        keyframe = g_FlybySceneryKeyframe;
    }
    return keyframe;
}

static void UpdateFlybyTransform(const SceneryMotionKeyframe *keyframe) {
    Matrix rotationY;
    Matrix rotationX;
    SVec direction = {
        0, 0, WrapSigned16(-(int64_t)keyframe->speed * 4), 0
    };
    LVec step;
    const s32 elapsed = g_FlybyScenery.keyframeTime;

    g_FlybyScenery.rotationX = InterpolateSceneryMotionValue(
        keyframe->rotationX, keyframe[1].rotationX, elapsed,
        keyframe->duration);
    g_FlybyScenery.rotationY = InterpolateSceneryMotionValue(
        keyframe->rotationY, keyframe[1].rotationY, elapsed,
        keyframe->duration);
    g_FlybyScenery.rotationZ = InterpolateSceneryMotionValue(
        keyframe->rotationZ, keyframe[1].rotationZ, elapsed,
        keyframe->duration);

    BuildRotMatrixY(
        &rotationY,
        WrapSigned32(
            (int64_t)0x800 - g_FlybyScenery.rotationY));
    BuildRotMatrixX(&rotationX, g_FlybyScenery.rotationX);
    MulMatrix2(&rotationY, &rotationX);
    BuildRotMatrixZ(&rotationY, g_FlybyScenery.rotationZ);
    MulMatrix(&rotationX, &rotationY);
    ApplyMatrix(&rotationX, &direction, &step);
    g_FlybyScenery.position.x = WrapSigned32(
        (int64_t)g_FlybyScenery.position.x + step.x / 4);
    g_FlybyScenery.position.y = WrapSigned32(
        (int64_t)g_FlybyScenery.position.y + step.y / 4);
    g_FlybyScenery.position.z = WrapSigned32(
        (int64_t)g_FlybyScenery.position.z + step.z / 4);
}

static s32 FlybyDistance(int64_t dx, int64_t dy, int64_t dz) {
    const uint64_t x = dx < 0 ? (uint64_t)-dx : (uint64_t)dx;
    const uint64_t y = dy < 0 ? (uint64_t)-dy : (uint64_t)dy;
    const uint64_t z = dz < 0 ? (uint64_t)-dz : (uint64_t)dz;
    const uint64_t audibleRadius =
        (uint64_t)FLYBY_MAX_VOLUME * FLYBY_DISTANCE_SCALE;
    uint64_t squared;

    if (x >= audibleRadius * 3 || y >= audibleRadius * 4 ||
        z >= audibleRadius * 3) {
        return FLYBY_MAX_VOLUME;
    }
    squared = x * x / 8 + y * y / 16 + z * z / 8;
    if (squared >= audibleRadius * audibleRadius) {
        return FLYBY_MAX_VOLUME;
    }
    return (s32)(SquareRoot12((long)squared) >> 12);
}

static void GetFlybyAudio(s32 active, s32 *pitch, s32 *volume) {
    int64_t dx;
    int64_t dy;
    int64_t dz;
    s32 distance;

    *pitch = 0;
    *volume = 0;
    if (!active || g_FlybyScenery.soundEnabled != 1) {
        g_FlybyScenery.volume = 0;
        return;
    }

    dx = (int64_t)g_PlayerCar.x - g_FlybyScenery.position.x;
    dy = (int64_t)g_PlayerCar.y - g_FlybyScenery.position.y;
    dz = (int64_t)g_PlayerCar.z - g_FlybyScenery.position.z;
    distance = FlybyDistance(dx, dy, dz);

    *volume = FLYBY_MAX_VOLUME - distance;
    if (*volume > FLYBY_MAX_VOLUME) {
        *volume = FLYBY_MAX_VOLUME;
    } else if (*volume < 0) {
        *volume = 0;
    }
    *pitch = FLYBY_ENGINE_PITCH;
    g_FlybyScenery.volume = *volume;
}

static s32 FlybySoundCue(s32 course, s32 *pitch, s32 *volume) {
    switch (course) {
    case 0:
        return 1;
    case 2:
    case 3:
        return 2;
    default:
        *pitch = 0;
        *volume = 0;
        return 1;
    }
}

void UpdateFlybyScenery(void) {
    s32 pitch;
    s32 volume;
    s32 cue;
    s32 active;

    StartFlybyIfTriggered();
    active = g_FlybyScenery.timer > 0;
    if (active) {
        UpdateFlybyTransform(AdvanceFlybyKeyframe());
    }
    GetFlybyAudio(active, &pitch, &volume);

    if (g_RacePhase >= 3) {
        pitch = 0;
        volume = 0;
    }
    cue = FlybySoundCue(SeriesCourseIndex(), &pitch, &volume);
    SetPitchedSoundCue(cue, pitch, volume);
}
