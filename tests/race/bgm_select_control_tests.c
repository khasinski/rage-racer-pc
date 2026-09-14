#include "common.h"
#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"

#include <limits.h>
#include <stdio.h>

static BgmSelect s_state;
s32 g_BgmShuffleIndex;
u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];
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
    s_state = (BgmSelect){.cdTrack = 3, .cursor = 1, .showUi = 1};
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
    s_state.track = 8;
    g_CdTrackEnded = 1;
    UpdateBgmSelectPlayback(&s_state);
    CHECK(s_state.track == 9 && s_state.cdTrack == 17);
    CHECK(s_state.changeDelay == 6 && s_cdStarts == 0);
    for (frame = 0; frame < 6; frame++) {
        UpdateBgmSelectPlayback(&s_state);
    }
    CHECK(s_state.changeDelay == 0 && s_state.cdTrack == 17);
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
    g_BgmTrackCount = INT_MAX;
    g_BgmShuffleIndex = INT_MAX;
    AdvanceBgmShuffleBag(1);
    CHECK(g_BgmTrackCount == BGM_PLAYABLE_TRACK_COUNT);
    CHECK(g_BgmShuffleIndex == 8 && s_shuffleCalls == 0);

    Reset();
    s_state.randomPlay = 1;
    g_BgmShuffleIndex = INT_MIN;
    g_CdTrackEnded = 1;
    UpdateBgmSelectPlayback(&s_state);
    CHECK(s_state.track == 1 && g_BgmShuffleIndex == 2);

    Reset();
    s_state.randomPlay = 1;
    g_BgmShuffleOrder[0] = 0xFF;
    g_CdTrackEnded = 1;
    UpdateBgmSelectPlayback(&s_state);
    CHECK(s_state.track == 0);

    Reset();
    s_state.track = INT_MAX;
    g_CdTrackEnded = 1;
    UpdateBgmSelectPlayback(&s_state);
    CHECK(s_state.track == 2);

    Reset();
    s_state.cursor = 0;
    s_state.track = 0;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.track == 2 && s_state.cdTrack == 5);
    CHECK(s_state.changeDelay == 0x40 && s_fadeCalls == 1);

    Reset();
    s_state.cursor = 2;
    s_state.track = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.track == 0 && s_state.cdTrack == 3);

    Reset();
    s_state.track = 1;
    g_PadPressed = PAD_L2;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.randomPlay == 1 && s_state.labelTimer == 60);
    CHECK(g_BgmShuffleOrder[0] == 2 && g_BgmShuffleOrder[2] == 1);

    Reset();
    s_state.cursor = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateBgmSelectInput(&s_state);
    CHECK(g_FadeStep == 4 && s_fadeCalls == 1);

    Reset();
    s_state.cursor = 1;
    g_PadPressed = PAD_LEFT | PAD_L1 | PAD_R1;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.cursor == 0 && s_state.showUi == 0);

    Reset();
    s_state.cursor = INT_MAX;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.cursor == 2);

    Reset();
    s_state.cursor = INT_MIN;
    g_BgmTrackCount = INT_MAX;
    UpdateBgmSelectInput(&s_state);
    CHECK(s_state.cursor == 0);
    CHECK(g_BgmTrackCount == BGM_PLAYABLE_TRACK_COUNT);

    puts("BGM selector preserves playback delay, shuffle, and input");
    return 0;
}
