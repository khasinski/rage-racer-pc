#include "game/race.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

char g_ClockTextCells[8] = "00'00\"";
char g_TimeTextBuffer[12] = "0'00\"000";

static s32 s_x;
static s32 s_y;
static s32 s_color;
static char s_text[16];

void DrawText8x8(s32 x, s32 y, const char *text, s32 color) {
    s_x = x;
    s_y = y;
    s_color = color;
    snprintf(s_text, sizeof(s_text), "%s", text);
}

static int CheckDraw(const char *expected, s32 x, s32 y, s32 color) {
    if (strcmp(s_text, expected) == 0 && s_x == x && s_y == y &&
        s_color == color) {
        return 0;
    }
    fprintf(stderr, "got text '%s' at %d,%d color %x; expected '%s'\n",
            s_text, s_x, s_y, s_color, expected);
    return 1;
}

int main(void) {
    DrawTimeValue(10, 20, 83456, 0x78CC, 1000);
    if (CheckDraw("1'23\"456", 10, 20, 0x78CC)) return 1;

    DrawTimeValue(1, 2, -1, 3, 1000);
    if (CheckDraw("-'--\"---", 1, 2, 3)) return 1;

    DrawTimeValue(4, 5, 1000, 6, 0);
    if (CheckDraw("-'--\"---", 4, 5, 6)) return 1;

    DrawMinuteSecondTime(7, 8, 9 * 60 * 25 + 59 * 25, 0x780F);
    if (CheckDraw(" 9'59\"", 7, 8, 0x780F)) return 1;

    DrawMinuteSecondTime(11, 12, 12 * 60 * 25 + 34 * 25, 0x7811);
    return CheckDraw("12'34\"", 11, 12, 0x7811);
}
