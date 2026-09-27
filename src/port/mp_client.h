#ifndef PORT_MP_CLIENT_H
#define PORT_MP_CLIENT_H
#include "game/car_control.h"
#include "game/driver.h"
#include "game/race_sim.h"
#include "game/car_asset.h"
#include <stddef.h>
#include <stdint.h>
#define MP_BROWSE_ROOM (UINT64_MAX - 1) /* Local menu choice, never sent on wire. */

/* Client side of the wire protocol rage-racer-server (server/src/main.rs)
 * implements, see docs/multiplayer.md. Small and binary, one TCP connection,
 * little-endian. The headless test client uses this adapter too. Keep it
 * in sync with server/src/main.rs's `mod wire`. */
enum {
    MP_PROTOCOL_VERSION = 26,
    MP_SEAT_LIMIT = 2,
    MP_FIELD_LIMIT = DRIVER_SEAT_LIMIT,
    MP_AI_LIMIT = MP_FIELD_LIMIT - MP_SEAT_LIMIT,
    MP_START_BODY_SIZE = 26 + MP_SEAT_LIMIT * 6 + 16 + MP_AI_LIMIT * 7,
    MP_NAME_CAPACITY = 15,
    MP_ROOM_LIMIT = 16,
    MP_C2S_HELLO = 0x01,
    MP_C2S_INPUT = 0x02,
    MP_C2S_LOADED = 0x03,
    MP_C2S_READY = 0x04,
    MP_C2S_PICK = 0x05,
    MP_C2S_RACE = 0x06,
    MP_C2S_ROOM = 0x07,
    MP_C2S_LIST = 0x08,
    MP_C2S_COMMAND = 0x09,
    MP_S2C_WELCOME = 0x81,
    MP_S2C_START = 0x82,
    MP_S2C_SNAPSHOT = 0x83,
    MP_S2C_RESULT = 0x84,
    MP_S2C_LIST = 0x85,
    MP_S2C_LOBBY = 0x86,
    MP_RESULT_BODY_SIZE = MP_SEAT_LIMIT * 6,
    MP_INPUT_WIRE_SIZE = 1 + 12,
    MP_COMMAND_WIRE_SIZE = 1 + 4 + 12,
    MP_SNAPSHOT_HEADER_SIZE = 4 + 4 + 1,
    MP_SNAPSHOT_SEAT_SIZE = 1 + 19 * 4,
    MP_SNAPSHOT_BODY_SIZE = MP_SNAPSHOT_HEADER_SIZE + MP_FIELD_LIMIT * MP_SNAPSHOT_SEAT_SIZE + MP_SEAT_LIMIT * 4
};

typedef struct MpSeat {
    uint8_t model, manual;
    uint32_t seed;
} MpSeat;
typedef struct MpRival {
    uint8_t active, model, slot;
    uint32_t seed;
} MpRival;

typedef struct MpSettings { int port, car, manual; } MpSettings;
typedef struct MpRaceOptions { uint8_t classIndex, course, laps, reverse; } MpRaceOptions;
typedef struct MpRoomInfo {
    uint64_t code;
    MpRaceOptions options;
    uint8_t occupied, ready, state;
} MpRoomInfo;
typedef struct MpLobbySeat { uint8_t variant, manual, length; char name[16]; } MpLobbySeat;
typedef struct MpLobby { MpRoomInfo room; MpLobbySeat seats[MP_SEAT_LIMIT]; } MpLobby;
int MpDecodeLobby(const uint8_t *wire, size_t size, MpLobby *lobby);
/* Complete list packet, including type/version/count. Atomic on rejection. */
int MpDecodeRoomList(const uint8_t *wire, size_t size,
                     MpRoomInfo rooms[MP_ROOM_LIMIT], size_t *count);
int MpValidRaceOptions(const MpRaceOptions *options);
/* Edit class/course/laps/reverse (field 0..3), wrapping bounded values. */
int MpChangeRace(MpRaceOptions *options, int field, int direction);
/* Missing/"auto": match any open room; "create"/0: own a new room.
 * Numeric codes select an existing room. Invalid text preserves output. */
