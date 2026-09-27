#include "game/audio.h"
#include "game/car_audio.h"

enum { LONG_LANDING_SOUND_FRAMES = 19, LANDING_SOUND_CUE = 0xE };

void PlayPlayerLandingCue(s32 landingFrames, int audible) {
    if (audible && landingFrames >= LONG_LANDING_SOUND_FRAMES) {
        PlaySoundCue(LANDING_SOUND_CUE);
    }
}

