#include "game/car_collision_internal.h"
#include <stddef.h>


DriverContact FindDriverContact(const PlayerCarRuntime *first, const DriverHull *firstHull,
                                  const PlayerCarRuntime *second, const DriverHull *secondHull,
                                  s32 trackLength) {
    const DriverContact none = {0};
    if (first == NULL || second == NULL || first == second ||
        firstHull == NULL || secondHull == NULL ||
        firstHull->points == NULL || firstHull->corners == NULL ||
        secondHull->points == NULL || secondHull->corners == NULL ||
        first->activeFlag == -1 || second->activeFlag == -1 || trackLength <= 0) {
        return none;
    }
    PlayerCarRuntime a = *first;
    PlayerCarRuntime b = *second;
    const CarCollider secondField = {.car = AsRivalCar(&b), .corners = secondHull->corners};
    const CarCollider firstField = {.car = AsRivalCar(&a), .corners = firstHull->corners};
    const CarContact hitA = FindCarContact(&a, firstHull->points, &secondField, 1, trackLength);
    const CarContact hitB = FindCarContact(&b, secondHull->points, &firstField, 1, trackLength);
    return (DriverContact){.firstRegion = hitA.region, .secondRegion = hitB.region};
}

/* Each human receives the player response from the same pre-impact state.
 * Opponent-only AI changes (boost timer and speed halving) do not replace a
 * human drivetrain. Both drivers receive crash losses in FinishDriver. */
void ApplyDriverCollision(PlayerCarRuntime *first, PlayerCarRuntime *second,
                           s32 firstRegion, s32 secondRegion, int reverse,
                           s32 firstWrongWay, s32 secondWrongWay) {
    if (first == NULL || second == NULL || first == second ||
        firstRegion < 0 || firstRegion > CAR_COLLISION_QUAD_COUNT ||
        secondRegion < 0 || secondRegion > CAR_COLLISION_QUAD_COUNT ||
        (firstRegion == 0 && secondRegion == 0)) {
        return;
    }
    PlayerCarRuntime firstResult = *first;
    PlayerCarRuntime secondResult = *second;
    GameCarRuntime firstOpponent = *AsRivalCar(first);
    GameCarRuntime secondOpponent = *AsRivalCar(second);
    /* Human motion is stored in drive; AI normally writes these base fields. */
    firstOpponent.worldVelocityX = first->drive.accelPos;
    firstOpponent.worldVelocityZ = first->drive.brakePos;
    secondOpponent.worldVelocityX = second->drive.accelPos;
    secondOpponent.worldVelocityZ = second->drive.brakePos;
    ApplyCarCollision(&firstResult, &secondOpponent, firstRegion,
                       reverse, firstWrongWay);
    ApplyCarCollision(&secondResult, &firstOpponent, secondRegion,
                       reverse, secondWrongWay);
    firstResult.collisionFlag = 1;
    secondResult.collisionFlag = 1;
    *first = firstResult;
    *second = secondResult;
}
