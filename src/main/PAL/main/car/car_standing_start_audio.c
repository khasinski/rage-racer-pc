#include "game/car_internal.h"
#include "game/audio.h"

enum {
    STANDING_START_EFFECT_PHASE = 0x1A80,
    STANDING_START_EFFECT_BASE_VOLUME = 0x60,
    STANDING_START_EFFECT_SPIN_MASK = 0x1F,
    STANDING_START_EFFECT_SPIN_SCALE = 2,
    PEDAL_INPUT_FULL = 256,
};

void PlayCarStandingStartVoice(const PlayerCarRuntime *car) {
    const GameCarDrive *drive = &car->drive;
    SetIndexedEffectVoice(
        0, STANDING_START_EFFECT_PHASE,
        (STANDING_START_EFFECT_BASE_VOLUME -
         (drive->standingStartSpin & STANDING_START_EFFECT_SPIN_MASK) *
             STANDING_START_EFFECT_SPIN_SCALE) *
            drive->acceleratorInput.value / PEDAL_INPUT_FULL);
}
