#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"

#include <stdio.h>

enum {
    TIME_DISPLAY_MAX_SECONDS = 9 * 60 + 59,
    CLOCK_DISPLAY_MAX_SECONDS = 99 * 60 + 59,
};

void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor) {
    char text[9];

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

        snprintf(text, sizeof(text), "%d'%02d\"%03d", totalSeconds / 60,
                 totalSeconds % 60, fraction);
    } else {
        snprintf(text, sizeof(text), "-'--\"---");
    }

    DrawText8x8(x, y, text, color);
}

void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color) {
    char text[7];
    s32 totalSeconds = ticks > 0 ? ticks / RACE_FRAMES_PER_SECOND : 0;
    s32 minutes = totalSeconds / 60;
    s32 seconds = totalSeconds % 60;

    if (totalSeconds > CLOCK_DISPLAY_MAX_SECONDS) {
        minutes = 99;
        seconds = 59;
    }

    snprintf(text, sizeof(text), "%2d'%02d\"", minutes, seconds);
    DrawText8x8(x, y, text, color);
}