int MpParseRoom(const char *value, uint64_t *code);
/* Welcome carries the assigned room, never the auto/create request sentinel. */
int MpDecodeWelcome(const uint8_t wire[11], int *seat, uint64_t *room);
/* NULL values use defaults (7878, car 0, AT). Explicit invalid values fail
 * atomically; they must not silently select a different server or car. */
int MpParseSettings(const char *port, const char *car, const char *manual,
                    MpSettings *out);
/* Picker state only. Direction -1/0/1 wraps the 32 variants; bits describe
 * retail AT availability. Manual-only variants stay MT. Invalid input is atomic. */
int MpChangeCar(MpSettings *choice, int direction, int toggle, uint32_t automaticCars);

typedef struct MpStart {
    uint8_t course, classIndex, laps, reverse;
    uint32_t countdown;
    char boot[16];
    uint64_t fingerprint, executable;
    MpSeat seats[MP_SEAT_LIMIT];
    MpRival rivals[MP_AI_LIMIT];
} MpStart;

typedef enum MpCarStatus { MP_RETIRED, MP_DRIVING, MP_FINISHED } MpCarStatus;

typedef struct MpCarPose {
    int status;
    int32_t x, y, z, yaw;
    int32_t pitch, roll, steering, wheels, brake, progress;
    int32_t rpm, throttle, clutch, gear;
    int32_t ground, rollSpeed, speed, lap, place;
} MpCarPose;

typedef struct MpFinish {
    int finished, place;
    int32_t milliseconds;
} MpFinish;
typedef struct MpResult { MpFinish seats[MP_SEAT_LIMIT]; } MpResult;

typedef struct MpSnapshot {
    uint32_t tick, elapsed;
    uint8_t phase;
    MpCarPose seats[MP_FIELD_LIMIT];
    uint32_t acknowledged[MP_SEAT_LIMIT];
} MpSnapshot;

/* Protocol 24 publishes a correction and its snapshot together. */
enum { MP_S2C_CORRECTION = 0x87, MP_CORRECTION_VERSION = 1,
       MP_CORRECTION_WIRE_SIZE = 2 + MP_SEAT_LIMIT * 4 + RACE_FRAME_WIRE_SIZE };
typedef struct MpCorrection {
    RaceFrame frame;
    uint32_t acknowledged[MP_SEAT_LIMIT];
} MpCorrection;
/* Complete packet, bound to the receiver's owned track/events. Rejection
 * preserves output. The caller must separately check acknowledgements against
 * its transmitted command history before restoring or pruning anything. */
int MpEncodeCorrection(const RaceSim *race, const uint32_t acknowledged[MP_SEAT_LIMIT],
                       uint8_t *wire, size_t size);
int MpDecodeCorrection(const RaceSim *race, const uint8_t *wire, size_t size,
                       MpCorrection *out);
enum { MP_PUBLICATION_WIRE_SIZE = MP_CORRECTION_WIRE_SIZE + 1 + MP_SNAPSHOT_BODY_SIZE };
/* Correction followed by its snapshot, decoded and cross-checked as one unit.
 * Both outputs stay unchanged on rejection. No simulation/history mutation. */
int MpDecodePublication(const RaceSim *race, const uint8_t *wire, size_t size,
                        MpCorrection *correction, MpSnapshot *snapshot);
/* Diagnostic-only pose reader: checks envelope/clock/ack binding, but does not
 * decode physics. A game must use MpDecodePublication with its owned race. */
int MpDecodePublicationSnapshot(const uint8_t *wire, size_t size, MpSnapshot *snapshot);

enum { MP_COMMAND_CAPACITY = 256 };
typedef struct MpCommand { uint32_t sequence, tick; DriverInput input; } MpCommand;
typedef struct MpCommands {
    MpCommand entries[MP_COMMAND_CAPACITY];
    uint32_t sent, acknowledged, lastTick;
    unsigned head, count;
} MpCommands;
/* Zero-initialize per connection. Retain only complete transmitted commands.
 * Full history refuses another command; it never drops unacknowledged input.
 * Tick is the local mapped server tick when sampled; zero is allowed for the
 * diagnostic client before any mapped clock. Ticks cannot regress, even after
 * the ring empties. Acknowledging a sent sequence removes its prefix atomically. */
