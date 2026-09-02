#include "game/audio.h"
#include "game/race.h"
#include "game/track.h"

enum {
    COUNTDOWN_FIRST_CUE_FRAME = 0x79,
    COUNTDOWN_SECOND_CUE_FRAME = 0x97,
    COUNTDOWN_THIRD_CUE_FRAME = 0xB5,
    COUNTDOWN_FOURTH_CUE_FRAME = 0xD3,
    COUNTDOWN_START_CUE_FRAME = 0x10F,
    COUNTDOWN_FIRST_CUE = 0x1E,
    COUNTDOWN_SECOND_CUE = 0x1F,
    COUNTDOWN_THIRD_CUE = 0x20,
    COUNTDOWN_FOURTH_CUE = 0x21,
    GRAND_PRIX_START_CUE = 0x22,
    GRAND_PRIX_LAP_CUE_DELAY = 90,
};

void PlayCountdownCues(s32 timer) {
    switch (timer) {
    case COUNTDOWN_FIRST_CUE_FRAME:
        PlaySoundCue(COUNTDOWN_FIRST_CUE);
        break;
    case COUNTDOWN_SECOND_CUE_FRAME:
        PlaySoundCue(COUNTDOWN_SECOND_CUE);
        break;
    case COUNTDOWN_THIRD_CUE_FRAME:
        PlaySoundCue(COUNTDOWN_THIRD_CUE);
        break;
    case COUNTDOWN_FOURTH_CUE_FRAME:
        PlaySoundCue(COUNTDOWN_FOURTH_CUE);
        break;
    case COUNTDOWN_START_CUE_FRAME:
        if (g_GrandPrixMode == 1) {
            PlaySoundCue(GRAND_PRIX_START_CUE);
            g_RaceCueDelay = GRAND_PRIX_LAP_CUE_DELAY;
        }
        break;
    }
}
