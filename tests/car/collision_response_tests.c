#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    PlayerCarRuntime initial = {0};
    GameCarRuntime initialOpponent = {0};
    initial.speed = 1000;
    initial.acceleration = 1200;
    initial.drive.drivetrainTorque = 2400;
    initialOpponent.speed = 100;
    initialOpponent.acceleration = 800;
    initialOpponent.worldVelocityX = 64;
    initialOpponent.worldVelocityZ = 32;
    initialOpponent.collisionBoostDuration = 9;
    PlayerCarRuntime front = initial;
    GameCarRuntime frontOpponent = initialOpponent;
    ApplyCarCollision(&front, &frontOpponent, 1, 0, 0);
    CHECK(front.acceleration == 600 && front.drive.drivetrainTorque == 1920);
    CHECK(front.drive.gripLossTimer == 30 && front.motionTimer == 15);
    CHECK(front.velocityX == 0 && front.velocityZ == 0);
    CHECK(frontOpponent.velocityX == 1 && frontOpponent.collisionFlag == 1);

    PlayerCarRuntime rear = initial;
    GameCarRuntime rearOpponent = initialOpponent;
    ApplyCarCollision(&rear, &rearOpponent, 3, 0, 0);
    CHECK(rearOpponent.speed == 50 && rearOpponent.acceleration == 400);
    CHECK(rearOpponent.boostTimer == 9);
    CHECK(rear.velocityX == -1 && rearOpponent.velocityX == 0);
    CHECK(front.acceleration == 600 && frontOpponent.speed == 100);

    PlayerCarRuntime restored = initial;
    GameCarRuntime restoredOpponent = initialOpponent;
    ApplyCarCollision(&restored, &restoredOpponent, 1, 0, 0);
    CHECK(memcmp(&front, &restored, sizeof(front)) == 0);
    CHECK(memcmp(&frontOpponent, &restoredOpponent, sizeof(frontOpponent)) == 0);

    restored = initial;
    restoredOpponent = initialOpponent;
    restored.facingBackwards = 1;
    ApplyCarCollision(&restored, &restoredOpponent, 1, 0, 10);
    CHECK(restored.acceleration == 0 && restored.drive.drivetrainTorque == 0);
    CHECK(restoredOpponent.velocityX == 0 && restored.velocityX == 0);

    restored = initial;
    restored.speed = 40;
    restoredOpponent = initialOpponent;
    ApplyCarCollision(&restored, &restoredOpponent, 1, 0, 0);
    CHECK(restored.velocityX == -1 && restoredOpponent.velocityX == 1);
    CHECK(restored.drive.gripLossTimer == 15);

    restored = initial;
    restoredOpponent = initialOpponent;
    ApplyCarCollision(&restored, &restoredOpponent, 0, 0, 0);
    ApplyCarCollision(&restored, &restoredOpponent, 5, 0, 0);
    ApplyCarCollision(&restored, AsRivalCar(&restored), 1, 0, 0);
    CHECK(memcmp(&restored, &initial, sizeof(initial)) == 0);
    CHECK(memcmp(&restoredOpponent, &initialOpponent, sizeof(initialOpponent)) == 0);
    PlayerCarRuntime humanA = initial;
    PlayerCarRuntime humanB = initial;
    humanA.drive.accelPos = 640;
    humanB.drive.accelPos = 320;
    humanB.speed = 100;
    PlayerCarRuntime swappedA = humanA;
    PlayerCarRuntime swappedB = humanB;
    ApplyDriverCollision(&humanA, &humanB, 1, 3, 0, 0, 0);
    ApplyDriverCollision(&swappedB, &swappedA, 3, 1, 0, 0, 0);
    CHECK(memcmp(&humanA, &swappedA, sizeof(humanA)) == 0);
    CHECK(memcmp(&humanB, &swappedB, sizeof(humanB)) == 0);
    CHECK(humanA.drive.drivetrainTorque == 1920 && humanA.drive.gripLossTimer == 30);
    CHECK(humanB.velocityX == -5 && humanB.drive.drivetrainTorque == initial.drive.drivetrainTorque);
    CHECK(humanA.collisionFlag == 1 && humanB.collisionFlag == 1);
    CHECK(humanB.speed == 100); /* Human crash loss is applied by FinishDriver. */
    for (int firstRegion = 1; firstRegion <= 4; firstRegion++) {
        for (int secondRegion = 1; secondRegion <= 4; secondRegion++) {
            for (int reverse = 0; reverse <= 1; reverse++) {
                humanA = initial;
                humanB = initial;
                humanA.drive.accelPos = 640;
                humanB.drive.brakePos = -320;
                humanB.speed = 40;
                swappedA = humanA;
                swappedB = humanB;
                ApplyDriverCollision(&humanA, &humanB, firstRegion, secondRegion, reverse, 10, 0);
                ApplyDriverCollision(&swappedB, &swappedA, secondRegion, firstRegion, reverse, 0, 10);
                CHECK(memcmp(&humanA, &swappedA, sizeof(humanA)) == 0);
                CHECK(memcmp(&humanB, &swappedB, sizeof(humanB)) == 0);
            }
        }
    }
    swappedA = humanA;
    swappedB = humanB;
    ApplyDriverCollision(&humanA, &humanB, 0, 0, 0, 0, 0);
    ApplyDriverCollision(&humanA, &humanA, 1, 1, 0, 0, 0);
    ApplyDriverCollision(NULL, &humanB, 1, 1, 0, 0, 0);
    CHECK(memcmp(&humanA, &swappedA, sizeof(humanA)) == 0);
    CHECK(memcmp(&humanB, &swappedB, sizeof(humanB)) == 0);
    puts("collision response tests passed");
    return 0;
}
