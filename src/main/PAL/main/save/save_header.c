#include "game/memcard.h"
#include "game/menu.h"

#include <string.h>

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
