#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"

enum {
    SAVE_ROW_TEXT_SIZE = 16,
    SAVE_ROW_VISIBLE_NAME_LENGTH = 6,
};

static const char s_saveNameCharacters[] =
    "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ.-!?@";

static char DecodeSaveNameCharacter(u8 characterIndex) {
    if (characterIndex >= SAVE_NAME_CHARACTER_COUNT) return '?';
    return s_saveNameCharacters[characterIndex];
}

static void DrawSaveRowSlotNumber(char *text, const char *format,
                                  s32 slotNumber, s32 y) {
    snprintf(text, SAVE_ROW_TEXT_SIZE, format, slotNumber);
    DrawLargeText(0x48, y, text, 0x7F, 0x7F, 0x7F, 0x244, 0xA0);
}

static void DrawUsedSaveRow(char *text, s32 slotNumber, s32 y,
                            const GameSaveHeaderRow *row) {
    s32 i;

    DrawSaveRowSlotNumber(text, "%1d /", slotNumber, y);
    for (i = 0; i < SAVE_ROW_VISIBLE_NAME_LENGTH; i++) {
        text[i] = i < row->fields.nameLength
                      ? DecodeSaveNameCharacter(row->fields.name[i])
                      : ' ';
    }
    snprintf(text + SAVE_ROW_VISIBLE_NAME_LENGTH,
             SAVE_ROW_TEXT_SIZE - SAVE_ROW_VISIBLE_NAME_LENGTH,
             "%s",
             " /");
    DrawLargeText(0x68, y, text, 0x7F, 0x7F, 0x7F, 0x244, 0xA0);
    DrawLargeText(
        0xB0,
        y,
        FormatSaveElapsedTime(text, row->fields.saveCounter),
        0x7F,
        0x7F,
        0x7F,
        0x244,
        0xA0);
}

void DrawMemoryCardSaveRows(s32 flags, GameSaveHeaderRow *rows) {
    char text[SAVE_ROW_TEXT_SIZE];
    s32 rowIndex;

    for (rowIndex = 0; rowIndex < MEMORY_CARD_SAVE_SLOT_COUNT; rowIndex++) {
        s32 slotNumber = rowIndex + 1;
        s32 y = 0xD8 + rowIndex * 0x30;
        s32 used = (flags & (1 << rowIndex)) != 0;
        s32 error = (flags & (0x10000 << rowIndex)) != 0;

        if (used) {
            DrawUsedSaveRow(text, slotNumber, y, &rows[rowIndex]);
        } else if (error) {
            DrawSaveRowSlotNumber(text, "%1d /", slotNumber, y);
            DrawLargeText(
                0x88, y, "FILE ERROR", 0x7F, 0x7F, 0x7F, 0x244, 0xA0);
        } else if (g_McFreeBlocks == 0 && g_McMenuPage != 0 &&
                   g_McMenuRowCursor != 0) {
            DrawSaveRowSlotNumber(text, "%1d /", slotNumber, y);
            DrawLargeText(
                0x90, y, "NO FILE", 0x7F, 0x7F, 0x7F, 0x244, 0xA0);
        } else if (g_McFreeBlocks == 0 || g_McMenuPage == 0) {
            DrawSaveRowSlotNumber(text, "%1d /        /", slotNumber, y);
        } else {
            const char *slotLabel =
                g_McMenuRowCursor == 0 ? "NEW FILE" : "NO FILE";

            DrawSaveRowSlotNumber(text, "%1d /", slotNumber, y);
            DrawLargeText(
                0x90, y, slotLabel, 0x7F, 0x7F, 0x7F, 0x244, 0xA0);
        }
    }
}
