#include "common.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
s32 g_LapCount;
const SceneryMotionData *g_FlybySceneryData;
FlybySceneryState g_FlybyScenery;
const SceneryMotionKeyframe *g_FlybySceneryKeyframe;

static s32 g_RandomValue;

s32 Random15(void) {
    return g_RandomValue;
}

typedef struct FlybyFixture {
    s16 triggerSection[2][2];
    s16 firstKeyframe[2][2];
    SceneryMotionStart start[2];
    SceneryMotionKeyframe keyframes[4];
} FlybyFixture;

int main(void) {
    static const s32 randomValues[] = {0, 1, 2, 5, 32767};
    static const s32 lapCounts[] = {0, 3, 6};
    FlybyFixture fixture;
    size_t series;
    size_t count;
    size_t random;

    memset(&fixture, 0, sizeof(fixture));
    fixture.firstKeyframe[0][0] = 1;
    fixture.firstKeyframe[1][0] = 2;
    fixture.start[0].position = (Vec4){10, 20, 30, 40};
    fixture.start[1].position = (Vec4){100, 200, 300, 400};
    g_FlybySceneryData = (SceneryMotionData *)&fixture;

    for (series = 0; series < 2; series++) {
        for (count = 0; count < sizeof(lapCounts) / sizeof(lapCounts[0]);
             count++) {
            for (random = 0;
                 random < sizeof(randomValues) / sizeof(randomValues[0]);
                 random++) {
                memset(&g_FlybyScenery, 0x7F, sizeof(g_FlybyScenery));
                g_RaceSeries = series == 0 ? 0 : 7;
                g_LapCount = lapCounts[count];
                g_RandomValue = randomValues[random];

                SeedFlybyScenery();

                if (g_FlybyScenery.lap !=
                        (lapCounts[count] > 0
                             ? randomValues[random] % lapCounts[count] + 1
                             : 1) ||
                    g_FlybyScenery.soundEnabled != 1 ||
                    g_FlybyScenery.timer != 0 ||
                    g_FlybyScenery.volume != 0 ||
                    memcmp(&g_FlybyScenery.position,
                           &fixture.start[series].position,
                           sizeof(g_FlybyScenery.position)) != 0 ||
                    g_FlybySceneryKeyframe !=
                        &fixture.keyframes[fixture.firstKeyframe[series][0]]) {
                    puts("FAIL: flyby scenery seed state");
                    return 1;
                }
            }
        }
    }

    g_FlybySceneryData = NULL;
    g_FlybyScenery.timer = 12;
    g_FlybyScenery.soundEnabled = 1;
    g_FlybyScenery.volume = 99;
    SeedFlybyScenery();
    if (g_FlybyScenery.timer != 0 || g_FlybyScenery.soundEnabled != 0 ||
        g_FlybyScenery.volume != 0 || g_FlybySceneryKeyframe != NULL) {
        puts("FAIL: missing flyby data was not disabled");
        return 1;
    }

    puts("flyby scenery seeding preserved");
    return 0;
}
