#include "game/memcard.h"
#include "game/menu.h"

#include <string.h>

static u32 CalculateSaveHeaderChecksum(const GameSaveHeaderRow *row) {
    const u8 *bytes = row->bytes;
    u32 sum = 0;
    s32 offset;

    for (offset = 0; offset < 0x7C; offset += 2) {
        sum += (u32)bytes[offset] | ((u32)bytes[offset + 1] << 8);
    }
    return ~sum;
}

void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    s32 row;

    for (row = 0; row < 3; row++) {
        memset(rows[row].bytes, 0, 7);
        rows[row].fields.saveCounter = 0;
        rows[row].halfwords[6] = 0;
        rows[row].fields.checksum = 0;
    }
}

void WriteSaveHeaderRow(GameSaveHeaderRow *row) {
    s32 i;

    row->fields.nameLength = g_TeamNameLength;

    for (i = 0; i < 7; i++) {
        row->fields.name[i] = g_TeamNameChars[i];
    }

    row->fields.saveCounter = g_SaveElapsedTicks;
    row->fields.checksum = CalculateSaveHeaderChecksum(row);
}
