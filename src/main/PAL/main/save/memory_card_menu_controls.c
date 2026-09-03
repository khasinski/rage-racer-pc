#include "game/audio.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/state.h"

void AdjustMenuSelectionVertical(s32 *value, s32 min, s32 max) {
    u16 input = g_PadPressedRepeat;

    if (input & PAD_DOWN) {
        if (*value < max) {
            (*value)++;
            PlaySoundCue(1);
        }
    } else if (input & PAD_UP) {
        if (*value > min) {
            (*value)--;
            PlaySoundCue(1);
        }
    }
}

void SetMenuBinaryChoiceHorizontal(s32 *value) {
    u16 input = g_PadPressedRepeat;

    if (input & PAD_LEFT) {
        if (*value == 0) {
            PlaySoundCue(1);
        }
        *value = 1;
    } else if (input & PAD_RIGHT) {
        if (*value == 1) {
            PlaySoundCue(1);
        }
        *value = 0;
    }
}

u16 PollMenuConfirmInput(void) {
    u16 value = g_PadPressed & PAD_CONFIRM;

    if (value != 0) {
        PlaySoundCue(2);
    }
    return value;
}

u16 PollMenuBackInput(void) {
    u16 value = g_PadPressed & PAD_CANCEL;

    if (value != 0) {
        PlaySoundCue(3);
    }
    return value;
}
