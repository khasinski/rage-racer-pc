#include "game/race_internal.h"

#define PROLOGUE_TEXT_FULL_BRIGHTNESS 0x7F
#define PROLOGUE_TEXT_FADE_TOP 0x60
#define PROLOGUE_TEXT_FADE_BOTTOM 0x90
#define PROLOGUE_WORLD_FIRST_FRAME 0x10
#define PROLOGUE_WORLD_FRAME_COUNT 0x40F

s32 PrologueLineIntensity(s32 screenY) {
    s32 fade;

    if (screenY < PROLOGUE_TEXT_FADE_TOP) {
        fade = (PROLOGUE_TEXT_FADE_TOP - screenY) * 2;
    } else if (screenY > PROLOGUE_TEXT_FADE_BOTTOM) {
        fade = (screenY - PROLOGUE_TEXT_FADE_BOTTOM) * 2;
    } else {
        fade = 0;
    }

    if (fade > PROLOGUE_TEXT_FULL_BRIGHTNESS) {
        fade = PROLOGUE_TEXT_FULL_BRIGHTNESS;
    }
    return PROLOGUE_TEXT_FULL_BRIGHTNESS - fade;
}

s32 IsPrologueWorldActive(s32 sceneTimer) {
    return sceneTimer >= PROLOGUE_WORLD_FIRST_FRAME &&
           sceneTimer < PROLOGUE_WORLD_FIRST_FRAME + PROLOGUE_WORLD_FRAME_COUNT;
}
