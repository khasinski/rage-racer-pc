#include "game/menu.h"
#include "game/menu_internal.h"

enum {
    TEAM_NAME_GRID_COLUMNS = 11,
    TEAM_NAME_GRID_ROWS = 4,
    TEAM_NAME_GRID_CELL_COUNT = TEAM_NAME_GRID_COLUMNS * TEAM_NAME_GRID_ROWS,
    TEAM_NAME_GRID_X = 0x56,
    TEAM_NAME_GRID_Y = 0xF9,
    TEAM_NAME_CELL_WIDTH = 0xC,
    TEAM_NAME_CELL_HEIGHT = 0x18,
    TEAM_NAME_LAST_FRAME = 0x19,
};

typedef struct TeamNameKeyboardRow {
    u8 length;
    u8 textureU;
    u8 textureV;
} TeamNameKeyboardRow;

static const TeamNameKeyboardRow s_keyboardRows[TEAM_NAME_GRID_ROWS] = {
    {10, 0x00, 0x18},
    {11, 0x50, 0x18},
    {11, 0xA8, 0x18},
    {11, 0x00, 0x28},
};

static s32 ClampAnimationFrame(s32 frame, s32 lastFrame) {
    if (frame < 0) {
        return -1;
    }
    return frame < lastFrame ? frame : lastFrame;
}

static s32 SlideUp(s32 base, s32 frame, s32 distanceTimes32) {
    return base - ((frame * distanceTimes32 + 31) / 32);
}

static void DrawKeyboardCharacter(GameOrderingTableEntry *ot, s32 index, s32 x, s32 y,
                                  s32 height, u32 flags) {
    s32 atlasIndex = index;

    /* Index 10 is a blank cell between digits and letters. */
    if (atlasIndex == 10) {
        return;
    }
    if (atlasIndex > 10) {
        atlasIndex--;
    }
    DrawSprite(ot, (s16)x, (s16)y, 8, (u16)height,
               (u16)((atlasIndex % 32) * 8),
               (u16)((atlasIndex / 32) * 16 + 0x18), 0, 0, 0, 0x244, 1, 1,
               flags);
}

static void DrawKeyboardGrid(GameOrderingTableEntry *ot, s32 frame, s32 cursorIndex) {
    s32 y = SlideUp(TEAM_NAME_GRID_Y, frame, 64);
    s32 height = frame * 2;
    s32 row;
    s32 column;

    DrawKeyboardCharacter(ot, cursorIndex,
                          TEAM_NAME_GRID_X +
                              (cursorIndex % TEAM_NAME_GRID_COLUMNS) *
                                  TEAM_NAME_CELL_WIDTH,
                          y + (cursorIndex / TEAM_NAME_GRID_COLUMNS) *
                                  TEAM_NAME_CELL_HEIGHT,
                          height, 0x5B);

    for (row = 0; row < TEAM_NAME_GRID_ROWS; row++) {
        const TeamNameKeyboardRow *layout = &s_keyboardRows[row];

        for (column = 0; column < layout->length; column++) {
            DrawSprite(ot + 1,
                       (s16)(TEAM_NAME_GRID_X + column * TEAM_NAME_CELL_WIDTH),
                       (s16)(y + row * TEAM_NAME_CELL_HEIGHT), 8,
                       (u16)height, (u16)(layout->textureU + column * 8),
                       layout->textureV, 0, 0, 0, 0x244, 1, 1, 0x3B);
        }
    }
}

static void DrawEnteredTeamName(GameOrderingTableEntry *ot, s32 frame,
                                s32 length) {
    s32 y = SlideUp(TEAM_NAME_GRID_Y, frame, 0x178);
    s32 i;

    for (i = 0; i < length; i++) {
        DrawKeyboardCharacter(ot, g_TeamNameChars[i],
                              TEAM_NAME_GRID_X + i * TEAM_NAME_CELL_WIDTH, y,
                              frame * 2, 0x3B);
    }
}

void DrawTeamNameEntry(s32 step, s32 cursorIndex) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    s32 nameLength;
    s32 frame;
    s32 sine;

    if (step == 0) {
        g_TeamNameEntrySlide = 0;
        return;
    }

    nameLength = g_TeamNameLength;
    if (nameLength > MENU_TEAM_NAME_MAX_LENGTH) {
        nameLength = MENU_TEAM_NAME_MAX_LENGTH;
    }
    if ((u32)cursorIndex >= TEAM_NAME_GRID_CELL_COUNT) {
        cursorIndex = 0;
    }
    g_TeamNameEntrySlide = AddClampedMenuValue(
        g_TeamNameEntrySlide, 0, 0, TEAM_NAME_LAST_FRAME);
    if (step < 0) {
        g_TeamNameEntrySlide = AddClampedMenuValue(
            g_TeamNameEntrySlide, step, 0, TEAM_NAME_LAST_FRAME);
    }

    if (g_TeamNameEntrySlide >= TEAM_NAME_LAST_FRAME &&
        nameLength < MENU_TEAM_NAME_MAX_LENGTH) {
        DrawSprite(ot, nameLength * TEAM_NAME_CELL_WIDTH + 0x53, 0x7D,
                   TEAM_NAME_CELL_WIDTH, 0x18, 0xF4, 0x28, 0, 0, 0, 0x244,
                   1, 1, 0x39);
    }

    frame = ClampAnimationFrame(g_TeamNameEntrySlide - 0xE, 0xB);
    if (frame >= 0) {
        s32 y = SlideUp(0xFB, frame, 64);
        s32 phase = g_TeamNameCursorPhase & 0xFFF;

        sine = rsin(phase);
        if (sine < 0) {
            sine += 0x3F;
        }
        DrawSolidRect(ot + 1,
                      (cursorIndex % TEAM_NAME_GRID_COLUMNS) *
                                  TEAM_NAME_CELL_WIDTH +
                              0x54,
                      y + (cursorIndex / TEAM_NAME_GRID_COLUMNS) *
                              TEAM_NAME_CELL_HEIGHT,
                      0xB, frame * 2, 0, (sine >> 6) - 0x41, 0, 0xFF);
        g_TeamNameCursorPhase =
            (s32)((u32)g_TeamNameCursorPhase + 0x60u);
    }

    frame = ClampAnimationFrame(g_TeamNameEntrySlide - 0x11, 8);
    if (frame >= 0) {
        DrawKeyboardGrid(ot, frame, cursorIndex);
    }

    frame = ClampAnimationFrame(g_TeamNameEntrySlide - 0x13, 6);
    if (frame >= 0) {
        s32 y = SlideUp(0xF4, frame, 64);
        s32 height = frame * 2;

        DrawSprite(ot + 1, 0xDA, (s16)y, 0x1E, (u16)height, 0xAC, 0xE8, 0,
                   0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot + 1, 0xDA, (s16)(y + 0x17), 0x20, (u16)height, 0xAC,
                   0xF4, 0, 0, 0, 0x244, 1, 1, 0x3A);
        DrawSprite(ot + 1, 0xDA, (s16)(y + 0x23), 0x14, (u16)height, 0xCC,
                   0xF4, 0, 0, 0, 0x244, 1, 1, 0x3A);
    }

    frame = ClampAnimationFrame(g_TeamNameEntrySlide - 0x11, 8);
    if (frame >= 0) {
        DrawEnteredTeamName(ot, frame, nameLength);
    }

    if (step > 0) {
        g_TeamNameEntrySlide = AddClampedMenuValue(
            g_TeamNameEntrySlide, step, 0, TEAM_NAME_LAST_FRAME);
    }
}
