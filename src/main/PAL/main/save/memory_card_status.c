#include "common.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/render.h"

s32 CalculateMemoryCardFreeBlocks(s32 port) {
    s32 i;
    s32 sum;
    DirEntry *ptr;
    s32 value;

    i = 0;
    sum = 0;

    if (port > 0) {
        ptr = g_McDirEntries;
        do {
            value = ptr->size;
            sum += value;
            ptr++;
        } while (++i < port);
    }

    {
        s32 biased;

        biased = sum;
        if (sum < 0) {
            biased = sum + 0x1FFF;
        }
        sum = biased >> 13;

        return 0xF - sum;
    }
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

/* sprintf: every caller declares its own arity; keep it prototypeless. */

char *FormatSaveElapsedTime(char *dst, u32 seconds) {
    u32 hours = seconds / 216000;
    u32 totalMinutes = seconds / 3600;
    u32 totalSeconds = seconds / 60;

    sprintf(dst, g_FmtPlayTime, hours, totalMinutes - (hours * 60), totalSeconds - (totalMinutes * 60));
    return dst + 2;
}

void DrawMemoryCardMessageLine(s32 unused, s32 messageIndex) {
    (void)unused;
    DrawText8x8(0x28, 0xB8, &g_McMessageText[messageIndex * 30], 0x78CC);
}

void DrawMemoryCardHelpPrompt(s32 page) {
    s32 i;

    i = page * 0x3C;
    DrawText8x8(0x50, 0x28, &g_McHelpText[i], 0x78CC);
    DrawText8x8(0x50, 0x40, &g_McHelpText[i + 0x1E], 0x78CC);
}
