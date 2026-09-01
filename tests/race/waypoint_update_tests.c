#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"

#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
TrackWaypointRuntime g_Waypoints[6];
s32 g_WaypointsCollected;

static s32 s_lastSound = -1;
static s32 s_soundCount;
static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

void *GetPlayerCarStorage(void) {
    return &g_PlayerCar;
}

void PlaySoundCue(s32 soundId) {
    s_lastSound = soundId;
    s_soundCount++;
}

static void ResetState(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(g_Waypoints, 0, sizeof(g_Waypoints));
    g_WaypointsCollected = 0;
    s_lastSound = -1;
    s_soundCount = 0;
}

static void CheckCollection(void) {
    TrackWaypointRuntime *waypoint;

    ResetState();
    g_PlayerCar.x = 1000;
    g_PlayerCar.z = 2000;
    g_PlayerVelocity[0].x = 0x300;
    g_PlayerVelocity[0].z = -0x200;
    waypoint = &g_Waypoints[2];
    waypoint->motion.x = 1001;
    waypoint->motion.y = 1999;

    UpdateWaypoints();

    CHECK_EQ(waypoint->active, 1, "collected state");
    CHECK_EQ(g_WaypointsCollected, 1, "collected count");
    CHECK_EQ(s_soundCount, 1, "collection sound count");
    CHECK_EQ(s_lastSound, 0xA, "collection sound");
    CHECK_EQ(waypoint->motion.velocity.fields.x, 0x600,
             "horizontal launch velocity");
    CHECK_EQ(waypoint->motion.velocity.fields.y, -0x400,
             "vertical launch velocity");
    CHECK_EQ(waypoint->motion.velocityMagnitude,
             (0x600 * 0x600 + 0x400 * 0x400) / 0x2000,
             "launch spin");
}

static void CheckStrictBoundary(void) {
    ResetState();
    g_PlayerCar.x = 1000;
    g_PlayerCar.z = 2000;
    g_Waypoints[0].motion.x = 1000 + 0x40;
    g_Waypoints[0].motion.y = 2000;
    UpdateWaypoints();
    CHECK_EQ(g_Waypoints[0].active, 0, "strict pickup boundary");
}

static void CheckFlightAndSettling(void) {
    TrackWaypointRuntime *waypoint;

    ResetState();
    waypoint = &g_Waypoints[0];
    waypoint->active = 1;
    waypoint->motion.x = 100;
    waypoint->motion.y = 200;
    waypoint->motion.velocity.fields.x = 0x100;
    waypoint->motion.velocity.fields.y = -0x200;
    waypoint->motion.velocityMagnitude = 0x200;
    waypoint->motion.rotationZ = 0x380;

    UpdateWaypoints();
    CHECK_EQ(waypoint->motion.x, 101, "flight x");
    CHECK_EQ(waypoint->motion.y, 198, "flight y");
    CHECK_EQ(waypoint->motion.velocity.fields.x, 0xF0, "damped x");
    CHECK_EQ(waypoint->motion.velocity.fields.y, -0x1E0, "damped y");
    CHECK_EQ(waypoint->motion.rotationY, 2, "spin rotation");
    CHECK_EQ(waypoint->motion.rotationZ, 0x400, "tilt rotation");
    CHECK_EQ(waypoint->active, 1, "still moving");

    waypoint->motion.velocity.fields.x = 0;
    waypoint->motion.velocity.fields.y = 0;
    waypoint->motion.velocityMagnitude = 0;
    UpdateWaypoints();
    CHECK_EQ(waypoint->active, 2, "settled state");
}

int main(void) {
    CheckCollection();
    CheckStrictBoundary();
    CheckFlightAndSettling();
    if (s_failures != 0) return 1;
    puts("waypoint updates passed");
    return 0;
}
