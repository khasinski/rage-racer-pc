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

StartCountdownRow BuildStartCountdownRow(s32 phase, s32 row,
                                         s32 wipeHalfStep,
                                         const u32 *glyphPatterns,
                                         const u32 *firstPattern) {
    StartCountdownRow result;

    result.colorBank = phase >= 4 || phase < 0;
    if (phase == 0) {
        result.pattern = UINT32_MAX;
    } else if (phase > 0 && phase < 4) {
        result.pattern = glyphPatterns[phase * 16 + row];
    } else {
        result.pattern = firstPattern[row];
    }

    if (row >= 8 - wipeHalfStep && row <= 7 + wipeHalfStep) {
        result.pattern = ~result.pattern;
    }
    return result;
}
