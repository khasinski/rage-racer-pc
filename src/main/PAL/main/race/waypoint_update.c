#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"

static s32 IsPlayerNearWaypoint(const TrackWaypointRuntime *waypoint) {
    return waypoint->motion.x > g_PlayerCar.x - 0x40 &&
           waypoint->motion.x < g_PlayerCar.x + 0x40 &&
           waypoint->motion.y > g_PlayerCar.z - 0x40 &&
           waypoint->motion.y < g_PlayerCar.z + 0x40;
}

static void CollectWaypoint(TrackWaypointRuntime *waypoint) {
    s32 velocityX;
    s32 velocityY;

    g_WaypointsCollected++;
    PlaySoundCue(0xA);
    waypoint->active = 1;
    waypoint->motion.velocity.vector = g_PlayerVelocity[0];
    waypoint->motion.velocity.fields.x *= 2;
    waypoint->motion.velocity.fields.y *= 2;
    velocityX = waypoint->motion.velocity.fields.x;
    velocityY = waypoint->motion.velocity.fields.y;
    waypoint->motion.velocityMagnitude =
        (velocityX * velocityX + velocityY * velocityY) / 0x2000;
}

static void UpdateCollectedWaypoint(TrackWaypointRuntime *waypoint) {
    waypoint->motion.x += waypoint->motion.velocity.fields.x / 0x100;
    waypoint->motion.y += waypoint->motion.velocity.fields.y / 0x100;
    waypoint->motion.velocity.fields.x =
        waypoint->motion.velocity.fields.x * 15 / 16;
    waypoint->motion.velocity.fields.y =
        waypoint->motion.velocity.fields.y * 15 / 16;
    waypoint->motion.rotationY += waypoint->motion.velocityMagnitude / 0x100;
    waypoint->motion.velocityMagnitude =
        waypoint->motion.velocityMagnitude * 15 / 16;

    if (waypoint->motion.rotationZ < 0x400) {
        waypoint->motion.rotationZ += 0x80;
    } else {
        waypoint->motion.rotationZ = 0x400;
    }

    if (waypoint->motion.velocity.fields.x == 0 &&
        waypoint->motion.velocity.fields.y == 0 &&
        waypoint->motion.velocityMagnitude == 0) {
        waypoint->active = 2;
    }
}

void UpdateWaypoints(void) {
    s32 index;

    for (index = 0; index < 6; index++) {
        TrackWaypointRuntime *waypoint = &g_Waypoints[index];

        if (waypoint->active == 0 && IsPlayerNearWaypoint(waypoint)) {
            CollectWaypoint(waypoint);
        } else if (waypoint->active == 1) {
            UpdateCollectedWaypoint(waypoint);
        }
    }
}
