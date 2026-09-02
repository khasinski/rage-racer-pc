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

s32 AdvanceStartCountdownBoard(s32 phase, s32 currentOffset) {
    if (phase >= 0) {
        return 0;
    }

    currentOffset -= 16;
    return currentOffset < -240 ? -240 : currentOffset;
}

StartCountdownLamp BuildStartCountdownLamp(s32 phase, s32 sceneTimer,
                                           s32 lampIndex) {
    StartCountdownLamp lamp;
    s32 column = lampIndex % 3;

    lamp.intensity = 0x80;
    if ((u32)phase < 4) {
        if (phase - 1 == column) {
            lamp.intensity = (sceneTimer % 30) * 8;
            if (lamp.intensity > 0x80) lamp.intensity = 0x80;
        }
        lamp.clut = phase - 1 >= column ? 0x7851 : 0x784F;
    } else {
        if (phase == 4) {
            lamp.intensity = (sceneTimer % 30) * 12;
            if (lamp.intensity > 0x80) lamp.intensity = 0x80;
        }
        lamp.clut = 0x7850;
    }
    return lamp;
}