int MpRememberCommand(MpCommands *commands, uint32_t sequence, uint32_t tick, const DriverInput *input);
int MpAcknowledgeCommands(MpCommands *commands, uint32_t sequence);
const MpCommand *MpCommandAt(const MpCommands *commands, unsigned index);
/* Replay cursor starts at commands->acknowledged after each correction.
 * Input starts with the checkpoint's held/pending controls. Consume due command
 * levels/edges once, in sequence order; future commands remain pending. The
 * caller passes the simulation's input back after stepping (which clears used
 * gear edges). Invalid history/cursor/input preserves both outputs. */
int MpReplayInput(const MpCommands *commands, uint32_t tick,
                   uint32_t *sequence, DriverInput *input);
/* Rebuild bounded prediction from authority in caller-owned scratch storage.
 * Immutable route/events are borrowed from authority, which must outlive output.
 * Source/history remain unchanged. On failure discard output. Never alias source.
 * Pending samples come from MpClientPendingCommands, not acknowledged history. */
int MpPredictRace(const RaceSim *authority, const MpCommands *commands,
                   const MpCommand pending[2], unsigned count, int seat,
                   uint32_t target, RaceSim *out);
/* Host/server clock mapping for fixed-step prediction. Own per race and supply
 * monotonic nanoseconds explicitly. Observations retain the origin unless the
 * server has overtaken it; rendering frequency never determines tick rate. */
typedef struct MpClock {
    uint64_t origin, observedAt;
    uint32_t originTick, latestTick;
    int started;
} MpClock;
enum { MP_PREDICTION_LEAD = 10 }; /* Freeze at 200 ms without fresh authority. */
int MpClockObserve(MpClock *clock, uint32_t tick, uint64_t now);
/* Optional presentation fraction is 0..65535 between target and target+1.
 * At the prediction lead/overflow limit it is zero: stalls never extrapolate. */
int MpClockTarget(const MpClock *clock, uint64_t now, uint32_t *tick, uint32_t *fraction);
/* Convert a 50 Hz clock fraction to the next actual motion tick. Terminal
 * drivers and the authority lead limit freeze. Invalid input preserves outputs. */
int MpMotionTarget(const RaceSim *prediction, uint32_t authorityTick, int seat,
                    uint32_t clockFraction, uint32_t *tick, uint32_t *fraction);
/* Apply a decoded authoritative checkpoint and prune only the local seat's
 * confirmed prefix. Both destinations stay unchanged on rejection. This does
 * not replay pending commands or change the presentation/host clock. */
int MpApplyCorrection(RaceSim *race, MpCommands *commands, int seat,
                      const MpCorrection *correction);

enum { MP_HISTORY_CAPACITY = 8 };
typedef struct MpHistory {
    MpSnapshot samples[MP_HISTORY_CAPACITY];
    uint64_t origin, receivedAt;
    uint32_t firstTick;
    unsigned count;
} MpHistory;
/* Zero-initialize per connection. Fixed-capacity presentation history; host
 * time is supplied in nanoseconds. New packets do not restart playback time.
 * Invalid/regressing samples preserve history. Pose clamps to retained samples,
 * with caller-selected delay; it never extrapolates or changes simulation. */
int MpHistoryPush(MpHistory *history, const MpSnapshot *snapshot, uint64_t now);
int MpHistoryPose(const MpHistory *history, int seat, uint64_t now,
                  uint64_t delay, MpCarPose *out);

/* Pure wire encode/decode, independent of sockets: testable headlessly.
 * MpEncodeInput always succeeds (input is a bounded native struct);
 * MpDecodeSnapshot rejects invalid size, phase or flags without touching *out. */
void MpEncodeInput(const DriverInput *input, uint8_t out[MP_INPUT_WIRE_SIZE]);
void MpEncodeCommand(const DriverInput *input, uint32_t sequence,
                      uint8_t out[MP_COMMAND_WIRE_SIZE]);
/* Exact versioned start body; invalid data preserves the destination. */
int MpDecodeStart(const uint8_t *body, size_t size, MpStart *out);
/* The assigned local seat must retain the car/transmission sent in pick. */
int MpMatchesChoice(const MpStart *start, int seat, const MpSettings *choice);
struct RaceData;
int MpMatchesArchive(const MpStart *start, const struct RaceData *archive);
struct RaceSetup;
/* Build the server-selected field without importing assets or game globals.
 * Invalid setup preserves the destination; unspecified seats remain empty. */
