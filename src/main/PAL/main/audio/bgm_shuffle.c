#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/random.h"

/* Refills the BGM shuffle bag with a random permutation and rewinds it. */
void ShuffleBgmOrder(void) {
    s32 track;
    s32 slot;
    s32 emptyCount;
    s32 selectedEmpty;

    g_BgmTrackCount = ClampBgmShuffleCount(g_BgmTrackCount);

    for (slot = 0; slot < g_BgmTrackCount; slot++) {
        g_BgmShuffleOrder[slot] = 0xFF;
    }

    for (track = 0; track < g_BgmTrackCount; track++) {
        emptyCount = g_BgmTrackCount - track;
        selectedEmpty = ((Random15() & 0xFFF) % emptyCount) + 1;
        for (slot = 0; selectedEmpty != 0; slot++) {
            if (g_BgmShuffleOrder[slot] == 0xFF) {
                selectedEmpty--;
            }
        }
        g_BgmShuffleOrder[slot - 1] = (u8)track;
    }

    g_BgmShuffleIndex = 0;
}
