#include "game/car.h"
#include "game/integer.h"

enum {
    FIRST_FORWARD_GEAR = 1,
    MINIMUM_REV_LIMIT = 1,
    STANDING_START_SPIN_SCALE = 10000,
    LOW_RPM_SPIN_CUTOFF = 2000,
    LOW_RPM_SPIN_OFFSET = 1000,
    OUTPUT_LOAD_PER_GEAR = 200,
    OUTPUT_LOAD_BASE = 300,
    GRIP_LOSS_FIRST_GEAR = 2,
    GRIP_LOSS_FRAMES = 200,
};

/* Seeds launch spin from revs above the power peak. Starting in second gear
 * or higher also begins with a short loss of grip. */
void BeginCarStandingStart(PlayerCarRuntime *car) {
    s32 spin;
    s32 gear = car->drive.gear;
    s32 revLimit = g_CarSpec->revLimit;

    if (gear < FIRST_FORWARD_GEAR) {
        gear = FIRST_FORWARD_GEAR;
    } else if (gear > CAR_FORWARD_GEAR_COUNT) {
        gear = CAR_FORWARD_GEAR_COUNT;
    }
    car->drive.gear = (s16)gear;
    if (revLimit <= 0) {
        revLimit = MINIMUM_REV_LIMIT;
    }

    spin = WrapSigned32(
        (int64_t)g_EngineRpm - g_PeakOutputRpm);
    spin = WrapSigned32(
        (int64_t)spin * STANDING_START_SPIN_SCALE) / revLimit;
    g_GripLossTimer = 0;

    if (spin < 0) {
        spin = g_EngineRpm < LOW_RPM_SPIN_CUTOFF
                   ? 0
                   : g_EngineRpm - LOW_RPM_SPIN_OFFSET;
    } else {
        spin = WrapSigned32(
            (int64_t)spin *
            (g_PeakOutputValue /
             (gear * OUTPUT_LOAD_PER_GEAR + OUTPUT_LOAD_BASE)));
        car->drive.drivetrainTorque /= gear;
        if (gear >= GRIP_LOSS_FIRST_GEAR) {
            g_GripLossTimer = GRIP_LOSS_FRAMES;
        }
    }

    g_StandingStartSpin = spin;
}
