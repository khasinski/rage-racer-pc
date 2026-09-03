#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/state.h"

#include "rage/trace.h"

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

static void SettleDisplayedEngineRpm(const GameCarDrive *drive) {
    s32 shown = g_EngineRpm;
    s32 gap = WrapSigned32((int64_t)drive->engineRpm - shown);
    s32 limit = g_CarSpec->revLimit;

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
    g_EngineRpm = shown;
}

static void UpdatePlayerEngineAudio(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 revFlag = 0;
    int usePoweredBank;

    if (g_EngineRpm >= g_CarSpec->revLimit - REV_LIMIT_JITTER_MARGIN &&
        drive->acceleratorInput.value >= ACCELERATOR_ACTIVE_THRESHOLD) {
        g_TachoShiftLightOn = (g_AnimTimer & SHIFT_LIGHT_BLINK_MASK) != 0;
        g_EngineRpmJitter = Random15() % REV_LIMIT_JITTER_RANGE /
                            REV_LIMIT_JITTER_DIVISOR;
    } else if (drive->engineRpm == 0 &&
               (g_AnimTimer & IDLE_JITTER_FRAME_MASK)) {
        g_TachoShiftLightOn = 0;
        g_EngineRpmJitter =
            rsin(Random15() & IDLE_JITTER_PHASE_MASK) *
            REV_LIMIT_JITTER_RANGE / TRIG_SCALE;
        if (g_EngineRpmJitter <= 0) {
            g_EngineRpmJitter = 0;
        }
        revFlag = g_EngineRpmJitter < IDLE_REV_THRESHOLD;
    } else {
        g_EngineRpmJitter = 0;
        g_TachoShiftLightOn = 0;
    }

    g_EngineRpmSnapshot = g_EngineRpm;
    if (drive->engineRpm != 0) {
        if (drive->gear == CAR_FIRST_FORWARD_GEAR) {
            revFlag = 1;
        } else if (g_EngineRpm >=
                   g_CarSpec->redline - REDLINE_RANDOM_REV_RANGE) {
            revFlag = g_EngineRpm >= g_CarSpec->redline
                ? 1
                : Random15() & RANDOM_BOOLEAN_MASK;
        } else {
            revFlag = 0;
        }
    }

    if (g_RacePhase >= 4) {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    usePoweredBank = drive->acceleratorInput.value > 0 && revFlag != 0 &&
                     (drive->manual == 0 || drive->clutch == 0);
    UpdateLoadedAudioVoices(
        g_EngineRpm + g_EngineRpmJitter, usePoweredBank);

    drive->gearDisp = drive->gear;
    TraceCarMotion("post-update", car);
}

void UpdatePlayerEnginePresentation(PlayerCarRuntime *car) {
    SettleDisplayedEngineRpm(&car->drive);
    UpdatePlayerEngineAudio(car);
}
