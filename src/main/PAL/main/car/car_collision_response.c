#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"

#include <stddef.h>

enum {
    RELATIVE_COLLISION_VELOCITY_DIVISOR = 0x20,
    WRONG_WAY_COLLISION_MINIMUM_SPEED = 0x51,
    WRONG_WAY_COLLISION_MINIMUM_FRAMES = 0xA,
    COLLISION_TORQUE_RETENTION_PERCENT = 0x50,
    HARD_COLLISION_SPEED_DIFFERENCE = 0x191,
    HARD_COLLISION_GRIP_LOSS_FRAMES = 0x1E,
    NORMAL_COLLISION_GRIP_LOSS_FRAMES = 0xF,
    STATIONARY_COLLISION_SPEED = 0x29,
};

static CarCollisionPoint GetCollisionVelocity(
    const PlayerCarRuntime *player, const GameCarRuntime *opponent,
    s32 includeOpponentMotion) {
    CarCollisionPoint velocity;
    s32 x = WrapSigned16((u16)opponent->worldVelocityX -
                         (u16)player->drive.accelPos);
    s32 z = WrapSigned16((u16)opponent->worldVelocityZ -
                         (u16)player->drive.brakePos);

    velocity.x = x / RELATIVE_COLLISION_VELOCITY_DIVISOR;
    velocity.z = z / RELATIVE_COLLISION_VELOCITY_DIVISOR;
    if (includeOpponentMotion) {
        velocity.x = WrapSigned16(
            (s32)velocity.x - WrapSigned16(opponent->velocityX));
        velocity.z = WrapSigned16(
            (s32)velocity.z - WrapSigned16(opponent->velocityZ));
    }
    return velocity;
}

static s32 IsWrongWayImpact(const PlayerCarRuntime *player, int reverse,
                             s32 wrongWayFrames) {
    return player->facingBackwards != reverse &&
           player->speed >= WRONG_WAY_COLLISION_MINIMUM_SPEED &&
           wrongWayFrames >= WRONG_WAY_COLLISION_MINIMUM_FRAMES;
}

static void ApplyLowRegionCollision(PlayerCarRuntime *player,
                                    GameCarRuntime *opponent, int reverse,
                                    s32 wrongWayFrames) {
    CarCollisionPoint velocity = GetCollisionVelocity(player, opponent, 0);

    if (player->facingBackwards != reverse) {
        player->drive.drivetrainTorque = 0;
        player->acceleration = 0;
    } else {
        player->acceleration /= 2;
        player->drive.drivetrainTorque = WrapSigned32(
            (int64_t)player->drive.drivetrainTorque *
            COLLISION_TORQUE_RETENTION_PERCENT) / 100;
    }
    player->drive.gripLossTimer =
        WrapSigned32((int64_t)player->speed - opponent->speed) >=
                HARD_COLLISION_SPEED_DIFFERENCE
            ? HARD_COLLISION_GRIP_LOSS_FRAMES
            : NORMAL_COLLISION_GRIP_LOSS_FRAMES;

    if (IsWrongWayImpact(player, reverse, wrongWayFrames)) {
        SetCarCollisionKnockback(opponent, 0, 0);
        SetCarCollisionKnockback(AsRivalCar(player), 0, 0);
        return;
    }
    if (player->speed >= STATIONARY_COLLISION_SPEED) {
        SetCarCollisionKnockback(AsRivalCar(player), 0, 0);
    } else {
        SetCarCollisionKnockback(AsRivalCar(player), -velocity.x,
                                 -velocity.z);
    }
    SetCarCollisionKnockback(opponent, velocity.x, velocity.z);
}

static void ApplyHighRegionCollision(PlayerCarRuntime *player,
                                     GameCarRuntime *opponent, int reverse,
                                    s32 wrongWayFrames) {
    CarCollisionPoint velocity;

    opponent->speed /= 2;
    opponent->acceleration /= 2;
    opponent->boostTimer = opponent->collisionBoostDuration;
    velocity = GetCollisionVelocity(player, opponent, 1);
    if (IsWrongWayImpact(player, reverse, wrongWayFrames)) {
        SetCarCollisionKnockback(opponent, 0, 0);
        SetCarCollisionKnockback(AsRivalCar(player), 0, 0);
    } else {
        SetCarCollisionKnockback(AsRivalCar(player), -velocity.x,
                                 -velocity.z);
        SetCarCollisionKnockback(opponent, 0, 0);
    }
}

void ApplyCarCollision(PlayerCarRuntime *car, GameCarRuntime *opponent,
                        s32 region, int reverse, s32 wrongWayFrames) {
    if (region < 1 || region > CAR_COLLISION_QUAD_COUNT ||
        car == NULL || opponent == NULL || opponent == AsRivalCar(car)) {
        return;
    }
    car->drive.gripLossTimer = 0;
    if (region <= LAST_FRONT_COLLISION_REGION) {
        ApplyLowRegionCollision(car, opponent, reverse, wrongWayFrames);
    } else {
        ApplyHighRegionCollision(car, opponent, reverse, wrongWayFrames);
    }
    opponent->collisionFlag = 1;
}
