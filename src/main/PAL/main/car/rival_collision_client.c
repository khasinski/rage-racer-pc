#include "game/car_collision_internal.h"
#include "game/track.h"

s32 CollideRivalCars(s32 index) {
    if (index < 0 || index >= RACE_CAR_SLOT_COUNT - 1) return 0;
    for (s32 next = index + 1; next < RACE_CAR_SLOT_COUNT; next++) {
        s32 hit = FindRivalContact(&g_Cars[index], g_CarCollisionCorners,
            &g_Cars[next], g_CarCollisionCorners, g_TrackLength);
        if (hit > 0) {
            ApplyRivalCollision(&g_Cars[index], &g_Cars[next], hit);
            return hit;
        }
    }
    return 0;
}
