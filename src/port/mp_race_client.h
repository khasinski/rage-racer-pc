#ifndef PORT_MP_RACE_CLIENT_H
#define PORT_MP_RACE_CLIENT_H
#include "port_config.h"

/* Connects to rage-racer-server and runs a real, windowed two-seat race
 * driven entirely by network snapshots (see docs/multiplayer.md, step 3:
 * "the game draws the other car on the received poses"). No local physics
 * prediction in this first version: every seat's pose, including the
 * player's own car, comes from the server. Returns 0 on any failure to
 * connect or load; the caller should treat that like any other startup
 * failure. Runs until the server disconnects, sends a result, or the player
 * closes the window. */
int RunMultiplayerRaceClient(const RagePortConfig *config);

#endif
