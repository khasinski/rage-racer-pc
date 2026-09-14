#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include <stdio.h>

s32 CalculateMemoryCardFreeBlocks(const DirEntry *entries, s32 fileCount) {
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
        if (entries[i].size > 0) {
            usedBytes += (u32)entries[i].size;
        }
        if (usedBytes >= capacity) {
            return 0;
        }
    }
    return MEMORY_CARD_BLOCK_COUNT -
           (s32)(usedBytes / MEMORY_CARD_BLOCK_SIZE);
}

s32 RefreshMemoryCardSaveStatus(GameSaveHeaderRow *header, s32 *freeBlocks) {
    DirEntry entries[MEMORY_CARD_MAX_FILES];
    const s32 fileCount = CountMemoryCardFiles(0, 0, entries);
    s32 ret;

    GameMenuLoadPhase = 0x100;
    ClearSaveHeaderRows(header);
    *freeBlocks = CalculateMemoryCardFreeBlocks(entries, fileCount);
    ret = ScanMemoryCardSaveHeaders(header);
    GameMenuLoadPhase = 0x200;

    return ret;
}

char *FormatSaveElapsedTime(char dst[SAVE_ELAPSED_TIME_CAPACITY], u32 ticks) {
    enum {
        TICKS_PER_SECOND = 60,
        TICKS_PER_MINUTE = 60 * TICKS_PER_SECOND,
        TICKS_PER_HOUR = 60 * TICKS_PER_MINUTE,
        PLAY_TIME_HIDDEN_PADDING = 2,
    };
    u32 hours = ticks / TICKS_PER_HOUR;
    u32 totalMinutes = ticks / TICKS_PER_MINUTE;
    u32 totalSeconds = ticks / TICKS_PER_SECOND;

    snprintf(dst, SAVE_ELAPSED_TIME_CAPACITY, "%5d:%02d:%02d", (s32)hours,
             (s32)(totalMinutes - hours * 60),
             (s32)(totalSeconds - totalMinutes * 60));
    return dst + PLAY_TIME_HIDDEN_PADDING;
}
