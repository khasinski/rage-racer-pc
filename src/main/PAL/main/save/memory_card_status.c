#include "game/memcard.h"
#include "game/menu.h"
#include <stdio.h>

s32 CalculateMemoryCardFreeBlocks(s32 fileCount) {
    s32 i;
    uint64_t usedBytes = 0;
    uint64_t capacity = MEMORY_CARD_BLOCK_COUNT * MEMORY_CARD_BLOCK_SIZE;

    if (fileCount < 0) {
        fileCount = 0;
    }
    if (fileCount > MEMORY_CARD_MAX_FILES) {
        fileCount = MEMORY_CARD_MAX_FILES;
    }
    for (i = 0; i < fileCount; i++) {
        if (g_McDirEntries[i].size > 0) {
            usedBytes += (u32)g_McDirEntries[i].size;
        }
        if (usedBytes >= capacity) {
            return 0;
        }
    }
    return MEMORY_CARD_BLOCK_COUNT -
           (s32)(usedBytes / MEMORY_CARD_BLOCK_SIZE);
}

s32 RefreshMemoryCardSaveStatus(GameSaveHeaderRow *header) {
    s32 ret;

    GameMenuLoadPhase = 0x100;
    ClearSaveHeaderRows(header);
    g_McCardFileCount = CountMemoryCardFiles(0, 0);
    g_McFreeBlocks = CalculateMemoryCardFreeBlocks(g_McCardFileCount);
    ret = ScanMemoryCardSaveHeaders(header);
    GameMenuLoadPhase = 0x200;

    return ret;
}

char *FormatSaveElapsedTime(char *dst, u32 ticks) {
    enum {
        TICKS_PER_SECOND = 60,
        TICKS_PER_MINUTE = 60 * TICKS_PER_SECOND,
        TICKS_PER_HOUR = 60 * TICKS_PER_MINUTE,
        PLAY_TIME_HIDDEN_PADDING = 2,
    };
    u32 hours = ticks / TICKS_PER_HOUR;
    u32 totalMinutes = ticks / TICKS_PER_MINUTE;
    u32 totalSeconds = ticks / TICKS_PER_SECOND;

    sprintf(dst, g_FmtPlayTime, (s32)hours,
            (s32)(totalMinutes - hours * 60),
            (s32)(totalSeconds - totalMinutes * 60));
    return dst + PLAY_TIME_HIDDEN_PADDING;
}
