#include "game/car.h"
#include "game/input_internal.h"
#include "game/state.h"

enum {
    FIRST_GEAR = 1,
    MAX_FORWARD_GEARS = 6,
    PAD_MAPPING_STRIDE = 8,
};

static s32 EffectiveTopGear(void) {
    if (g_CarSpec->topGear < FIRST_GEAR) {
        return FIRST_GEAR;
    }
    if (g_CarSpec->topGear > MAX_FORWARD_GEARS) {
        return MAX_FORWARD_GEARS;
    }
    return g_CarSpec->topGear;
}

/*
 * Pick the gear for this frame.
 *
 * With a manual box the two shift buttons do it, bounded by the car's top gear
 * and by the clutch being free. Automatic reads the car's own shift-point
 * table: below the current gear's downshift speed it drops one, above the next
 * one's upshift speed it takes one, and a cooldown keeps it from hunting -
 * counted down twice as fast while the brake is on, because a car slowing hard
 * needs the gears sooner.
 *
 * Coming to a stop drops it straight back to first, unless the car has not
 * pulled away yet.
 */
void ShiftPlayerGears(PlayerCarRuntime *car, int mode23) {
    s32 topGear = EffectiveTopGear();
    s32 mappingBase = mode23 != 0 ? PAD_MAPPING_STRIDE : 0;

    if (car->drive.gear < FIRST_GEAR) {
        car->drive.gear = FIRST_GEAR;
    } else if (car->drive.gear > topGear) {
        car->drive.gear = (s16)topGear;
    }

    if (car->drive.manual != 0) {
        if (g_PadPressed & g_PadButtonMapping[4 + mappingBase]) {
            s32 g = car->drive.gear;

            if (g < topGear && car->drive.clutch == 0) {
                car->drive.gear++;
                g_SteerHoldFrames = 0;
            }
        }
        if (g_PadPressed & g_PadButtonMapping[5 + mappingBase]) {
            s32 g = car->drive.gear;

            if (g >= 2) {
                car->drive.gear--;
                g_SteerHoldFrames = 0;
            }
        }
    } else {
        if (car->verticalMotionState == 0) {
            s32 g;

            g = car->drive.gear;
            if (car->speed < g_CarSpec->shiftPoints[g - 1].downshiftSpeed &&
                g_AutoShiftCooldown <= 0 && car->drive.clutch == 0) {
                if (g >= 2) {
                    car->drive.gear--;
                    g_AutoShiftCooldown = 25;
                    g_SteerHoldFrames = 0;
                }
            } else {
                if (g_CarSpec->shiftPoints[g - 1].upshiftSpeed < car->speed &&
                    g_AutoShiftCooldown <= 0 && car->drive.clutch == 0 &&
                    g < topGear) {
                    car->drive.gear++;
                    g_AutoShiftCooldown = 25;
                    g_SteerHoldFrames = 0;
                }
            }
        }
        if (g_AutoShiftCooldown > 0) {
            if (car->drive.brakeInput >= 129) {
                g_AutoShiftCooldown = g_AutoShiftCooldown - 2;
            } else {
                g_AutoShiftCooldown--;
            }
        }
        if (car->speed == 0 && car->drive.gear >= 2 &&
            car->drive.motionState != CAR_MOTION_STANDING_START) {
            car->drive.gear = FIRST_GEAR;
            car->drive.clutch = 0;
            g_AutoShiftCooldown = 0;
        }
    }
}
