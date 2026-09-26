#include "game/car_internal.h"
#include "game/audio.h"

enum {
    AIRBORNE_SKID_PHASE_LIMIT = 513,
    AIRBORNE_SKID_PHASE_BASE = 0x1800,
    AIRBORNE_SKID_PHASE_MAXIMUM = 0x1E00,
    AIRBORNE_SKID_PHASE_PER_YAW = 3,
    AIRBORNE_TIMER_VOLUME_SCALE = 2,
    AIRBORNE_TIMER_VOLUME_BASE = 80,
    AIRBORNE_SHIFT_VOLUME_BASE = 25,
};

static s32 AbsoluteYawOffset(s32 yawOffset) {
    return yawOffset < 0 ? WrapSigned32(-(int64_t)yawOffset) : yawOffset;
}

static void UpdateAirborneTyreVoice(const GameCarDrive *drive) {
    if (drive->shiftSoundLevel == 0) {
        s32 offAxis = AbsoluteYawOffset(drive->yawOffset);
        s32 phase = offAxis < AIRBORNE_SKID_PHASE_LIMIT
                        ? WrapSigned32(
                              (int64_t)offAxis *
                                  AIRBORNE_SKID_PHASE_PER_YAW +
                              AIRBORNE_SKID_PHASE_BASE)
                        : AIRBORNE_SKID_PHASE_MAXIMUM;

        SetIndexedEffectVoice(
            0, phase,
            drive->jumpTimer * AIRBORNE_TIMER_VOLUME_SCALE +
                AIRBORNE_TIMER_VOLUME_BASE);
    } else {
        SetIndexedEffectVoice(
            0, AIRBORNE_SKID_PHASE_BASE,
            WrapSigned32((int64_t)drive->shiftSoundLevel +
                         AIRBORNE_SHIFT_VOLUME_BASE));
    }
}

void PlayCarAirborneVoice(const PlayerCarRuntime *car) {
    UpdateAirborneTyreVoice(&car->drive);
}
