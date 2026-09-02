#include "game/memcard.h"
#include "game/menu.h"
#include "game/save_internal.h"

#include <string.h>

void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    s32 row;

    for (row = 0; row < MEMORY_CARD_SAVE_SLOT_COUNT; row++) {
        rows[row].fields.nameLength = 0;
        memset(rows[row].fields.name, 0, sizeof(rows[row].fields.name));
        rows[row].fields.saveCounter = 0;
        rows[row].halfwords[6] = 0;
        rows[row].fields.checksum = 0;
    }
}

void WriteSaveHeaderRow(GameSaveHeaderRow *row) {
    s32 i;

    row->fields.nameLength = g_TeamNameLength < SAVE_TEAM_NAME_CAPACITY
                                 ? g_TeamNameLength
                                 : SAVE_TEAM_NAME_CAPACITY;

    for (i = 0; i < SAVE_TEAM_NAME_CAPACITY; i++) {
        row->fields.name[i] = g_TeamNameChars[i];
    }

    row->fields.saveCounter = g_SaveElapsedTicks;
    row->fields.checksum = CalculateSaveHeaderChecksum(row);
}
