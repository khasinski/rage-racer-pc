#include "game/race.h"
#include "game/render.h"

void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor) {
    if (value >= 0 && divisor > 0) {
        s32 totalSeconds = value / divisor;
        s32 fraction = (value % divisor) * 1000 / divisor;

        g_TimeTextBuffer[0] = totalSeconds / 60 + '0';
        g_TimeTextBuffer[2] = totalSeconds % 60 / 10 + '0';
        g_TimeTextBuffer[3] = totalSeconds % 10 + '0';
        g_TimeTextBuffer[5] = fraction / 100 + '0';
        g_TimeTextBuffer[6] = fraction / 10 % 10 + '0';
        g_TimeTextBuffer[7] = fraction % 10 + '0';
    } else {
        static const u8 digitSlots[] = {0, 2, 3, 5, 6, 7};
        s32 index;

        for (index = 0; index < (s32)(sizeof(digitSlots)); index++) {
            g_TimeTextBuffer[digitSlots[index]] = '-';
        }
    }

    DrawText8x8(x, y, g_TimeTextBuffer, color);
}

void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color) {
    s32 totalSeconds = ticks / 25;
    s32 minutes = totalSeconds / 60;
    s32 seconds = totalSeconds % 60;

    g_ClockTextBuffer = minutes < 10 ? ' ' : minutes / 10 + '0';
    g_ClockTextMinUnits[0] = minutes % 10 + '0';
    g_ClockTextSecTens = seconds / 10 + '0';
    g_ClockTextSecUnits = seconds % 10 + '0';
    DrawText8x8(x, y, g_ClockTextCells, color);
}
