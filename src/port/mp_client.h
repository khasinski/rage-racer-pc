#ifndef PORT_MP_CLIENT_H
#define PORT_MP_CLIENT_H
#include "game/car_control.h"
#include <stddef.h>
#include <stdint.h>

/* Client side of the wire protocol rage-racer-server (server/src/main.rs)
 * implements, see docs/multiplayer.md. Small and binary, one TCP connection,
 * little-endian. The headless test client uses this adapter too. Keep it
 * in sync with server/src/main.rs's `mod wire`. */
enum {
    MP_PROTOCOL_VERSION = 9,
    MP_SEAT_LIMIT = 2,
    MP_START_BODY_SIZE = 26 + MP_SEAT_LIMIT * 6 + 16,
    MP_NAME_CAPACITY = 15,
    MP_C2S_HELLO = 0x01,
    MP_C2S_INPUT = 0x02,
    MP_C2S_LOADED = 0x03,
    MP_S2C_WELCOME = 0x81,
    MP_S2C_START = 0x82,
    MP_S2C_SNAPSHOT = 0x83,
    MP_S2C_RESULT = 0x84,
    MP_RESULT_BODY_SIZE = MP_SEAT_LIMIT * 6,
    MP_INPUT_WIRE_SIZE = 1 + 12,
    MP_SNAPSHOT_HEADER_SIZE = 4 + 1,
    MP_SNAPSHOT_SEAT_SIZE = 1 + 16 * 4
};

typedef struct MpSeat {
    uint8_t model, manual;
    uint32_t seed;
} MpSeat;

typedef struct MpStart {
    uint8_t course, classIndex, laps, reverse;
    uint32_t countdown;
    char boot[16];
    uint64_t fingerprint, executable;
    MpSeat seats[MP_SEAT_LIMIT];
} MpStart;

typedef enum MpCarStatus { MP_RETIRED, MP_DRIVING, MP_FINISHED } MpCarStatus;

typedef struct MpCarPose {
    int status;
    int32_t x, y, z, yaw;
    int32_t pitch, roll, steering, wheels, brake, progress;
    int32_t rpm, throttle, clutch, gear;
    int32_t ground, rollSpeed;
} MpCarPose;

typedef struct MpFinish {
    int finished, place;
    int32_t milliseconds;
} MpFinish;
typedef struct MpResult { MpFinish seats[MP_SEAT_LIMIT]; } MpResult;

typedef struct MpSnapshot {
    uint32_t tick;
    uint8_t phase;
    MpCarPose seats[MP_SEAT_LIMIT];
} MpSnapshot;

/* Pure wire encode/decode, independent of sockets: testable headlessly.
 * MpEncodeInput always succeeds (input is a bounded native struct);
 * MpDecodeSnapshot rejects invalid size, phase or flags without touching *out. */
void MpEncodeInput(const DriverInput *input, uint8_t out[MP_INPUT_WIRE_SIZE]);
/* Exact versioned start body; invalid data preserves the destination. */
int MpDecodeStart(const uint8_t *body, size_t size, MpStart *out);
struct RaceData;
int MpMatchesArchive(const MpStart *start, const struct RaceData *archive);
struct RaceSetup;
/* Build the server-selected field without importing assets or game globals.
 * Invalid setup preserves the destination; unspecified seats remain empty. */
int MpBuildSetup(const MpStart *start, struct RaceSetup *out);
int MpDecodeResult(const uint8_t *body, size_t size, MpResult *out);
int MpDecodeSnapshot(const uint8_t *body, size_t size, MpSnapshot *out);
/* Presentation only: blend spatial fields, with fraction 0..65536.
 * Controls/status remain those of the newer pose. No extrapolation; invalid
 * poses/fraction preserve output. Terminal transitions use the newer pose. */
int MpBlendPose(const MpCarPose *before, const MpCarPose *after,
                uint32_t fraction, MpCarPose *out);

struct RaceSim;
/* Apply prototype poses to an already configured field, without stepping physics.
 * Reject stale/regressing or invalid snapshots before changing any seat. */
int MpApplySnapshot(struct RaceSim *race, const MpSnapshot *snapshot);

/* Opaque socket handle. NULL on failure; MpClientClose accepts NULL. */
typedef struct MpClient MpClient;

MpClient *MpClientConnect(const char *host, uint16_t port);
void MpClientClose(MpClient *client);
/* name is truncated to MP_NAME_CAPACITY bytes. */
int MpClientSendHello(MpClient *client, const char *name);
/* Blocks for the server's seat assignment. */
int MpClientRecvWelcome(MpClient *client, int *seat);
/* Blocks for the server's race parameters. */
int MpClientRecvStart(MpClient *client, MpStart *start);
/* Call only after the server-selected race and presentation resources are ready. */
int MpClientSendLoaded(MpClient *client);
/* Race-only, nonblocking: accepts the newest controls and retains pending
 * gear edges. Keeps at most an in-flight packet and one latest packet.
 * Returns 1 when accepted (not necessarily sent), 0 on failure. Switches the
 * socket to nonblocking; thereafter use only PollInput/PollMessage and Close. */
int MpClientPollInput(MpClient *client, const DriverInput *input);
int MpClientSendInput(MpClient *client, const DriverInput *input);
/* Blocks for one message; returns 0 on disconnect, 1 for a snapshot (written
 * to *out), 2 for the race result written to *result (stop calling after it). */
int MpClientRecvMessage(MpClient *client, MpSnapshot *out, MpResult *result);
/* Nonblocking receive: latest of up to 32 queued snapshots, plus 3 while
 * incomplete. Final snapshot precedes the result. Malformed/regressing input
 * preserves output. Retains partial messages; one caller owns receive. */
int MpClientPollMessage(MpClient *client, MpSnapshot *out, MpResult *result);

#endif
