#include "common.h"
#include "game/memcard.h"
#include "game/menu.h"

#include <stdio.h>
#include <string.h>

u8 g_TeamNameLength;
u8 g_TeamNameChars[8];
s32 g_SaveElapsedTicks;

static s32 s_failures;

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestClearRows(void) {
    GameSaveHeaderRow rows[3];
    s32 row;
    s32 byte;

    memset(rows, 0xA5, sizeof(rows));
    ClearSaveHeaderRows(rows);
    for (row = 0; row < 3; row++) {
        for (byte = 0; byte < 0x80; byte++) {
            s32 cleared = byte < 8 || (byte >= 8 && byte < 14) || byte >= 0x7C;
            u8 expected = cleared ? 0 : 0xA5;
            if (rows[row].bytes[byte] != expected) {
                printf("FAIL clear row %d byte %d: got %02x expected %02x\n",
                       row, byte, rows[row].bytes[byte], expected);
                s_failures++;
            }
        }
    }
}

static void TestWriteRow(void) {
    GameSaveHeaderRow row;
    u32 sum = 0;
    s32 i;

    memset(&row, 0x3C, sizeof(row));
    g_TeamNameLength = 7;
    memcpy(g_TeamNameChars, "RAGE123", 7);
    g_SaveElapsedTicks = 0x12345678;
    WriteSaveHeaderRow(&row);

    Check(row.fields.nameLength == 7, "save name length");
    Check(memcmp(row.fields.name, "RAGE123", 7) == 0, "save team name");
    Check(row.fields.saveCounter == 0x12345678, "save elapsed ticks");
    for (i = 0; i < 0x3E; i++) sum += row.halfwords[i];
    Check(row.fields.checksum == ~sum, "save header checksum");

    g_TeamNameLength = 0xFF;
    WriteSaveHeaderRow(&row);
    Check(row.fields.nameLength == SAVE_TEAM_NAME_CAPACITY,
          "save name length clamps to the stored field");
}

int main(void) {
    TestClearRows();
    TestWriteRow();
    if (s_failures != 0) return 1;
    puts("save headers clear their status fields and checksum their contents");
    return 0;
}
