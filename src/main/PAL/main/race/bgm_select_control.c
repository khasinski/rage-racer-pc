#include "game/audio_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"

enum {
    BGM_CHANGE_DELAY_AUTO = 6,
    BGM_CHANGE_DELAY_MANUAL = 0x40,
    BGM_RANDOM_LABEL_FRAMES = 60,
};

static void PreventImmediateShuffleRepeat(u32 track) {
    if (g_BgmTrackCount <= 1) {
        return;
    }

    if (track == g_BgmShuffleOrder[0]) {
        u8 replacement = g_BgmShuffleOrder[g_BgmTrackCount - 1];

        g_BgmShuffleOrder[0] = replacement;
        g_BgmShuffleOrder[g_BgmTrackCount - 1] = (u8)track;
    }
}

void AdvanceBgmShuffleBag(u32 track) {
    if (g_BgmTrackCount <= 0) {
        g_BgmShuffleIndex = 0;
        return;
    }

    g_BgmShuffleIndex++;
    if (g_BgmShuffleIndex >= g_BgmTrackCount) {
        ShuffleBgmOrder();
        PreventImmediateShuffleRepeat(track);
    }
}

static void SelectNextBgmTrack(void) {
    if (g_BgmRandomPlay != 0 && g_BgmTrackCount > 0) {
        g_BgmSelectTrack = g_BgmShuffleOrder[g_BgmShuffleIndex];
        AdvanceBgmShuffleBag((u32)g_BgmSelectTrack);
    } else {
        g_BgmSelectTrack =
            WrapBgmTrackIndex(g_BgmSelectTrack + 1, g_BgmTrackCount);
    }
    g_BgmSelectCdTrack = BgmCdTrack(g_BgmSelectTrack);
}

static void BeginManualTrackChange(void) {
    if (g_BgmChangeDelay == 0) {
        StartCdVolumeFade(60);
        g_BgmChangeDelay = BGM_CHANGE_DELAY_MANUAL;
    }
    g_BgmSelectCdTrack = BgmCdTrack(g_BgmSelectTrack);
}

void UpdateBgmSelectPlayback(void) {
    if (g_BgmChangeDelay > 0) {
        g_BgmChangeDelay--;
        if (g_BgmChangeDelay == 0) {
            RequestCdTrack(g_BgmSelectCdTrack);
            StartCdAudio();
            g_CdTrackEnded = 0;
        }
    } else if (g_CdTrackEnded != 0) {
        g_BgmChangeDelay = BGM_CHANGE_DELAY_AUTO;
        SelectNextBgmTrack();
    }
}

static void EnableRandomPlay(void) {
    ShuffleBgmOrder();
    PreventImmediateShuffleRepeat((u32)g_BgmSelectTrack);
    g_BgmRandomPlay = 1;
    g_BgmRandomLabelTimer = BGM_RANDOM_LABEL_FRAMES;
}

static void ExitBgmSelectScreen(void) {
    StartCdVolumeFade(60);
    g_FadeStep = 4;
}

void UpdateBgmSelectInput(void) {
    u16 buttons = g_PadPressed;

    if ((buttons & PAD_LEFT) && g_BgmSelectCursor > 0) {
        g_BgmSelectCursor--;
    }
    if ((buttons & PAD_RIGHT) && g_BgmSelectCursor < 2) {
        g_BgmSelectCursor++;
    }
    if (buttons & PAD_L2) {
        EnableRandomPlay();
    }
    if (buttons & PAD_R2) {
        g_BgmRandomPlay = 0;
        g_BgmRandomLabelTimer = 0;
    }

    if (buttons & PAD_CONFIRM) {
        switch (g_BgmSelectCursor) {
        case 0:
            if (g_BgmRandomPlay == 0) {
                g_BgmSelectTrack = WrapBgmTrackIndex(
                    g_BgmSelectTrack - 1, g_BgmTrackCount);
            }
            BeginManualTrackChange();
            break;
        case 1:
            ExitBgmSelectScreen();
            break;
        case 2:
            SelectNextBgmTrack();
            BeginManualTrackChange();
            break;
        }
    } else if (buttons & PAD_CANCEL) {
        ExitBgmSelectScreen();
    }

    if (buttons & PAD_L1) {
        g_BgmSelectShowUi = 1;
    }
    if (buttons & PAD_R1) {
        g_BgmSelectShowUi = 0;
    }
}
