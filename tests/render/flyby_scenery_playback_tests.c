#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_CourseIndex;
s32 g_RaceSeries;
s16 g_RacePhase;
PlayerCarRuntime g_PlayerCar;
SceneryMotionData *g_FlybySceneryData;
FlybySceneryState g_FlybyScenery;
SceneryMotionKeyframe *g_FlybySceneryKeyframe;

static s32 g_LastCue;
static s32 g_LastPitch;
static s32 g_LastVolume;

void SetPitchedSoundCue(s32 cue, s32 pitch, s32 volume) {
    g_LastCue = cue;
    g_LastPitch = pitch;
    g_LastVolume = volume;
}

static void BuildIdentity(void *matrix) {
    Matrix *mtx = matrix;

    memset(mtx, 0, sizeof(*mtx));
    mtx->m[0][0] = 0x1000;
    mtx->m[1][1] = 0x1000;
    mtx->m[2][2] = 0x1000;
}

void BuildRotMatrixX(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

void BuildRotMatrixZ(void *matrix, s32 angle) {
    (void)angle;
    BuildIdentity(matrix);
}

typedef struct FlybyFixture {
    s16 triggerSection[2][2];
    s16 firstKeyframe[2][2];
    SceneryMotionStart start[2];
    SceneryMotionKeyframe keyframes[3];
} FlybyFixture;

static int CheckAudio(s32 cue, s32 pitchEnabled) {
    if (g_LastCue != cue || (g_LastPitch != 0) != pitchEnabled ||
        (g_LastVolume != 0) != pitchEnabled) {
        printf("FAIL audio: cue=%d pitch=%d volume=%d\n",
               g_LastCue, g_LastPitch, g_LastVolume);
        return 0;
    }
    return 1;
}

int main(void) {
    FlybyFixture fixture;

    memset(&fixture, 0, sizeof(fixture));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    fixture.triggerSection[0][0] = 7;
    fixture.start[0].position = (Vec4){100, 200, 300, 400};
    fixture.keyframes[0] =
        (SceneryMotionKeyframe){0, 0, 0, 2, 4, 0};
    fixture.keyframes[1] =
        (SceneryMotionKeyframe){200, 400, 600, 2, 4, 0};
    fixture.keyframes[2].duration = -1;
    g_FlybySceneryData = (SceneryMotionData *)&fixture;
    g_FlybySceneryKeyframe = fixture.keyframes;
    g_PlayerCar.x = 100;
    g_PlayerCar.y = 200;
    g_PlayerCar.z = 300;

    g_FlybyScenery.lap = 2;
    g_PlayerCar.lap = 1;
    UpdateFlybyScenery();
    if (g_FlybyScenery.timer != 0 || !CheckAudio(1, 0)) {
        puts("FAIL: inactive flyby changed state");
        return 1;
    }

    g_PlayerCar.lap = 2;
    g_PlayerCar.trackSection = 7;
    UpdateFlybyScenery();
    if (g_FlybyScenery.timer != 2 || g_FlybyScenery.lap != 0 ||
        g_FlybyScenery.keyframeTime != 1 ||
        g_FlybyScenery.rotationX != 100 ||
        g_FlybyScenery.rotationY != 200 ||
        g_FlybyScenery.rotationZ != 300 ||
        g_FlybyScenery.position.z != 296 || !CheckAudio(1, 1)) {
        puts("FAIL: flyby trigger frame");
        return 1;
    }

    UpdateFlybyScenery();
    if (g_FlybyScenery.keyframeIndex != 1 ||
        g_FlybyScenery.keyframeTime != 0 ||
        g_FlybyScenery.rotationX != 200) {
        puts("FAIL: flyby keyframe advance");
        return 1;
    }
    UpdateFlybyScenery();
    UpdateFlybyScenery();
    if (g_FlybyScenery.keyframeIndex != 0 ||
        g_FlybyScenery.keyframeTime != 0) {
        puts("FAIL: flyby keyframe loop");
        return 1;
    }

    g_FlybyScenery.timer = 450;
    UpdateFlybyScenery();
    if (g_FlybyScenery.timer != 0 || !CheckAudio(1, 1)) {
        puts("FAIL: flyby final active frame");
        return 1;
    }
    UpdateFlybyScenery();
    if (!CheckAudio(1, 0) || g_FlybyScenery.volume != 0) {
        puts("FAIL: stopped flyby audio");
        return 1;
    }

    g_FlybyScenery.timer = 1;
    g_CourseIndex = 2;
    UpdateFlybyScenery();
    if (!CheckAudio(2, 1)) {
        return 1;
    }
    g_CourseIndex = 1;
    UpdateFlybyScenery();
    if (!CheckAudio(1, 0)) {
        return 1;
    }
    g_CourseIndex = 0;
    g_RacePhase = 3;
    UpdateFlybyScenery();
    if (!CheckAudio(1, 0)) {
        puts("FAIL: finished race did not mute flyby");
        return 1;
    }

    puts("flyby scenery playback preserved");
    return 0;
}
