#include "game/audio_internal.h"
#include "game/race.h"
#include "game/race_internal.h"

#include <stdint.h>

enum {
    ATTRACT_TITLE_MAX_FADE = 0x7F,
    ATTRACT_TITLE_FADE_SPEED = 4,
    ATTRACT_LOAD_TIMER_LIMIT = 10000,
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
        int64_t fade = (int64_t)timer * ATTRACT_TITLE_FADE_SPEED - delay;

        if (fade <= 0) {
            return 0;
        }
        return fade < ATTRACT_TITLE_MAX_FADE ? (s32)fade
                                            : ATTRACT_TITLE_MAX_FADE;
    }
    return ClampAttractTitleFade(fadeLevel);
}

s32 AttractOpeningWashLevel(s32 timer) {
    int64_t fade = ATTRACT_OPENING_FADE_START -
                   ((int64_t)timer - ATTRACT_OPENING_FADE_OFFSET) *
                       ATTRACT_OPENING_FADE_SPEED;

    if (fade < INT32_MIN) {
        return INT32_MIN;
    }
    return fade > INT32_MAX ? INT32_MAX : (s32)fade;
}

s32 NextAttractLoadTimer(s32 timer) {
    if (timer < 0) {
        return 0;
    }
    return timer < ATTRACT_LOAD_TIMER_LIMIT ? timer + 1
                                            : ATTRACT_LOAD_TIMER_LIMIT;
}

s32 NextAttractRaceTimer(s32 timer) {
    if (timer < 0) {
        return 0;
    }
    return timer < ATTRACT_RETURN_FRAME ? timer + 1 : ATTRACT_RETURN_FRAME;
}

s32 AttractBgmTrack(const u8 *shuffleOrder, s32 trackCount,
                    s32 shuffleIndex) {
    s32 track;

    if (shuffleOrder == NULL || trackCount <= 0) {
        return 0;
    }
    if (trackCount > BGM_PLAYABLE_TRACK_COUNT) {
        trackCount = BGM_PLAYABLE_TRACK_COUNT;
    }
    shuffleIndex %= trackCount;
    if (shuffleIndex < 0) {
        shuffleIndex += trackCount;
    }
    track = shuffleOrder[shuffleIndex];
    return track < trackCount ? track : 0;
}

s32 ShouldStartAttractExitFade(s32 timer) {
    return timer == ATTRACT_EXIT_FADE_FRAME;
}

s32 AttractClosingWashLevel(s32 timer) {
    int64_t fade = ((int64_t)timer - ATTRACT_EXIT_FADE_FRAME) *
                   ATTRACT_EXIT_FADE_SPEED;

    if (fade < INT32_MIN) {
        return INT32_MIN;
    }
    return fade > INT32_MAX ? INT32_MAX : (s32)fade;
}

s32 ShouldReturnFromAttractDemo(s32 timer) {
    return timer >= ATTRACT_RETURN_FRAME;
}
