#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"
#include "psyq/gte.h"

enum {
    FLYBY_LIFETIME_FRAMES = 0x1C3,
    FLYBY_MAX_VOLUME = 0x74,
    FLYBY_ENGINE_PITCH = 0x1900,
};

static void StartFlybyIfTriggered(void) {
    const s32 series = g_RaceSeries;
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

static SceneryMotionKeyframe *AdvanceFlybyKeyframe(void) {
    SceneryMotionKeyframe *keyframe;

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

static void UpdateFlybyTransform(SceneryMotionKeyframe *keyframe) {
    Matrix rotationY;
    Matrix rotationX;
    SVec direction = {0, 0, (s16)(-keyframe->speed * 4), 0};
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

    BuildRotMatrixY(&rotationY, 0x800 - g_FlybyScenery.rotationY);
    BuildRotMatrixX(&rotationX, g_FlybyScenery.rotationX);
    MulMatrix2(&rotationY, &rotationX);
    BuildRotMatrixZ(&rotationY, g_FlybyScenery.rotationZ);
    MulMatrix(&rotationX, &rotationY);
    ApplyMatrix(&rotationX, &direction, &step);
    g_FlybyScenery.position.x += step.x / 4;
    g_FlybyScenery.position.y += step.y / 4;
    g_FlybyScenery.position.z += step.z / 4;
}

static void GetFlybyAudio(s32 active, s32 *pitch, s32 *volume) {
    s32 dx;
    s32 dy;
    s32 dz;
    s32 distance;

    *pitch = 0;
    *volume = 0;
    if (!active || g_FlybyScenery.soundEnabled != 1) {
        g_FlybyScenery.volume = 0;
        return;
    }

    dx = g_PlayerCar.x - g_FlybyScenery.position.x;
    dy = g_PlayerCar.y - g_FlybyScenery.position.y;
    dz = g_PlayerCar.z - g_FlybyScenery.position.z;
    distance = SquareRoot12(dx * dx / 8 + dy * dy / 16 + dz * dz / 8) >> 12;
    if (distance < 0) {
        g_FlybyScenery.soundEnabled = 0;
        distance = FLYBY_MAX_VOLUME;
    }

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
