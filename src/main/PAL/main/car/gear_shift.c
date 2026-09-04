#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/state.h"

enum {
    PAD_MAPPING_STRIDE = 8,
    SHIFT_UP_MAPPING_SLOT = 4,
    SHIFT_DOWN_MAPPING_SLOT = 5,
    AUTO_SHIFT_COOLDOWN_FRAMES = 25,
    HARD_BRAKE_THRESHOLD = 129,
    NORMAL_COOLDOWN_STEP = 1,
    HARD_BRAKE_COOLDOWN_STEP = 2,
};

static void ShiftManualGears(GameCarDrive *drive, s32 topGear,
                             s32 mappingBase) {
    /* Retail accepts another upshift only after its clutch counter reaches
     * zero, but accepts a downshift immediately. Preserve that asymmetric
     * behaviour: its eleven race frames are about 440 ms on PAL and 367 ms on
     * NTSC, so comparing the two releases by wall-clock time is misleading. */
    if ((g_PadPressed &
         g_PadButtonMapping[SHIFT_UP_MAPPING_SLOT + mappingBase]) != 0 &&
        drive->gear < topGear && drive->clutch == 0) {
        drive->gear++;
        g_SteerHoldFrames = 0;
    }
    if ((g_PadPressed &
         g_PadButtonMapping[SHIFT_DOWN_MAPPING_SLOT + mappingBase]) != 0 &&
        drive->gear > CAR_FIRST_FORWARD_GEAR) {
        drive->gear--;
        g_SteerHoldFrames = 0;
    }
}

static void UpdateAutoShiftCooldown(const GameCarDrive *drive) {
    if (g_AutoShiftCooldown <= 0) {
        return;
    }

    g_AutoShiftCooldown -=
        drive->brakeInput >= HARD_BRAKE_THRESHOLD
            ? HARD_BRAKE_COOLDOWN_STEP
            : NORMAL_COOLDOWN_STEP;
}

static void ResetStoppedAutomaticGear(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    if (car->speed != 0 || drive->gear <= CAR_FIRST_FORWARD_GEAR ||
        drive->motionState == CAR_MOTION_STANDING_START) {
        return;
    }
    drive->gear = CAR_FIRST_FORWARD_GEAR;
    drive->clutch = 0;
    g_AutoShiftCooldown = 0;
}

static void ShiftAutomaticGears(PlayerCarRuntime *car, s32 topGear) {
    GameCarDrive *drive = &car->drive;

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED &&
        g_AutoShiftCooldown <= 0 && drive->clutch == 0) {
        s32 gear = drive->gear;

        if (car->speed < g_CarSpec->shiftPoints[gear - 1].downshiftSpeed) {
            if (gear > CAR_FIRST_FORWARD_GEAR) {
                drive->gear--;
                g_AutoShiftCooldown = AUTO_SHIFT_COOLDOWN_FRAMES;
                g_SteerHoldFrames = 0;
            }
        } else if (g_CarSpec->shiftPoints[gear - 1].upshiftSpeed < car->speed &&
                   gear < topGear) {
            drive->gear++;
            g_AutoShiftCooldown = AUTO_SHIFT_COOLDOWN_FRAMES;
            g_SteerHoldFrames = 0;
        }
    }
    UpdateAutoShiftCooldown(drive);
    ResetStoppedAutomaticGear(car);
}

/* Pick the bounded manual or automatic gear for this frame. */
void ShiftPlayerGears(PlayerCarRuntime *car, int useAlternateMapping) {
    GameCarDrive *drive = &car->drive;
    s32 topGear = ClampCarGear(g_CarSpec->topGear, CAR_FORWARD_GEAR_COUNT);

    drive->gear = ClampCarGear(drive->gear, topGear);
    if (drive->manual != 0) {
        s32 mappingBase = useAlternateMapping ? PAD_MAPPING_STRIDE : 0;

        ShiftManualGears(drive, topGear, mappingBase);
    } else {
        ShiftAutomaticGears(car, topGear);
    }
}