int MpBuildSetup(const MpStart *start, struct RaceSetup *out);
typedef struct MpCarConfig {
    uint8_t variant[MP_SEAT_LIMIT];
    GameCarSpec specs[MP_SEAT_LIMIT];
} MpCarConfig;
enum { MP_CONFIG_WIRE_SIZE = 2 + MP_SEAT_LIMIT * (1 + CAR_SPEC_WIRE_SIZE) };
/* Configuration packet format 1 (type 0x88). Atomic codecs; application is
 * setup-only and must precede Loaded/StartRaceSim and any prediction. */
int MpEncodeConfig(const MpCarConfig *config, uint8_t *wire, size_t size);
int MpDecodeConfig(const uint8_t *wire, size_t size, MpCarConfig *config);
int MpApplyConfig(RaceSim *race, const MpCarConfig *config);
/* 1 complete, 3 pending, 0 terminal failure. Partial bytes never touch config. */
struct MpClient;
int MpClientPollConfig(struct MpClient *client, MpCarConfig *config);
int MpClientRecvConfig(struct MpClient *client, MpCarConfig *config);
enum { MP_AVAILABILITY_WIRE_SIZE = 6 };
int MpEncodeAvailability(uint32_t mask, uint8_t *wire, size_t size);
int MpDecodeAvailability(const uint8_t *wire, size_t size, uint32_t *mask);
int MpClientPollAvailability(struct MpClient *client, uint32_t *mask);
int MpClientRecvAvailability(struct MpClient *client, uint32_t *mask);
int MpDecodeResult(const uint8_t *body, size_t size, MpResult *out);
/* A decoded result must agree with the final authoritative snapshot. */
int MpMatchesResult(const MpSnapshot *snapshot, const MpResult *result);
int MpDecodeSnapshot(const uint8_t *body, size_t size, MpSnapshot *out);
/* Race receive watchdog, using caller-supplied monotonic nanoseconds.
 * Initial loading allows 65 seconds; an established race allows five.
 * Only a complete accepted snapshot should reset receivedAt. */
int MpSnapshotExpired(uint64_t now, uint64_t receivedAt, int started);
/* Presentation only: blend spatial fields, with fraction 0..65536.
 * Controls/status remain those of the newer pose. No extrapolation; invalid
 * poses/fraction preserve output. Terminal transitions use the newer pose. */
int MpBlendPose(const MpCarPose *before, const MpCarPose *after,
                uint32_t fraction, MpCarPose *out);
/* Human prediction presentation only; preserves the newer native model flags.
 * Terminal transitions snap to the newer pose. Invalid input preserves output. */
int MpBlendDriver(const SimDriver *before, const SimDriver *after,
                  uint32_t fraction, PlayerCarRuntime *out);

struct RaceSim;
/* Apply all field poses to an already configured race, without stepping physics.
 * Reject stale/regressing or invalid snapshots before changing any seat. */
int MpApplySnapshot(struct RaceSim *race, const MpSnapshot *snapshot);
/* Copies wire car fields only, selecting human or rival drivetrain storage.
 * Ownership/model/status and race clocks stay intact. AI gearbox/clutch is zero. */
int MpApplyPose(struct PlayerCarRuntime *car, const MpCarPose *pose, int rival);

/* Opaque socket handle. NULL on failure; MpClientClose accepts NULL. */
typedef struct MpClient MpClient;
uint64_t MpClientRoom(const MpClient *client);
const MpCommands *MpClientCommands(const MpClient *client);
/* Copy at most two not-yet-completed writes in order: immutable in-flight
 * command then newest queued controls. Queued sequence is zero until assigned.
 * These are local prediction samples, not entries in acknowledged history.
 * Returns count (0..2); NULL arguments return zero without touching output. */
unsigned MpClientPendingCommands(const MpClient *client, MpCommand out[2]);
const MpLobby *MpClientLobby(const MpClient *client);

MpClient *MpClientConnect(const char *host, uint16_t port);
/* Numeric IPv4 only. Begin never waits for the peer. Poll returns 1 connected,
 * 3 pending or 0 terminal failure, with a five-second connection deadline.
 * Until connected, use only PollConnect and Close. */
