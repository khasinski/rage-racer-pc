#include "game/race_hud_internal.h"

#include <stdio.h>

static int Check(s32 timer, s32 visible, s32 phase, s32 wipeHalfStep) {
    StartCountdownTiming timing = CalculateStartCountdownTiming(timer);

    if (timing.visible == visible && timing.phase == phase &&
        timing.wipeHalfStep == wipeHalfStep) {
        return 0;
    }
    fprintf(stderr, "timer %d: got {%d,%d,%d}, expected {%d,%d,%d}\n",
            timer, timing.visible, timing.phase, timing.wipeHalfStep,
            visible, phase, wipeHalfStep);
    return 1;
}

int main(void) {
    u32 glyphs[5 * 16] = {0};
    u32 first[16] = {0};
    StartCountdownRow row;

    if (Check(104, 0, 0, 0)) return 1;
    if (Check(105, 1, 0, 0)) return 1;
    if (Check(119, 1, 0, 0)) return 1;
    if (Check(120, 1, 1, 0)) return 1;
    if (Check(130, 1, 1, 5)) return 1;
    if (Check(149, 1, 1, 8)) return 1;
    if (Check(150, 1, 2, 0)) return 1;
    if (Check(210, 1, 4, 8)) return 1;
    if (Check(212, 1, 4, 0)) return 1;
    if (Check(214, 1, 4, 8)) return 1;
    if (Check(240, 1, -1, 0)) return 1;
    if (Check(242, 1, -1, 8)) return 1;
    if (Check(299, 1, -1, 8)) return 1;
    if (Check(300, 0, 0, 0)) return 1;

    glyphs[16 + 3] = 0x12345678;
    first[3] = 0x89ABCDEF;
    row = BuildStartCountdownRow(0, 3, 0, glyphs, first);
    if (row.pattern != UINT32_MAX || row.colorBank != 0) return 1;
    row = BuildStartCountdownRow(1, 3, 0, glyphs, first);
    if (row.pattern != 0x12345678 || row.colorBank != 0) return 1;
    row = BuildStartCountdownRow(4, 3, 0, glyphs, first);
    if (row.pattern != 0x89ABCDEF || row.colorBank != 1) return 1;
    row = BuildStartCountdownRow(-1, 3, 0, glyphs, first);
    if (row.pattern != 0x89ABCDEF || row.colorBank != 1) return 1;

    glyphs[16 + 5] = 0x0F0F0F0F;
    glyphs[16 + 6] = 0x00FF00FF;
    row = BuildStartCountdownRow(1, 5, 2, glyphs, first);
    if (row.pattern != 0x0F0F0F0F) return 1;
    row = BuildStartCountdownRow(1, 6, 2, glyphs, first);
    return row.pattern != ~UINT32_C(0x00FF00FF);
}
