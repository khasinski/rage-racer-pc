#include "game/race.h"
#include "game/race_internal.h"

enum {
    ATTRACT_TITLE_MAX_FADE = 0x7F,
    ATTRACT_TITLE_FADE_SPEED = 4,
    ATTRACT_OPENING_FADE_OFFSET = 6,
    ATTRACT_OPENING_FADE_START = 0xFF,
    ATTRACT_OPENING_FADE_SPEED = 11,
    ATTRACT_EXIT_FADE_FRAME = 0x6CC,
    ATTRACT_EXIT_FADE_SPEED = 5,
    ATTRACT_RETURN_FRAME = 0x708,
};

static s32 ClampAttractTitleFade(s32 fade) {
    if (fade < 0) {
        return 0;
    }
    if (fade > ATTRACT_TITLE_MAX_FADE) {
        return ATTRACT_TITLE_MAX_FADE;
    }
    return fade;
}

s32 AttractTitleFadeLevel(s32 step, s32 timer, s32 fadeLevel, s32 delay) {
    if (step == ATTRACT_DEMO_STEP_LOAD) {
        return ClampAttractTitleFade(timer * ATTRACT_TITLE_FADE_SPEED - delay);
    }
    return ClampAttractTitleFade(fadeLevel);
}

s32 AttractOpeningWashLevel(s32 timer) {
    return ATTRACT_OPENING_FADE_START -
           (timer - ATTRACT_OPENING_FADE_OFFSET) * ATTRACT_OPENING_FADE_SPEED;
}

s32 ShouldStartAttractExitFade(s32 timer) {
    return timer == ATTRACT_EXIT_FADE_FRAME;
}

s32 AttractClosingWashLevel(s32 timer) {
    return (timer - ATTRACT_EXIT_FADE_FRAME) * ATTRACT_EXIT_FADE_SPEED;
}

s32 ShouldReturnFromAttractDemo(s32 timer) {
    return timer == ATTRACT_RETURN_FRAME;
}
