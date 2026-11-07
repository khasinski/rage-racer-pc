#include "game/audio.h"
#include "game/car_audio.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/race.h"

enum {
    SKID_CUE_MIN_TIMER = 15,
    STRAIGHT_SLIP_MIN = 768,
    STRAIGHT_SLIP_SPAN = 257,
    CUE_LIGHT_SKID = 0xA,
    CUE_NEAR_SIDE_SKID = 0xB,
    CUE_FAR_SIDE_SKID = 0xC,
    CUE_HEAVY_SKID = 0xD,
};

static int IsStraightSlip(s32 slip) {
    return slip >= STRAIGHT_SLIP_MIN &&
           slip < STRAIGHT_SLIP_MIN + STRAIGHT_SLIP_SPAN;
}

static void PlayPlayerSkidCue(const PlayerCarRuntime *car, s32 skid,
                              s32 slip) {
    int lightTouch;
    int nearSide;

    if (skid < CAR_TRACK_CONTACT_FRONT_LEFT ||
        skid > CAR_TRACK_CONTACT_REAR_RIGHT ||
        WrapSigned16(car->motionTimer) < SKID_CUE_MIN_TIMER) {
        return;
    }
    lightTouch = skid <= CAR_TRACK_CONTACT_FRONT_RIGHT;
    nearSide = skid == CAR_TRACK_CONTACT_FRONT_LEFT ||
               skid == CAR_TRACK_CONTACT_REAR_LEFT;
    if (IsStraightSlip(slip)) {
        if (lightTouch) {
            PlaySoundCue(CUE_LIGHT_SKID);
        } else if (car->speed >= CAR_CONTACT_MIN_SPEED) {
            PlaySoundCue(CUE_HEAVY_SKID);
        }
        return;
    }
    if (nearSide) {
        PlaySoundCue(g_MirrorMode == 0 ? CUE_NEAR_SIDE_SKID
                                      : CUE_FAR_SIDE_SKID);
    } else {
        PlaySoundCue(g_MirrorMode == 0 ? CUE_FAR_SIDE_SKID
                                      : CUE_NEAR_SIDE_SKID);
    }
}


void PlayPlayerContactCue(const PlayerCarRuntime *car, s32 skid, s32 slip, int audible) {
    if (audible && slip >= 0) {
        PlayPlayerSkidCue(car, skid, slip);
    }
}
