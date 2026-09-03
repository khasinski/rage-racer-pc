#include "game/audio_internal.h"
#include "game/menu.h"

#include <stdio.h>
#include <string.h>

s32 g_BgmShuffleIndex;
u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];
s32 g_BgmTrackCount;

static u32 s_randomState;

s32 Random15(void) {
    s_randomState = s_randomState * 1103515245u + 12345u;
    return (s32)((s_randomState >> 16) & 0x7FFF);
}

static int IsPermutation(s32 count) {
    u8 seen[BGM_SHUFFLE_CAPACITY] = {0};
    s32 slot;

    for (slot = 0; slot < count; slot++) {
        u8 track = g_BgmShuffleOrder[slot];

        if (track >= count || seen[track] != 0) {
            return 0;
        }
        seen[track] = 1;
    }
    return 1;
}

int main(void) {
    s32 count;

    for (count = 1; count <= BGM_PLAYABLE_TRACK_COUNT; count++) {
        memset(g_BgmShuffleOrder, 0xA5, sizeof(g_BgmShuffleOrder));
        g_BgmTrackCount = count;
        g_BgmShuffleIndex = 7;
        s_randomState = (u32)count;
        ShuffleBgmOrder();
        if (g_BgmShuffleIndex != 0 || !IsPermutation(count) ||
            (count < BGM_PLAYABLE_TRACK_COUNT &&
             g_BgmShuffleOrder[count] != 0xA5)) {
            fprintf(stderr, "invalid shuffle for %d tracks\n", count);
            return 1;
        }
    }

    g_BgmTrackCount = 0;
    g_BgmShuffleIndex = 4;
    ShuffleBgmOrder();
    if (g_BgmTrackCount != 0 || g_BgmShuffleIndex != 0) {
        puts("empty shuffle was not reset safely");
        return 1;
    }

    g_BgmTrackCount = BGM_SHUFFLE_CAPACITY + 5;
    memset(g_BgmShuffleOrder, 0xA5, sizeof(g_BgmShuffleOrder));
    ShuffleBgmOrder();
    if (g_BgmTrackCount != BGM_PLAYABLE_TRACK_COUNT ||
        !IsPermutation(BGM_PLAYABLE_TRACK_COUNT) ||
        g_BgmShuffleOrder[BGM_PLAYABLE_TRACK_COUNT] != 0xA5) {
        puts("oversized shuffle was not bounded");
        return 1;
    }

    g_BgmTrackCount = -3;
    ShuffleBgmOrder();
    if (g_BgmTrackCount != 0 || g_BgmShuffleIndex != 0) {
        puts("negative shuffle size was not bounded");
        return 1;
    }

    puts("BGM shuffle tests passed");
    return 0;
}
