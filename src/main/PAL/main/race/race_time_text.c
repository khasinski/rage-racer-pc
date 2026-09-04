#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"

enum {
    TIME_MINUTES = 0,
    TIME_SECONDS_TENS = 2,
    TIME_SECONDS_UNITS = 3,
    TIME_FRACTION_HUNDREDS = 5,
    TIME_FRACTION_TENS = 6,
    TIME_FRACTION_UNITS = 7,
    CLOCK_MINUTES_TENS = 0,
    CLOCK_MINUTES_UNITS = 1,
    CLOCK_SECONDS_TENS = 3,
    CLOCK_SECONDS_UNITS = 4,
    TIME_DISPLAY_MAX_SECONDS = 9 * 60 + 59,
    CLOCK_DISPLAY_MAX_SECONDS = 99 * 60 + 59,
};

static void FillTimeDigits(char digit) {
    static const u8 digitSlots[] = {
        TIME_MINUTES,
        TIME_SECONDS_TENS,
        TIME_SECONDS_UNITS,
        TIME_FRACTION_HUNDREDS,
        TIME_FRACTION_TENS,
        TIME_FRACTION_UNITS,
    };
    s32 index;

    for (index = 0;
         index < (s32)(sizeof(digitSlots) / sizeof(digitSlots[0])); index++) {
        g_TimeTextBuffer[digitSlots[index]] = digit;
    }
}

void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor) {
    if (value >= 0 && divisor > 0) {
        s32 totalSeconds = value / divisor;
        s32 fraction;

        if (totalSeconds > TIME_DISPLAY_MAX_SECONDS) {
            totalSeconds = TIME_DISPLAY_MAX_SECONDS;
            fraction = 999;
        } else {
            fraction =
                (s32)(((int64_t)(value % divisor) * 1000) / divisor);
        }

        g_TimeTextBuffer[TIME_MINUTES] = totalSeconds / 60 + '0';
        g_TimeTextBuffer[TIME_SECONDS_TENS] = totalSeconds % 60 / 10 + '0';
        g_TimeTextBuffer[TIME_SECONDS_UNITS] = totalSeconds % 10 + '0';
        g_TimeTextBuffer[TIME_FRACTION_HUNDREDS] = fraction / 100 + '0';
        g_TimeTextBuffer[TIME_FRACTION_TENS] = fraction / 10 % 10 + '0';
        g_TimeTextBuffer[TIME_FRACTION_UNITS] = fraction % 10 + '0';
    } else {
        FillTimeDigits('-');
    }

    DrawText8x8(x, y, g_TimeTextBuffer, color);
}

void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color) {
    s32 totalSeconds = ticks > 0 ? ticks / RACE_FRAMES_PER_SECOND : 0;
    s32 minutes = totalSeconds / 60;
    s32 seconds = totalSeconds % 60;

    if (totalSeconds > CLOCK_DISPLAY_MAX_SECONDS) {
        minutes = 99;
        seconds = 59;
    }

    g_ClockTextCells[CLOCK_MINUTES_TENS] =
        minutes < 10 ? ' ' : minutes / 10 + '0';
    g_ClockTextCells[CLOCK_MINUTES_UNITS] = minutes % 10 + '0';
    g_ClockTextCells[CLOCK_SECONDS_TENS] = seconds / 10 + '0';
    g_ClockTextCells[CLOCK_SECONDS_UNITS] = seconds % 10 + '0';
    DrawText8x8(x, y, g_ClockTextCells, color);
}
