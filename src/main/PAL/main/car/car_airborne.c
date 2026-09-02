#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "psyq/gte.h"

static s32 YawMagnitude(s32 yawOffset) {
    return yawOffset < 0 ? -yawOffset : yawOffset;
}

void UpdateCarAirborne(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    if (g_ShiftSoundLevel == 0) {
        s32 offAxis = YawMagnitude(drive->yawOffset);
        s32 phase = offAxis < 513 ? offAxis * 3 + 6144 : 0x1E00;

        SetIndexedEffectVoice(0, phase, drive->jumpTimer * 2 + 80);
    } else {
        SetIndexedEffectVoice(0, 0x1800, g_ShiftSoundLevel + 25);
    }

    car->bodyYaw += GetAngleDelta(car->bodyYaw, drive->targetHeading) / 5;
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    drive->accelPos =
        rsin(car->headingAngle + drive->yawOffset) * car->speed / 256;
    drive->brakePos =
        rcos(car->headingAngle + drive->yawOffset) * car->speed / 256;
    alongBody = (bodySin * drive->accelPos + bodyCos * drive->brakePos) / 4096;

    drive->accelPos = rsin(drive->launchHeading) * drive->launchSpeed / 256 +
                      bodySin * alongBody / 4096;
    drive->brakePos = rcos(drive->launchHeading) * drive->launchSpeed / 256 +
                      bodyCos * alongBody / 4096;

    if (drive->acceleratorLatch != 1 && drive->brakeLatch != 1 &&
        drive->acceleratorInput.value < 128)
        drive->groundedFrames++;
    else
        drive->groundedFrames = 0;

    drive->spinRate = drive->spinRate * 31 / 32;
    drive->launchSpeed = drive->launchSpeed * 31 / 32;
    drive->yawOffset = drive->yawOffset * 31 / 32;
    drive->bodyLiftOffset = drive->bodyLiftOffset * 2 / 3;

    if (YawMagnitude(drive->yawOffset) >= 1537)
        car->speed = car->speed * 4 / 5;

    if (drive->jumpTimer <= 0) {
        SetIndexedEffectVoice(-1, 0, 0);
        car->bodyYaw -= drive->spinRate;
        g_ShiftSoundLevel = 0;
        drive->shiftRpmDelta = 0;
        drive->yawOffset = 0;
        drive->launchSpeed = 0;
        drive->motionState = CAR_MOTION_DRIVING;
        drive->bodyLiftOffset = 0;
    }
}
