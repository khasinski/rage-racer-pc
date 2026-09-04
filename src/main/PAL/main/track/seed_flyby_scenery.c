#include "game/random.h"
#include "game/race.h"
#include "game/track_internal.h"

void SeedFlybyScenery(void) {
    s32 series;
    s32 keyframeIndex;
    s32 randomLap;

    if (g_FlybySceneryData == NULL) {
        g_FlybyScenery.timer = 0;
        g_FlybyScenery.soundEnabled = 0;
        g_FlybyScenery.volume = 0;
        g_FlybySceneryKeyframe = NULL;
        return;
    }
    series = g_RaceSeries != 0;
    keyframeIndex = g_FlybySceneryData->firstKeyframe[series][0];
    randomLap = Random15();

    g_FlybyScenery.lap = g_LapCount > 0
        ? (s16)(randomLap % g_LapCount + 1)
        : 1;
    g_FlybyScenery.soundEnabled = 1;
    g_FlybyScenery.timer = 0;
    g_FlybyScenery.position = g_FlybySceneryData->start[series].position;
    g_FlybyScenery.volume = 0;
    g_FlybySceneryKeyframe =
        &g_FlybySceneryData->keyframes[keyframeIndex];
}
