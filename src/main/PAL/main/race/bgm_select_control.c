#include "game/audio_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"

enum {
    BGM_CHANGE_DELAY_AUTO = 6,
    BGM_CHANGE_DELAY_MANUAL = 0x40,
    BGM_VOLUME_FADE_FRAMES = 60,
    BGM_RANDOM_LABEL_FRAMES = 60,
    BGM_SELECT_EXIT_FADE_STEP = 4,
    BGM_SELECT_FIRST_OPTION = 0,
    BGM_SELECT_LAST_OPTION = 2,
};

static void PreventImmediateShuffleRepeat(u32 track) {
    s32 trackCount = ClampBgmTrackCount(g_BgmTrackCount);

    if (trackCount <= 1) {
        return;
    }

    if (track == g_BgmShuffleOrder[0]) {
        u8 replacement = g_BgmShuffleOrder[trackCount - 1];

        g_BgmShuffleOrder[0] = replacement;
        g_BgmShuffleOrder[trackCount - 1] = (u8)track;
    }
}

void AdvanceBgmShuffleBag(u32 track) {
    s32 trackCount = ClampBgmTrackCount(g_BgmTrackCount);

    g_BgmTrackCount = trackCount;
    if (trackCount == 0) {
        g_BgmShuffleIndex = 0;
        return;
    }

    g_BgmShuffleIndex = WrapBgmTrackIndex(g_BgmShuffleIndex, trackCount) + 1;
    if (g_BgmShuffleIndex >= trackCount) {
        ShuffleBgmOrder();
        PreventImmediateShuffleRepeat(track);
    }
}

static void SelectNextBgmTrack(BgmSelect *state) {
    if (state->randomPlay != 0 && g_BgmTrackCount > 0) {
        state->track = BgmShuffleTrackAt(
            g_BgmShuffleOrder, g_BgmTrackCount, g_BgmShuffleIndex);
        AdvanceBgmShuffleBag((u32)state->track);
    } else {
        state->track = WrapBgmTrackIndex(
            WrapBgmTrackIndex(state->track, g_BgmTrackCount) + 1,
            g_BgmTrackCount);
    }
    state->cdTrack = BgmCdTrack(state->track);
}

static void BeginManualTrackChange(BgmSelect *state) {
    if (state->changeDelay == 0) {
        StartCdVolumeFade(BGM_VOLUME_FADE_FRAMES);
        state->changeDelay = BGM_CHANGE_DELAY_MANUAL;
    }
    state->cdTrack = BgmCdTrack(state->track);
}

void UpdateBgmSelectPlayback(BgmSelect *state) {
    g_BgmTrackCount = ClampBgmTrackCount(g_BgmTrackCount);

    if (state->changeDelay > 0) {
        state->changeDelay--;
        if (state->changeDelay == 0) {
            RequestCdTrack(state->cdTrack);
            StartCdAudio();
            g_CdTrackEnded = 0;
        }
    } else if (g_CdTrackEnded != 0) {
        state->changeDelay = BGM_CHANGE_DELAY_AUTO;
        SelectNextBgmTrack(state);
    }
}

static void EnableRandomPlay(BgmSelect *state) {
    ShuffleBgmOrder();
    PreventImmediateShuffleRepeat((u32)state->track);
    state->randomPlay = 1;
    state->labelTimer = BGM_RANDOM_LABEL_FRAMES;
}

static void ExitBgmSelectScreen(void) {
    StartCdVolumeFade(BGM_VOLUME_FADE_FRAMES);
    g_FadeStep = BGM_SELECT_EXIT_FADE_STEP;
}

void UpdateBgmSelectInput(BgmSelect *state) {
    u16 buttons = g_PadPressed;

    g_BgmTrackCount = ClampBgmTrackCount(g_BgmTrackCount);
    if (state->cursor < BGM_SELECT_FIRST_OPTION) {
        state->cursor = BGM_SELECT_FIRST_OPTION;
    } else if (state->cursor > BGM_SELECT_LAST_OPTION) {
        state->cursor = BGM_SELECT_LAST_OPTION;
    }

    if ((buttons & PAD_LEFT) &&
        state->cursor > BGM_SELECT_FIRST_OPTION) {
        state->cursor--;
    }
    if ((buttons & PAD_RIGHT) &&
        state->cursor < BGM_SELECT_LAST_OPTION) {
        state->cursor++;
    }
    if (buttons & PAD_L2) {
        EnableRandomPlay(state);
    }
    if (buttons & PAD_R2) {
        state->randomPlay = 0;
        state->labelTimer = 0;
    }

    if (buttons & PAD_CONFIRM) {
        switch (state->cursor) {
        case 0:
            if (state->randomPlay == 0) {
                state->track = WrapBgmTrackIndex(
                    WrapBgmTrackIndex(state->track, g_BgmTrackCount) - 1,
                    g_BgmTrackCount);
            }
            BeginManualTrackChange(state);
            break;
        case 1:
            ExitBgmSelectScreen();
            break;
        case 2:
            SelectNextBgmTrack(state);
            BeginManualTrackChange(state);
            break;
        }
    } else if (buttons & PAD_CANCEL) {
        ExitBgmSelectScreen();
    }

    if (buttons & PAD_L1) {
        state->showUi = 1;
    }
    if (buttons & PAD_R1) {
        state->showUi = 0;
    }
}
