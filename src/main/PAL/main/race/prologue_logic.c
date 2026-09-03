#include "game/car.h"
#include "game/race_internal.h"

#include <stdint.h>

#define PROLOGUE_TEXT_FULL_BRIGHTNESS 0x7F
#define PROLOGUE_TEXT_FADE_TOP 0x60
#define PROLOGUE_TEXT_FADE_BOTTOM 0x90
#define PROLOGUE_WORLD_FIRST_FRAME 0x10
#define PROLOGUE_WORLD_FRAME_COUNT 0x40F

s32 NextPrologueTimer(s32 sceneTimer) {
    if (sceneTimer < 0) {
        return 0;
    }
    return sceneTimer < PROLOGUE_END_FRAME
        ? sceneTimer + 1
        : PROLOGUE_END_FRAME;
}

s32 AdvancePrologueFade(s32 level, s32 step, s32 maximum) {
    int64_t next = (int64_t)level + step;

    if (maximum < 0 || next <= 0) {
        return 0;
    }
    return next < maximum ? (s32)next : maximum;
}

s32 PrologueCameraIndex(s32 cameraIndex) {
    return (u32)cameraIndex < RACE_CAR_SLOT_COUNT ? cameraIndex : 0;
}

s32 PrologueLineIntensity(s32 screenY) {
    int64_t fade;

    if (screenY < PROLOGUE_TEXT_FADE_TOP) {
        fade = ((int64_t)PROLOGUE_TEXT_FADE_TOP - screenY) * 2;
    } else if (screenY > PROLOGUE_TEXT_FADE_BOTTOM) {
        fade = ((int64_t)screenY - PROLOGUE_TEXT_FADE_BOTTOM) * 2;
    } else {
        fade = 0;
    }

    if (fade > PROLOGUE_TEXT_FULL_BRIGHTNESS) {
        fade = PROLOGUE_TEXT_FULL_BRIGHTNESS;
    }
    return PROLOGUE_TEXT_FULL_BRIGHTNESS - (s32)fade;
}

s32 IsPrologueWorldActive(s32 sceneTimer) {
    return sceneTimer >= PROLOGUE_WORLD_FIRST_FRAME &&
           sceneTimer < PROLOGUE_WORLD_FIRST_FRAME + PROLOGUE_WORLD_FRAME_COUNT;
}
