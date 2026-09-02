#include "game/audio.h"
#include "game/race.h"

enum {
    CLASS_CLEAR_CUE_TIMER = 180,
    SOUND_CUE_CLASS_CLEAR = 0x42,
};

void TickClassClearFanfare(void) {
    if (g_ClassClearFanfareTimer > 0) {
        g_ClassClearFanfareTimer--;
    }
    if (g_ClassClearFanfareTimer == CLASS_CLEAR_CUE_TIMER) {
        PlaySoundCue(SOUND_CUE_CLASS_CLEAR);
    }
}
