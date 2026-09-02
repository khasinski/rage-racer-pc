#include "game/car.h"

/* Seeds launch spin from revs above the power peak. Starting in second gear
 * or higher also begins with a short loss of grip. */
void BeginCarStandingStart(PlayerCarRuntime *car) {
    s32 spin;
    s32 gear = car->drive.gear;
    s32 revLimit = g_CarSpec->revLimit;

    if (gear < 1) {
        gear = 1;
    } else if (gear > CAR_FORWARD_GEAR_COUNT) {
        gear = CAR_FORWARD_GEAR_COUNT;
    }
    car->drive.gear = (s16)gear;
    if (revLimit <= 0) {
        revLimit = 1;
    }

    spin = ((g_EngineRpm - g_PeakOutputRpm) * 10000) / revLimit;
    g_GripLossTimer = 0;

    if (spin < 0) {
        spin = g_EngineRpm < 2000 ? 0 : g_EngineRpm - 1000;
    } else {
        spin *= g_PeakOutputValue / ((gear * 200) + 300);
        car->drive.drivetrainTorque /= gear;
        if (gear >= 2) {
            g_GripLossTimer = 200;
        }
    }

    g_StandingStartSpin = spin;
}
