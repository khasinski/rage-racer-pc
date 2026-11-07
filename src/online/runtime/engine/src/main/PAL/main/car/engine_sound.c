#include "game/engine_sound.h"
#include "game/integer.h"
#include "game/random.h"
#include "game/angle.h"

enum {
    DISPLAYED_RPM_MINIMUM = 500,
    DISPLAYED_RPM_CLUTCH_RESPONSE = 2,
    DISPLAYED_RPM_DRIVING_RESPONSE = 4,
    REV_LIMIT_JITTER_MARGIN = 100,
    ACCELERATOR_ACTIVE_THRESHOLD = 129,
    REV_LIMIT_JITTER_RANGE = 150,
    REV_LIMIT_JITTER_DIVISOR = 2,
    IDLE_JITTER_PHASE_MASK = 0xFFF,
    TRIG_SCALE = 4096,
    IDLE_REV_THRESHOLD = 37,
    REDLINE_RANDOM_REV_RANGE = 2000,
    SHIFT_LIGHT_BLINK_MASK = 2,
    IDLE_JITTER_FRAME_MASK = 8,
    RANDOM_BOOLEAN_MASK = 1,
};

static void SettleDisplayedEngineRpm(EngineSound *sound, const GameCarDrive *drive, const GameCarSpec *spec) {
    s32 shown = sound->rpm;
    s32 gap = WrapSigned32((int64_t)drive->engineRpm - shown);
    s32 limit = spec->revLimit;

    shown = WrapSigned32(
        (int64_t)shown +
        (drive->clutch > 0
             ? gap / DISPLAYED_RPM_CLUTCH_RESPONSE
             : gap / DISPLAYED_RPM_DRIVING_RESPONSE));
    if (shown >= limit) {
        shown = limit;
    } else if (shown < DISPLAYED_RPM_MINIMUM) {
        shown = DISPLAYED_RPM_MINIMUM;
    }
    sound->rpm = shown;
}

static void SelectEngineSound(EngineSound *sound, const GameCarDrive *drive, const GameCarSpec *spec, u32 frame, u32 *random) {
    s32 revFlag = 0;

    if (sound->rpm >= spec->revLimit - REV_LIMIT_JITTER_MARGIN &&
        drive->acceleratorInput.value >= ACCELERATOR_ACTIVE_THRESHOLD) {
        sound->shiftLight = (frame & SHIFT_LIGHT_BLINK_MASK) != 0;
        sound->jitter = RandomNext(random) % REV_LIMIT_JITTER_RANGE /
                            REV_LIMIT_JITTER_DIVISOR;
    } else if (drive->engineRpm == 0 &&
               (frame & IDLE_JITTER_FRAME_MASK)) {
        sound->shiftLight = 0;
        sound->jitter =
            SinAngle(RandomNext(random) & IDLE_JITTER_PHASE_MASK) *
            REV_LIMIT_JITTER_RANGE / TRIG_SCALE;
        if (sound->jitter <= 0) {
            sound->jitter = 0;
        }
        revFlag = sound->jitter < IDLE_REV_THRESHOLD;
    } else {
        sound->jitter = 0;
        sound->shiftLight = 0;
    }

    if (drive->engineRpm != 0) {
        if (drive->gear == CAR_FIRST_FORWARD_GEAR) {
            revFlag = 1;
        } else if (sound->rpm >=
                   spec->redline - REDLINE_RANDOM_REV_RANGE) {
            revFlag = sound->rpm >= spec->redline
                ? 1
                : RandomNext(random) & RANDOM_BOOLEAN_MASK;
        } else {
            revFlag = 0;
        }
    }

    sound->powered = drive->acceleratorInput.value > 0 && revFlag != 0 &&
                     (drive->manual == 0 || drive->clutch == 0);

}


int StepEngineSound(EngineSound *sound, const GameCarDrive *drive,
                    const GameCarSpec *spec, u32 frame, u32 seed) {
    if (!sound || !drive || !spec) return 0;
    u32 random = seed ^ frame;
    SettleDisplayedEngineRpm(sound, drive, spec);
    SelectEngineSound(sound, drive, spec, frame, &random);
    return 1;
}
