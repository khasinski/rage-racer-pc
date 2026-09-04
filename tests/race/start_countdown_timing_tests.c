#include "game/race_hud_internal.h"

#include <limits.h>
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
    StartCountdownPattern
        glyphs[START_COUNTDOWN_GLYPH_PATTERN_COUNT] = {{0}};
    StartCountdownPattern first = {0};
    StartCountdownRow row;
    StartCountdownLamp lamp;

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
    if (Check(INT_MIN, 0, 0, 0)) return 1;
    if (Check(INT_MAX, 0, 0, 0)) return 1;

    if (CountdownTileBufferIndex(0) != 0 ||
        CountdownTileBufferIndex(1) != 1 ||
        CountdownTileBufferIndex(INT_MIN) != 0 ||
        CountdownTileBufferIndex(INT_MAX) != 0) return 1;

    glyphs[1][3] = 0x12345678;
    first[3] = 0x89ABCDEF;
    row = BuildStartCountdownRow(0, 3, 0, glyphs, first);
    if (row.pattern != UINT32_MAX || row.colorBank != 0) return 1;
    row = BuildStartCountdownRow(1, 3, 0, glyphs, first);
    if (row.pattern != 0x12345678 || row.colorBank != 0) return 1;
    row = BuildStartCountdownRow(4, 3, 0, glyphs, first);
    if (row.pattern != 0x89ABCDEF || row.colorBank != 1) return 1;
    row = BuildStartCountdownRow(-1, 3, 0, glyphs, first);
    if (row.pattern != 0x89ABCDEF || row.colorBank != 1) return 1;
    row = BuildStartCountdownRow(1, -1, 0, glyphs, first);
    if (row.pattern != 0) return 1;
    row = BuildStartCountdownRow(1, 16, 0, glyphs, first);
    if (row.pattern != 0) return 1;
    row = BuildStartCountdownRow(1, 3, 0, NULL, first);
    if (row.pattern != 0) return 1;

    glyphs[1][5] = 0x0F0F0F0F;
    glyphs[1][6] = 0x00FF00FF;
    row = BuildStartCountdownRow(1, 5, 2, glyphs, first);
    if (row.pattern != 0x0F0F0F0F) return 1;
    row = BuildStartCountdownRow(1, 6, 2, glyphs, first);
    if (row.pattern != ~UINT32_C(0x00FF00FF)) return 1;

    if (AdvanceStartCountdownBoard(0, -64) != 0) return 1;
    if (AdvanceStartCountdownBoard(4, -64) != 0) return 1;
    if (AdvanceStartCountdownBoard(-1, 0) != -16) return 1;
    if (AdvanceStartCountdownBoard(-1, -224) != -240) return 1;
    if (AdvanceStartCountdownBoard(-1, -240) != -240) return 1;
    if (AdvanceStartCountdownBoard(-1, INT_MIN) != -240) return 1;
    if (AdvanceStartCountdownBoard(-1, INT_MAX) != -16) return 1;

    lamp = BuildStartCountdownLamp(0, 105, 0);
    if (lamp.intensity != 0x80 || lamp.clut != 0x784F) return 1;
    lamp = BuildStartCountdownLamp(1, 120, 0);
    if (lamp.intensity != 0 || lamp.clut != 0x7851) return 1;
    lamp = BuildStartCountdownLamp(1, 130, 0);
    if (lamp.intensity != 80 || lamp.clut != 0x7851) return 1;
    lamp = BuildStartCountdownLamp(1, 140, 3);
    if (lamp.intensity != 0x80 || lamp.clut != 0x7851) return 1;
    lamp = BuildStartCountdownLamp(2, 150, 2);
    if (lamp.intensity != 0x80 || lamp.clut != 0x784F) return 1;
    lamp = BuildStartCountdownLamp(4, 210, 4);
    if (lamp.intensity != 0 || lamp.clut != 0x7850) return 1;
    lamp = BuildStartCountdownLamp(4, 225, 4);
    if (lamp.intensity != 0x80 || lamp.clut != 0x7850) return 1;
    lamp = BuildStartCountdownLamp(-1, 250, 5);
    if (lamp.intensity != 0x80 || lamp.clut != 0x7850) return 1;
    lamp = BuildStartCountdownLamp(1, INT_MIN, -3);
    return lamp.intensity < 0 || lamp.intensity > 0x80;
}
