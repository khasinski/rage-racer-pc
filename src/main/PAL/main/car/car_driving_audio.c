#include "game/car_internal.h"
#include "game/audio.h"

enum {
    REDLINE_EFFECT_RPM_MARGIN = 1000,
    REDLINE_EFFECT_HOLD_FRAMES = 41,
    REDLINE_EFFECT_MAX_LEVEL = 100,
    REDLINE_EFFECT_VOLUME_BASE = 24,
    REDLINE_EFFECT_VOICE = 2,
    REDLINE_EFFECT_PHASE = 0x1500,
};

void PlayCarDrivingVoice(const PlayerCarRuntime *car,
                                       const GameCarSpec *spec) {
    const GameCarDrive *drive = &car->drive;
    s32 voiceLevel;

    if (drive->engineRpm <= WrapSigned32(
            (int64_t)spec->redline + REDLINE_EFFECT_RPM_MARGIN) ||
        drive->steerHoldFrames < REDLINE_EFFECT_HOLD_FRAMES ||
        drive->gear != spec->topGear ||
        car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    voiceLevel = drive->steerHoldFrames + REDLINE_EFFECT_VOLUME_BASE;
    if (voiceLevel > REDLINE_EFFECT_MAX_LEVEL) {
        voiceLevel = REDLINE_EFFECT_MAX_LEVEL;
    }
    SetIndexedEffectVoice(
        REDLINE_EFFECT_VOICE, REDLINE_EFFECT_PHASE, voiceLevel);
}