MpClient *MpClientBeginConnect(const char *host, uint16_t port);
int MpClientPollConnect(MpClient *client);
void MpClientClose(MpClient *client);
/* name is truncated to MP_NAME_CAPACITY bytes. */
int MpClientSendHello(MpClient *client, const char *name);
int MpClientSendReady(MpClient *client, int ready);
int MpClientSendPick(MpClient *client, int variant, int manual);
/* Waits up to five seconds for the complete seat assignment. */
int MpClientRecvWelcome(MpClient *client, int *seat);
/* Waits up to two minutes for complete race parameters, including waiting for
 * the second seat. Partial packets do not restart either setup deadline.
 * Failure leaves the caller's output unchanged; close the failed session. */
int MpClientRecvStart(MpClient *client, MpStart *start);
/* Nonblocking setup receive: 1 complete, 3 pending, 0 terminal failure.
 * PollStart also returns 2 for a validated lobby update; keep waiting for Start.
 * Welcome allows five seconds. Start waits for room decisions without a timer;
 * its first byte starts an absolute five-second packet deadline. Disconnect
 * still fails immediately. One receive owner; do not
 * mix polling and blocking receive during an incomplete message. */
int MpClientPollWelcome(MpClient *client, int *seat);
int MpClientPollStart(MpClient *client, MpStart *start);
/* Call only after the server-selected race and presentation resources are ready. */
int MpClientSendLoaded(MpClient *client);
/* Nonblocking setup writes: 1 fully sent, 3 pending, 0 terminal failure.
 * Five-second deadline from first call. Hello retains the first supplied name
 * while pending. Stop polling a message after success. Once used, use only
 * polling APIs and Close; do not begin another operation while pending.
 * Ready and Pick may be sent while PollStart is waiting/receiving; complete that
 * write before consuming Start and reporting Loaded. A pending write retains
 * its original value even if a later call supplies another one. */
int MpClientPollHello(MpClient *client, const char *name);
int MpClientPollLoaded(MpClient *client);
int MpClientPollReady(MpClient *client, int ready);
int MpClientPollPick(MpClient *client, int variant, int manual);
/* Seat zero's room settings, sent before Ready. */
int MpClientPollRace(MpClient *client, const MpRaceOptions *options);
/* Sent after Hello, before Welcome. UINT64_MAX means automatic selection. */
int MpClientPollRoom(MpClient *client, uint64_t code);
int MpClientSendRoom(MpClient *client, uint64_t code);
/* Request/receive a bounded directory before choosing a room. 1 complete,
 * 3 pending, 0 failed; five-second receive deadline, atomic output. */
int MpClientPollList(MpClient *client, MpRoomInfo rooms[MP_ROOM_LIMIT], size_t *count);
/* Race-only, nonblocking: accepts the newest controls and retains pending
 * gear edges. Keeps at most an in-flight packet and one latest packet.
 * Returns 1 when accepted (not necessarily sent), 0 on failure. Switches the
 * socket to nonblocking; thereafter use only PollInput/PollMessage and Close. */
int MpClientPollInput(MpClient *client, const DriverInput *input, uint32_t tick);
int MpClientSendInput(MpClient *client, const DriverInput *input);
/* Blocks for one message; returns 0 on disconnect, 1 for a snapshot (written
 * to *out), 2 for the race result written to *result (stop calling after it). */
int MpClientRecvMessage(MpClient *client, MpSnapshot *out, MpResult *result);
/* Nonblocking receive: latest of up to 32 queued snapshots, plus 3 while
 * incomplete. Final snapshot precedes the result. Malformed/regressing input
 * preserves output. Retains partial messages; one caller owns receive. */
int MpClientPollMessage(MpClient *client, MpSnapshot *out, MpResult *result);
/* Correction-capable receive: coalesces up to 32 complete publications, same
 * 0/1/2/3 codes as PollMessage, plus 4 for an applied checkpoint and its snapshot.
 * Each accepted publication atomically changes race/history after validation;
 * a later malformed publication makes the connection terminal. Optional outputs
 * receive the newest accepted state only on success. Final state precedes results.
 * Never mix receive APIs while a packet is partial. */
int MpClientPollRaceState(MpClient *client, RaceSim *race, MpSnapshot *out,
                          MpResult *result, MpCorrection *correction);

#endif
