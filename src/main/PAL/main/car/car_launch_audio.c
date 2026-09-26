#include "game/angle.h"
#include "game/audio.h"
#include "game/car_drive.h"

static void UpdateLaunchTyreVoice(const PlayerCarRuntime *car, s32 skid) {
    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    if (skid < 513) {
        SetIndexedEffectVoice(
            0, skid * 3 + 0x1800,
            skid / 8 + 0x40);
    } else {
        SetIndexedEffectVoice(0, 0x1E00,
                              0x7F);
    }
}

void PlayCarLaunchVoice(const PlayerCarRuntime *car) {
    UpdateLaunchTyreVoice(car, GetAngleDistance(car->bodyYaw, car->headingAngle));
}
