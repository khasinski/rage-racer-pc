#include "game/race_hud_internal.h"

StartCountdownTiming CalculateStartCountdownTiming(s32 sceneTimer) {
    StartCountdownTiming timing = {0};
    s32 elapsed;

    if (sceneTimer < 105 || sceneTimer >= 300) {
        return timing;
    }

    timing.visible = 1;
    elapsed = sceneTimer - 90;
    timing.phase = elapsed / 30;
    if (timing.phase >= 5) {
        timing.phase = -1;
    }

    timing.wipeHalfStep = (sceneTimer % 30) / 2;
    if (timing.phase == 4 || timing.phase < 0) {
        timing.wipeHalfStep = (sceneTimer & 2) << 2;
    } else if (timing.phase == 0) {
        timing.wipeHalfStep = 0;
    } else if (timing.wipeHalfStep > 8) {
        timing.wipeHalfStep = 8;
    }

    return timing;
}
