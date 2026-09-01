#ifndef GAME_SHOWROOM_INTERNAL_H
#define GAME_SHOWROOM_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/vector.h"

typedef struct ShowroomCarPose {
    s32 position[4];
    s32 unk10[4];
    Vec4 rotation;
} ShowroomCarPose;

typedef union ShowroomPlayerCarState {
    ShowroomCarPose pose;
    s32 courseViewX;
    PlayerCarRuntime runtime;
} ShowroomPlayerCarState;

/*
 * The showroom reads the player car's storage as a pose rather than as a car.
 *
 * This header used to say `extern ShowroomPlayerCarState g_PlayerCar;` while
 * player_car_internal.h said `extern PlayerCarRuntime g_PlayerCar;`. One symbol
 * with two types is undefined, and in practice it meant the two headers could
 * not be included in the same translation unit at all. The union already ends
 * in a PlayerCarRuntime, so this is the same storage seen a second way.
 */
#include "game/player_car_internal.h"

static inline ShowroomPlayerCarState *ShowroomPlayerCar(void) {
    return (ShowroomPlayerCarState *)&g_PlayerCar;
}

static inline struct GameRenderObject *ShowroomRenderObject(void) {
    return GetCarRenderObject(GetPlayerCarRuntime(&g_PlayerCar));
}

#endif
