#include "game/car.h"
#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/audio.h"
#include "game/state.h"
#include "game/track.h"

enum {
    COLLISION_SOUND_TIMER_LIMIT = 0xB,
    COLLISION_SOUND_CLOSE_LATERAL_DISTANCE = 30,
};

static void PlayPlayerCollisionSound(const PlayerCarRuntime *player,
                                     const CarContact *hit) {
    s32 soundCue;

    if (WrapSigned16(player->motionTimer) >= COLLISION_SOUND_TIMER_LIMIT ||
        g_RacePhase >= RACE_PHASE_UNOBSERVED) {
        return;
    }
    if (hit->lateralDistance < COLLISION_SOUND_CLOSE_LATERAL_DISTANCE) {
        soundCue = hit->region >= 3 ? 0xD : 0xA;
    } else {
        soundCue = (hit->region & 1) != g_MirrorMode ? 0xB : 0xC;
    }
    PlaySoundCue(soundCue);
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car) {
    CarCollider field[RACE_CAR_SLOT_COUNT];
    CarContact hit;
    s32 index;

    if (!RaceHasRivals() || g_TrackLength <= 0) {
        return 0;
    }

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].collisionFlag = 0;
        field[index].car = &g_Cars[index];
        field[index].corners = g_OpponentHullCorners;
    }

    hit = FindCarContact(car, g_PlayerHullPoints, field,
                         RACE_CAR_SLOT_COUNT, g_TrackLength);
    if (hit.region <= 0) {
        return hit.region;
    }

    PlayPlayerCollisionSound(car, &hit);
    ApplyCarCollision(car, hit.opponent, hit.region,
                      g_RaceSeries != 0, g_WrongWayTimer);
    return hit.region;
}
