#include "game/audio.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/track.h"
#include "game/player_car_internal.h"

void SeedWaypoints(void) {
    TrackWaypointRuntime *waypoint;
    s32 i;
    TrackWaypointSeed *seed;
    s32 track;

    track = g_PlayerCar.lap - 1;
    track = track % 10;
    if (track < 0) {
        track = 0;
    } else if (track >= 9) {
        track = 9;
    }

    waypoint = g_Waypoints;
    seed = &g_WaypointSeeds[track];

    for (i = 0; i < 6; i++) {
        waypoint->active = 0;
        waypoint->motion.x = seed->x + seed->stepX * i;
        waypoint->motion.y = seed->y + seed->stepY * i;
        waypoint->motion.height = 0x1766;
        waypoint->motion.rotationY = 0x174;
        waypoint->motion.rotationZ = 0;
        waypoint->motion.field1C = 0;
        waypoint++;
    }

}
