#include "common.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
s16 g_GrandPrixMode;
s16 g_RaceCueDelay;
s16 g_RivalCueEnabled;
s16 g_WrongWayTimer;
s32 g_TrackLength;

static s32 s_cues[8];
static s32 s_cueCount;
static s32 s_failures;

void PlaySoundCue(s32 cue) {
    s_cues[s_cueCount++] = cue;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestCountdownCues(void) {
    static const s32 frames[] = {0x79, 0x97, 0xB5, 0xD3};
    static const s32 cues[] = {0x1E, 0x1F, 0x20, 0x21};
    s32 i;

    s_cueCount = 0;
    for (i = 0; i < 4; i++) {
        PlayCountdownCues(frames[i] - 1);
        PlayCountdownCues(frames[i]);
        Check(s_cueCount == i + 1, "countdown cue count");
        Check(s_cues[i] == cues[i], "countdown cue number");
    }

    g_GrandPrixMode = 0;
    g_RaceCueDelay = 0;
    PlayCountdownCues(0x10F);
    Check(s_cueCount == 4, "time attack has no start voice");
    Check(g_RaceCueDelay == 0, "time attack leaves cue delay alone");

    g_GrandPrixMode = 1;
    PlayCountdownCues(0x10F);
    Check(s_cueCount == 5 && s_cues[4] == 0x22,
          "Grand Prix start voice");
    Check(g_RaceCueDelay == 0x5A, "Grand Prix lap cue delay");
}

static void UpdateGateAt(s32 progress) {
    g_PlayerCar.trackProgress = progress;
    UpdateRivalCueGate();
}

static void TestRivalCueGate(void) {
    g_TrackLength = 0x20000;
    g_WrongWayTimer = 0;

    g_RivalCueEnabled = 1;
    UpdateGateAt(0x7000);
    Check(g_RivalCueEnabled == 0, "cue zone lower outside boundary");
    UpdateGateAt(0x7001);
    Check(g_RivalCueEnabled == 1, "cue zone lower inside boundary");
    UpdateGateAt(g_TrackLength - 0x3000 - 1);
    Check(g_RivalCueEnabled == 1, "cue zone upper inside boundary");
    UpdateGateAt(g_TrackLength - 0x3000);
    Check(g_RivalCueEnabled == 0, "cue zone upper outside boundary");

    g_RivalCueEnabled = 2;
    UpdateGateAt(0);
    Check(g_RivalCueEnabled == 2, "delayed lap cue state remains sticky");
    UpdateGateAt(0x7001);
    Check(g_RivalCueEnabled == 1, "cue zone normalizes delayed state");

    g_WrongWayTimer = 9;
    UpdateGateAt(0x7001);
    Check(g_RivalCueEnabled == 1, "wrong-way mute lower boundary");
    g_WrongWayTimer = 10;
    UpdateGateAt(0x7001);
    Check(g_RivalCueEnabled == 0, "wrong-way mute threshold");
}

int main(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    TestCountdownCues();
    TestRivalCueGate();

    if (s_failures != 0) {
        return 1;
    }
    puts("race cue gates honor their exact frame and track boundaries");
    return 0;
}
