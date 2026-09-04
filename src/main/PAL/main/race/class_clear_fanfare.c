#include "game/audio.h"
#include "game/race.h"

enum {
    CLASS_CLEAR_CUE_TIMER = 180,
    SOUND_CUE_CLASS_CLEAR = 0x42,
};

void TickClassClearFanfare(void) {
    s32 timer = g_ClassClearFanfareTimer;

    if (timer <= 0) {
        g_ClassClearFanfareTimer = 0;
        return;
    }
    if (timer > CLASS_CLEAR_FANFARE_DURATION_FRAMES) {
        timer = CLASS_CLEAR_FANFARE_DURATION_FRAMES;
    }

    timer--;
    g_ClassClearFanfareTimer = timer;
    if (timer == CLASS_CLEAR_CUE_TIMER) {
        PlaySoundCue(SOUND_CUE_CLASS_CLEAR);
    }
}
