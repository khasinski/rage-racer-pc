#include "game/race.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/track_internal.h"

enum {
    COUNTDOWN_FIRST_CUE_FRAME = 0x79,
    COUNTDOWN_SECOND_CUE_FRAME = 0x97,
    COUNTDOWN_THIRD_CUE_FRAME = 0xB5,
    COUNTDOWN_FOURTH_CUE_FRAME = 0xD3,
    COUNTDOWN_START_CUE_FRAME = 0x10F,
    RIVAL_CUE_ZONE_START = 0x7001,
    RIVAL_CUE_FINISH_MARGIN = 0x3000,
    WRONG_WAY_CUE_MUTE_FRAMES = 10
};

void PlayCountdownCues(s32 timer) {
    switch (timer) {
    case COUNTDOWN_FIRST_CUE_FRAME:
        PlaySoundCue(0x1E);
        break;
    case COUNTDOWN_SECOND_CUE_FRAME:
        PlaySoundCue(0x1F);
        break;
    case COUNTDOWN_THIRD_CUE_FRAME:
        PlaySoundCue(0x20);
        break;
    case COUNTDOWN_FOURTH_CUE_FRAME:
        PlaySoundCue(0x21);
        break;
    case COUNTDOWN_START_CUE_FRAME:
        if (g_GrandPrixMode == 1) {
            PlaySoundCue(0x22);
            g_RaceCueDelay = 0x5A;
        }
        break;
    }
}

void UpdateRivalCueGate(void) {
    s32 progress = g_PlayerCar.trackProgress;
    s32 inCueZone = progress >= RIVAL_CUE_ZONE_START &&
                    progress < g_TrackLength - RIVAL_CUE_FINISH_MARGIN;

    if (inCueZone) {
        g_RivalCueEnabled = 1;
    } else if (g_RivalCueEnabled == 1) {
        /* State 2 is deliberately sticky outside the zone: lap cue playback
         * uses it to keep rival speech enabled after its delay expires. */
        g_RivalCueEnabled = 0;
    }

    if (g_WrongWayTimer >= WRONG_WAY_CUE_MUTE_FRAMES) {
        g_RivalCueEnabled = 0;
    }
}
