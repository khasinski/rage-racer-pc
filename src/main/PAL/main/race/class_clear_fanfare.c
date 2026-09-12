#include "game/audio.h"
#include "game/race.h"

enum {
    CLASS_CLEAR_FANFARE_DURATION_FRAMES = 210,
    CLASS_CLEAR_CUE_TIMER = 180,
    SOUND_CUE_CLASS_CLEAR = 0x42,
};

static s32 s_timer;

void StartClassClearFanfare(void) {
    s_timer = CLASS_CLEAR_FANFARE_DURATION_FRAMES;
}

s32 TickClassClearFanfare(void) {
    if (s_timer != 0 && --s_timer == CLASS_CLEAR_CUE_TIMER) {
        PlaySoundCue(SOUND_CUE_CLASS_CLEAR);
    }

    return s_timer;
}
