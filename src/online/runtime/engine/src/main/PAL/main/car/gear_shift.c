#include "game/car_shift.h"

enum {
    AUTO_SHIFT_COOLDOWN_FRAMES = 25,
    HARD_BRAKE_THRESHOLD = 129,
    NORMAL_COOLDOWN_STEP = 1,
    HARD_BRAKE_COOLDOWN_STEP = 2,
};

static void ShiftManualGears(GameCarDrive *drive, s32 topGear,
                             int shiftUp, int shiftDown) {
    /* Retail accepts another upshift only after its clutch counter reaches
     * zero, but accepts a downshift immediately. Preserve that asymmetric
     * behaviour: its eleven race frames are about 440 ms on PAL and 367 ms on
     * NTSC, so comparing the two releases by wall-clock time is misleading. */
    if (shiftUp &&
        drive->gear < topGear && drive->clutch == 0) {
        drive->gear++;
        drive->steerHoldFrames = 0;
    }
    if (shiftDown &&
        drive->gear > CAR_FIRST_FORWARD_GEAR) {
        drive->gear--;
        drive->steerHoldFrames = 0;
    }
}

static void UpdateAutoShiftCooldown(GameCarDrive *drive) {
    if (drive->autoShiftCooldown <= 0) {
        return;
    }

    drive->autoShiftCooldown -=
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
    drive->autoShiftCooldown = 0;
}

static void ShiftAutomaticGears(PlayerCarRuntime *car, const GameCarSpec *spec,
                                s32 topGear) {
    GameCarDrive *drive = &car->drive;

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED &&
        drive->autoShiftCooldown <= 0 && drive->clutch == 0) {
        s32 gear = drive->gear;

        if (car->speed < spec->shiftPoints[gear - 1].downshiftSpeed) {
            if (gear > CAR_FIRST_FORWARD_GEAR) {
                drive->gear--;
                drive->autoShiftCooldown = AUTO_SHIFT_COOLDOWN_FRAMES;
                drive->steerHoldFrames = 0;
            }
        } else if (spec->shiftPoints[gear - 1].upshiftSpeed < car->speed &&
                   gear < topGear) {
            drive->gear++;
            drive->autoShiftCooldown = AUTO_SHIFT_COOLDOWN_FRAMES;
            drive->steerHoldFrames = 0;
        }
    }
    UpdateAutoShiftCooldown(drive);
    ResetStoppedAutomaticGear(car);
}

/* Pick the bounded manual or automatic gear for this frame. */
void ShiftCarGears(PlayerCarRuntime *car, const GameCarSpec *spec,
                    int shiftUp, int shiftDown) {
    GameCarDrive *drive = &car->drive;
    s32 topGear = ClampCarGear(spec->topGear, CAR_FORWARD_GEAR_COUNT);

    drive->gear = ClampCarGear(drive->gear, topGear);
    if (drive->manual != 0) {
        ShiftManualGears(drive, topGear, shiftUp, shiftDown);
    } else {
        ShiftAutomaticGears(car, spec, topGear);
    }
}
