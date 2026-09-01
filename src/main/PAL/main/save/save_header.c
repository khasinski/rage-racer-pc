#include "game/memcard.h"
#include "game/menu.h"
#include "game/save_internal.h"

#include <string.h>

void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    s32 row;

    for (row = 0; row < MEMORY_CARD_SAVE_SLOT_COUNT; row++) {
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
