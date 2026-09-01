#include "game/audio.h"
#include "game/menu.h"

#include <string.h>

static u32 ReverseLogoWord(u32 word) {
    u32 reversed = 0;
    s32 pixel;

    for (pixel = 0; pixel < 8; pixel++) {
        reversed = (reversed << 4) | ((word >> (pixel * 4)) & 0xF);
    }
    return reversed;
}

static u32 ReadLogoPixel(const TeamLogoCanvas *canvas, s32 x, s32 y) {
    return (canvas->words[y][x / 8] >> ((x & 7) * 4)) & 0xF;
}

static void WriteLogoPixel(TeamLogoCanvas *canvas, s32 x, s32 y, u32 colour) {
    s32 shift = (x & 7) * 4;
    u32 mask = 0xFu << shift;

    canvas->words[y][x / 8] =
        (canvas->words[y][x / 8] & ~mask) | ((colour & 0xF) << shift);
}

void ScrollTeamLogoUp(void) {
    s32 row;
    u32 firstRow[8];

    PlaySoundCue(1);
    memcpy(firstRow, g_TeamLogoCanvas.words[0], sizeof(firstRow));
    for (row = 0; row < 63; row++) {
        memcpy(g_TeamLogoCanvas.words[row], g_TeamLogoCanvas.words[row + 1],
               sizeof(firstRow));
    }
    memcpy(g_TeamLogoCanvas.words[63], firstRow, sizeof(firstRow));
}

void ScrollTeamLogoDown(void) {
    s32 row;
    u32 lastRow[8];

    PlaySoundCue(1);
    memcpy(lastRow, g_TeamLogoCanvas.words[63], sizeof(lastRow));
    for (row = 63; row > 0; row--) {
        memcpy(g_TeamLogoCanvas.words[row], g_TeamLogoCanvas.words[row - 1],
               sizeof(lastRow));
    }
    memcpy(g_TeamLogoCanvas.words[0], lastRow, sizeof(lastRow));
}

void ScrollTeamLogoLeft(void) {
    s32 row;
    s32 word;

    PlaySoundCue(1);
    for (row = 0; row < 64; row++) {
        u32 wrap = g_TeamLogoCanvas.words[row][0] << 28;

        for (word = 0; word < 7; word++) {
            g_TeamLogoCanvas.words[row][word] =
                (g_TeamLogoCanvas.words[row][word] >> 4) |
                (g_TeamLogoCanvas.words[row][word + 1] << 28);
        }
        g_TeamLogoCanvas.words[row][7] =
            (g_TeamLogoCanvas.words[row][7] >> 4) | wrap;
    }
}

void ScrollTeamLogoRight(void) {
    s32 row;
    s32 word;

    PlaySoundCue(1);
    for (row = 0; row < 64; row++) {
        u32 wrap = g_TeamLogoCanvas.words[row][7] >> 28;

        for (word = 7; word > 0; word--) {
            g_TeamLogoCanvas.words[row][word] =
                (g_TeamLogoCanvas.words[row][word] << 4) |
                (g_TeamLogoCanvas.words[row][word - 1] >> 28);
        }
        g_TeamLogoCanvas.words[row][0] =
            (g_TeamLogoCanvas.words[row][0] << 4) | wrap;
    }
}

void FlipTeamLogoVertical(void) {
    s32 row;
    s32 word;

    PlaySoundCue(8);
    for (row = 0; row < 32; row++) {
        for (word = 0; word < 8; word++) {
            u32 temp = g_TeamLogoCanvas.words[row][word];

            g_TeamLogoCanvas.words[row][word] =
                g_TeamLogoCanvas.words[63 - row][word];
            g_TeamLogoCanvas.words[63 - row][word] = temp;
        }
    }
}

void FlipTeamLogoHorizontal(void) {
    s32 row;
    s32 word;

    PlaySoundCue(8);
    for (row = 0; row < 64; row++) {
        for (word = 0; word < 4; word++) {
            u32 left = g_TeamLogoCanvas.words[row][word];
            u32 right = g_TeamLogoCanvas.words[row][7 - word];

            g_TeamLogoCanvas.words[row][word] = ReverseLogoWord(right);
            g_TeamLogoCanvas.words[row][7 - word] = ReverseLogoWord(left);
        }
    }
}

void RotateTeamLogoCcw(void) {
    TeamLogoCanvas source = g_TeamLogoCanvas;
    s32 x;
    s32 y;

    PlaySoundCue(8);
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            WriteLogoPixel(&g_TeamLogoCanvas, x, y,
                           ReadLogoPixel(&source, 63 - y, x));
        }
    }
}

void RotateTeamLogoCw(void) {
    TeamLogoCanvas source = g_TeamLogoCanvas;
    s32 x;
    s32 y;

    PlaySoundCue(8);
    for (y = 0; y < 64; y++) {
        for (x = 0; x < 64; x++) {
            WriteLogoPixel(&g_TeamLogoCanvas, x, y,
                           ReadLogoPixel(&source, y, 63 - x));
        }
    }
}
