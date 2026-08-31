#include "game/memcard.h"
#include "game/menu.h"

void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    u8 *rowBytes = rows->bytes;
    s32 i = 0;
    s32 j;
    u8 *ptr2;
    GameSaveHeaderClearCursor *ptr3;
    GameSaveHeaderRowsAddress rowsAddress;
    GameSaveHeaderWordAddress clearAddress;

    rowsAddress.bytes = rowBytes;

    do {
        rowBytes[0] = 0;
        j = 5;
        ptr2 = rowBytes + 5;
        do {
            ptr2[1] = 0;
            ptr2--;
            j--;
        } while (j >= 0);

        clearAddress.bytes = &rowBytes[8];
        *clearAddress.word = 0;

        ptr3 = rowsAddress.clearCursors;
        ptr3->reservedHalfword = 0;

        clearAddress.bytes = &rowBytes[0x7C];
        *clearAddress.word = 0;
        rowBytes += 0x80;
        i++;
        rowsAddress.clearCursors++;
    } while (i < 3);
}

/* sprintf: every caller declares its own arity; keep it prototypeless. */

/* The icon strip is copied to VRAM in two passes: one 16x1 run of palette
 * entries at block+0x60, then the 4x16 frame tiles from block+0x80 on.  The
 * frame loop is written against the counter rather than against running
 * offsets because that is what the retail code's induction variables are:
 * `i * 0x80` and `i * 4` are strength-reduced back into the `addiu s4,s4,128`
 * / `addiu s1,s1,4` pair, and doing it this way puts the two increments'
 * initialisers after the loop-invariant `4` and `0x10` in the preheader,
 * which is the order retail schedules them in. */
void BuildSaveIconBlock(u8 *block, char *title, s32 iconTile, s32 imageX, s32 imageY) {
    Rect *iconRect;
    Rect *frameRect;
    s32 tileRow;
    s32 tileX;

    block[0] = 'S';
    block[1] = 'C';
    block[2] = 0x11;
    block[3] = 1;
    sprintf((char *)block + 4, g_FmtString, title);

    tileRow = iconTile / 20;
    iconRect = &g_SaveIconRect;
    g_SaveIconRect.w = 0x10;
    g_SaveIconRect.h = 1;
    tileX = iconTile % 20;
    iconRect->x = tileX * 16;
    g_SaveIconRect.y = tileRow + 0x1E0;
    StoreImage(iconRect, block + 0x60);
    DrawSync(0);

    frameRect = iconRect;
    frameRect->x = imageX;
    frameRect->y = imageY;
    frameRect->w = 4;
    frameRect->h = 0x10;
    StoreImage(frameRect, block + 0x80);
    DrawSync(0);
}

void WriteSaveHeaderRow(GameSaveHeaderRow *row) {
    s32 i;
    u32 checksumIndex;
    u32 checksum;
    u16 *scan;

    row->fields.nameLength = g_TeamNameLength;

    for (i = 0; i < 7; i++) {
        row->fields.name[i] = g_TeamNameChars[i];
    }

    i = 0;
    checksum = 0;
    row->fields.saveCounter = g_SaveElapsedTicks;
    scan = row->halfwords;

    do {
        checksum += *scan++;
        i++;
        checksumIndex = i;
    } while (checksumIndex < 0x3E);

    checksum = ~checksum;
    row->fields.checksum = checksum;
}
