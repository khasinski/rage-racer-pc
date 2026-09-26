#include "game/car_collision_internal.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void) {
    const CarHullPoint hull[4] = {{-32,64},{32,64},{-32,-64},{32,-64}};
    GameCarRuntime a = {0}, b = {0};
    a.speed = 100; b.speed = 200;
    a.worldVelocityX = 100; b.worldVelocityX = 200;
    a.acceleration = b.acceleration = 1000;
    GameCarRuntime savedA = a, savedB = b;
    s32 hit = FindRivalContact(&a, hull, &b, hull, 32768);
    CHECK(hit > 0);
    CHECK(memcmp(&a, &savedA, sizeof a) == 0);
    CHECK(memcmp(&b, &savedB, sizeof b) == 0);
    ApplyRivalCollision(&a, &b, hit);
    CHECK(a.collisionFlag == 1 && b.collisionFlag == 1);
    GameCarRuntime repeatA = savedA, repeatB = savedB;
    ApplyRivalCollision(&repeatA, &repeatB, hit);
    CHECK(memcmp(&a, &repeatA, sizeof a) == 0);
    CHECK(memcmp(&b, &repeatB, sizeof b) == 0);
    b = savedB; b.activeFlag = -1;
    CHECK(FindRivalContact(&a, hull, &b, hull, 32768) == 0);
    b = savedB; b.x = 1000;
    CHECK(FindRivalContact(&a, hull, &b, hull, 32768) == 0);
    CHECK(FindRivalContact(NULL, hull, &b, hull, 32768) == 0);
    CHECK(FindRivalContact(&a, hull, &a, hull, 32768) == 0);
    CHECK(FindRivalContact(&a, NULL, &b, hull, 32768) == 0);
    CHECK(FindRivalContact(&a, hull, &b, hull, 0) == 0);
    savedA = a; savedB = b;
    ApplyRivalCollision(&a, &b, 5);
    CHECK(memcmp(&a, &savedA, sizeof a) == 0 && memcmp(&b, &savedB, sizeof b) == 0);
    a = b = (GameCarRuntime){0};
    b.z = b.trackProgress = 100;
    CHECK(FindRivalContact(&a, g_CarCollisionCorners, &b, g_CarCollisionCorners, 32768) > 0);
    CHECK(FindRivalContact(&a, g_OpponentHullCorners, &b, g_OpponentHullCorners, 32768) == 0);
    return 0;
}
