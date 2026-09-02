#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/random.h"
#include "psyq/gte.h"

void UpdateCarStandingStart(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    car->bodyYaw += GetAngleDelta(car->bodyYaw, drive->targetHeading) / 5;
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);
    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
    alongBody = (bodySin * drive->accelPos + bodyCos * drive->brakePos) / 4096;
    drive->accelPos = bodySin * alongBody / 16384;
    drive->brakePos = bodyCos * alongBody / 16384;

    SetIndexedEffectVoice(0, 0x1A80,
                          (0x60 - (g_StandingStartSpin & 0x1F) * 2) *
                              drive->acceleratorInput.value / 256);
    car->speed /= 10;

    if (g_StandingStartSpin >= 11) {
        s32 throttle = drive->acceleratorInput.value;
        s32 rpm = drive->engineRpm;
        s32 grip = throttle + 32;

        g_StandingStartSpin -= drive->brakeInput * 2;
        if (rpm < 2000) grip = throttle + 1032;
        if (throttle < 127 && rpm >= 2001) grip += 127;

        drive->standingStartBounceY = (Random15() & 3) * grip / 256;
        drive->standingStartBounceX = (Random15() & 7) * grip / 256;
        g_StandingStartSpin -= grip;
        if (g_StandingStartSpin > 0) return;
    }

    drive->standingStartBounceY = 0;
    drive->standingStartBounceX = 0;
    drive->motionState = CAR_MOTION_DRIVING;
    SetIndexedEffectVoice(-1, 0, 0);
}
