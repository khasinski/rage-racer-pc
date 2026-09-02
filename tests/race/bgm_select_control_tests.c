#include "common.h"
#include "game/menu.h"
#include "game/race.h"

#include <stdio.h>

s32 g_BgmChangeDelay;
s32 g_BgmRandomLabelTimer;
s32 g_BgmRandomPlay;
s32 g_BgmSelectCdTrack;
s32 g_BgmSelectCursor;
s32 g_BgmSelectShowUi;
s32 g_BgmSelectTrack;
s32 g_BgmShuffleIndex;
u8 g_BgmShuffleOrder[16];
s32 g_BgmTrackCount;
s32 g_CdTrackEnded;
s32 g_FadeStep;
u16 g_PadPressed;

static s32 s_cdRequest;
static s32 s_cdStarts;
static s32 s_fadeCalls;
static s32 s_shuffleCalls;

void RequestCdTrack(s32 track) { s_cdRequest = track; }
void StartCdAudio(void) { s_cdStarts++; }
void StartCdVolumeFade(s32 frames) {
    if (frames == 60) {
        s_fadeCalls++;
    }
}
void ShuffleBgmOrder(void) {
    s_shuffleCalls++;
    g_BgmShuffleOrder[0] = 1;
    g_BgmShuffleOrder[1] = 0;
    g_BgmShuffleOrder[2] = 2;
    g_BgmShuffleIndex = 0;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

static void Reset(void) {
    g_BgmChangeDelay = 0;
    g_BgmRandomLabelTimer = 0;
    g_BgmRandomPlay = 0;
    g_BgmSelectCdTrack = 3;
    g_BgmSelectCursor = 1;
    g_BgmSelectShowUi = 1;
    g_BgmSelectTrack = 0;
    g_BgmShuffleIndex = 0;
    g_BgmShuffleOrder[0] = 0;
    g_BgmShuffleOrder[1] = 1;
    g_BgmShuffleOrder[2] = 2;
    g_BgmTrackCount = 3;
    g_CdTrackEnded = 0;
    g_FadeStep = 0;
    g_PadPressed = 0;
    s_cdRequest = -1;
    s_cdStarts = 0;
    s_fadeCalls = 0;
    s_shuffleCalls = 0;
}

int main(void) {
    s32 frame;

    Reset();
    g_BgmTrackCount = 10;
    g_BgmSelectTrack = 8;
    g_CdTrackEnded = 1;
    UpdateBgmSelectPlayback();
    CHECK(g_BgmSelectTrack == 9 && g_BgmSelectCdTrack == 17);
    CHECK(g_BgmChangeDelay == 6 && s_cdStarts == 0);
    for (frame = 0; frame < 6; frame++) {
        UpdateBgmSelectPlayback();
    }
    CHECK(g_BgmChangeDelay == 0 && g_BgmSelectCdTrack == 17);
    CHECK(s_cdRequest == 17 && s_cdStarts == 1 && g_CdTrackEnded == 0);

    Reset();
    g_BgmShuffleIndex = 2;
    AdvanceBgmShuffleBag(1);
    CHECK(s_shuffleCalls == 1 && g_BgmShuffleIndex == 0);
    CHECK(g_BgmShuffleOrder[0] == 2 && g_BgmShuffleOrder[2] == 1);

    Reset();
    g_BgmTrackCount = 0;
    g_BgmShuffleIndex = 8;
    AdvanceBgmShuffleBag(1);
    CHECK(g_BgmShuffleIndex == 0 && s_shuffleCalls == 0);

    Reset();
    g_BgmShuffleIndex = 8;
    AdvanceBgmShuffleBag(1);
    CHECK(g_BgmShuffleIndex == 0 && s_shuffleCalls == 1);

    Reset();
    g_BgmSelectCursor = 0;
    g_BgmSelectTrack = 0;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput();
    CHECK(g_BgmSelectTrack == 2 && g_BgmSelectCdTrack == 5);
    CHECK(g_BgmChangeDelay == 0x40 && s_fadeCalls == 1);

    Reset();
    g_BgmSelectCursor = 2;
    g_BgmSelectTrack = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput();
    CHECK(g_BgmSelectTrack == 0 && g_BgmSelectCdTrack == 3);

    Reset();
    g_BgmSelectTrack = 1;
    g_PadPressed = PAD_L2;
    UpdateBgmSelectInput();
    CHECK(g_BgmRandomPlay == 1 && g_BgmRandomLabelTimer == 60);
    CHECK(g_BgmShuffleOrder[0] == 2 && g_BgmShuffleOrder[2] == 1);

    Reset();
    g_BgmSelectCursor = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput();
    CHECK(g_FadeStep == 4 && s_fadeCalls == 1);

    Reset();
    g_BgmSelectCursor = 1;
    g_PadPressed = PAD_LEFT | PAD_L1 | PAD_R1;
    UpdateBgmSelectInput();
    CHECK(g_BgmSelectCursor == 0 && g_BgmSelectShowUi == 0);

    puts("BGM selector preserves playback delay, shuffle, and input");
    return 0;
}
