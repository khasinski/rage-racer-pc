#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/random.h"

enum {
    EMPTY_BGM_SHUFFLE_SLOT = 0xFF,
    BGM_RANDOM_VALUE_MASK = 0xFFF,
    FIRST_EMPTY_SLOT_ORDINAL = 1,
};

/* Refills the BGM shuffle bag with a random permutation and rewinds it. */
void ShuffleBgmOrder(void) {
    s32 track;
    s32 slot;
    s32 emptyCount;
    s32 selectedEmpty;

    g_BgmTrackCount = ClampBgmTrackCount(g_BgmTrackCount);

    for (slot = 0; slot < g_BgmTrackCount; slot++) {
        g_BgmShuffleOrder[slot] = EMPTY_BGM_SHUFFLE_SLOT;
    }

    for (track = 0; track < g_BgmTrackCount; track++) {
        emptyCount = g_BgmTrackCount - track;
        selectedEmpty =
            ((Random15() & BGM_RANDOM_VALUE_MASK) % emptyCount) +
            FIRST_EMPTY_SLOT_ORDINAL;
        for (slot = 0; selectedEmpty != 0; slot++) {
            if (g_BgmShuffleOrder[slot] == EMPTY_BGM_SHUFFLE_SLOT) {
                selectedEmpty--;
            }
        }
        g_BgmShuffleOrder[slot - 1] = (u8)track;
    }

    g_BgmShuffleIndex = 0;
}
