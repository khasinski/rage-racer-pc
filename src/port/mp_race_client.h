#ifndef PORT_MP_RACE_CLIENT_H
#define PORT_MP_RACE_CLIENT_H
#include <stddef.h>
#include <stdint.h>

/* Connects to rage-racer-server and runs a windowed two-human/AI race
 * driven entirely by network snapshots (see docs/multiplayer.md, step 3:
 * "the game draws the other car on the received poses"). No local physics
 * prediction in this first version: every seat's pose, including the
 * player's own car, comes from the server. Returns 0 on failure and 1 on
 * normal completion/cancellation; the caller initializes input and owns
 * return routing. The previous renderer is restored before returning.
 * interactive requires booted menu/font assets and lets the player pick a
 * car/transmission and, for the creator, race settings before ready.
 * Startup commands pass zero and keep the operator's race settings.
 * Runs until the server disconnects, sends a result, or the player
 * closes the window. */
int PortRunMultiplayer(int interactive);

struct MpSettings;
struct MpRaceOptions;
struct MpResult;
struct RaceData;
/* Booted host menu only. Returns 1 confirmed, -1 cancelled, 0 invalid data.
 * These screens own no sockets or race resources. */
int MpSelectCar(struct MpSettings *settings, const struct RaceData *archive, uint32_t automaticCars);
int MpSelectRace(struct MpRaceOptions *options);
int MpSelectRoom(uint64_t *code);
/* Arcade-style cursor text entry. name/hostPort start empty on first use;
 * an empty buffer loads the last saved value (or a default), and a 1
 * return saves the new one. capacity bounds the editable width. */
int MpEnterName(char *name, size_t capacity);
int MpEnterServer(char *hostPort, size_t capacity);
struct MpClient;
struct MpStart;
/* Connected waiting screen: Ready toggle, authoritative Start or cancellation. */
int MpWaitRoom(struct MpClient *client, int seat, struct MpStart *start,
               struct MpSettings *settings, uint32_t automaticCars);
int MpBrowseRooms(struct MpClient *client, uint64_t *code);
/* Caller validates the authoritative result and releases its queued GPU frame. */
void MpShowResults(const struct MpResult *result, int localSeat);
void MpShowError(const char *message);
struct RaceSim;
int MpHudText(const struct RaceSim *race, int seat, int speed,
               const char *units, char rows[2][64]);

#endif
