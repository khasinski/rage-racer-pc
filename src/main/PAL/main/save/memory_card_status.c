#include "game/memcard.h"
#include "game/menu.h"
#include <stdio.h>

s32 CalculateMemoryCardFreeBlocks(s32 fileCount) {
    s32 i;
    s32 usedBytes = 0;

    for (i = 0; i < fileCount; i++) {
        usedBytes += g_McDirEntries[i].size;
    }
    return 0xF - usedBytes / 0x2000;
}

s32 RefreshMemoryCardSaveStatus(s32 slot, GameSaveHeaderRow *header) {
    s32 ret;

    (void)slot;
    GameMenuLoadPhase = 0x100;
    ClearSaveHeaderRows(header);
    g_McCardFileCount = CountMemoryCardFiles(0, 0);
    g_McFreeBlocks = CalculateMemoryCardFreeBlocks(g_McCardFileCount);
    ret = ScanMemoryCardSaveHeaders(header);
    GameMenuLoadPhase = 0x200;

    return ret;
}

char *FormatSaveElapsedTime(char *dst, u32 seconds) {
    u32 hours = seconds / 216000;
    u32 totalMinutes = seconds / 3600;
    u32 totalSeconds = seconds / 60;

    sprintf(dst, g_FmtPlayTime, hours, totalMinutes - hours * 60,
            totalSeconds - totalMinutes * 60);
    return dst + 2;
}
