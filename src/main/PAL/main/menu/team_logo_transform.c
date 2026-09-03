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

void ScrollTeamLogoUp(void) {
    s32 row;
    u32 firstRow[TEAM_LOGO_WORDS_PER_ROW];

    PlaySoundCue(1);
    memcpy(firstRow, g_TeamLogoCanvas.words[0], sizeof(firstRow));
    for (row = 0; row < TEAM_LOGO_HEIGHT - 1; row++) {
        memcpy(g_TeamLogoCanvas.words[row], g_TeamLogoCanvas.words[row + 1],
               sizeof(firstRow));
    }
    memcpy(g_TeamLogoCanvas.words[TEAM_LOGO_HEIGHT - 1], firstRow,
           sizeof(firstRow));
}

void ScrollTeamLogoDown(void) {
    s32 row;
    u32 lastRow[TEAM_LOGO_WORDS_PER_ROW];

    PlaySoundCue(1);
    memcpy(lastRow, g_TeamLogoCanvas.words[TEAM_LOGO_HEIGHT - 1],
           sizeof(lastRow));
    for (row = TEAM_LOGO_HEIGHT - 1; row > 0; row--) {
        memcpy(g_TeamLogoCanvas.words[row], g_TeamLogoCanvas.words[row - 1],
               sizeof(lastRow));
    }
    memcpy(g_TeamLogoCanvas.words[0], lastRow, sizeof(lastRow));
}

void ScrollTeamLogoLeft(void) {
    s32 row;
    s32 word;

    PlaySoundCue(1);
    for (row = 0; row < TEAM_LOGO_HEIGHT; row++) {
        u32 wrap = g_TeamLogoCanvas.words[row][0] << 28;

        for (word = 0; word < TEAM_LOGO_WORDS_PER_ROW - 1; word++) {
            g_TeamLogoCanvas.words[row][word] =
                (g_TeamLogoCanvas.words[row][word] >> 4) |
                (g_TeamLogoCanvas.words[row][word + 1] << 28);
        }
        g_TeamLogoCanvas.words[row][TEAM_LOGO_WORDS_PER_ROW - 1] =
            (g_TeamLogoCanvas.words[row][TEAM_LOGO_WORDS_PER_ROW - 1] >> 4) |
            wrap;
    }
}

void ScrollTeamLogoRight(void) {
    s32 row;
    s32 word;

    PlaySoundCue(1);
    for (row = 0; row < TEAM_LOGO_HEIGHT; row++) {
        u32 wrap =
            g_TeamLogoCanvas.words[row][TEAM_LOGO_WORDS_PER_ROW - 1] >> 28;

        for (word = TEAM_LOGO_WORDS_PER_ROW - 1; word > 0; word--) {
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
    for (row = 0; row < TEAM_LOGO_HEIGHT / 2; row++) {
        for (word = 0; word < TEAM_LOGO_WORDS_PER_ROW; word++) {
            u32 temp = g_TeamLogoCanvas.words[row][word];

            g_TeamLogoCanvas.words[row][word] =
                g_TeamLogoCanvas.words[TEAM_LOGO_HEIGHT - 1 - row][word];
            g_TeamLogoCanvas.words[TEAM_LOGO_HEIGHT - 1 - row][word] = temp;
        }
    }
}

void FlipTeamLogoHorizontal(void) {
    s32 row;
    s32 word;

    PlaySoundCue(8);
    for (row = 0; row < TEAM_LOGO_HEIGHT; row++) {
        for (word = 0; word < TEAM_LOGO_WORDS_PER_ROW / 2; word++) {
            u32 left = g_TeamLogoCanvas.words[row][word];
            u32 right = g_TeamLogoCanvas
                            .words[row][TEAM_LOGO_WORDS_PER_ROW - 1 - word];

            g_TeamLogoCanvas.words[row][word] = ReverseLogoWord(right);
            g_TeamLogoCanvas.words[row][TEAM_LOGO_WORDS_PER_ROW - 1 - word] =
                ReverseLogoWord(left);
        }
    }
}

void RotateTeamLogoCcw(void) {
    TeamLogoCanvas source = g_TeamLogoCanvas;
    s32 x;
    s32 y;

    PlaySoundCue(8);
    for (y = 0; y < TEAM_LOGO_HEIGHT; y++) {
        for (x = 0; x < TEAM_LOGO_WIDTH; x++) {
            SetTeamLogoCanvasPixel(
                &g_TeamLogoCanvas, x, y,
                GetTeamLogoCanvasPixel(&source, TEAM_LOGO_WIDTH - 1 - y, x));
        }
    }
}

void RotateTeamLogoCw(void) {
    TeamLogoCanvas source = g_TeamLogoCanvas;
    s32 x;
    s32 y;

    PlaySoundCue(8);
    for (y = 0; y < TEAM_LOGO_HEIGHT; y++) {
        for (x = 0; x < TEAM_LOGO_WIDTH; x++) {
            SetTeamLogoCanvasPixel(
                &g_TeamLogoCanvas, x, y,
                GetTeamLogoCanvasPixel(&source, y,
                                       TEAM_LOGO_HEIGHT - 1 - x));
        }
    }
}
