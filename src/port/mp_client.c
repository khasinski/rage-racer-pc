#include "mp_client.h"
#include "game/race_sim.h"
#include "client_race.h"

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
typedef int IoCount;
#define INVALID_FD INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
typedef int Socket;
typedef ssize_t IoCount;
#define INVALID_FD (-1)
#endif

struct MpClient {
    Socket fd;
    uint8_t incoming[1 + MP_SNAPSHOT_HEADER_SIZE + MP_SEAT_LIMIT * MP_SNAPSHOT_SEAT_SIZE];
    size_t received;
    size_t needed;
    int failed;
    int nonblocking;
    uint8_t outgoing[MP_INPUT_WIRE_SIZE];
    size_t sent;
    int sending;
    DriverInput latest;
    int queued;
    MpResult result;
    int resultPending;
};

static int StartSockets(void) {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 0;
    if (data.wVersion == MAKEWORD(2, 2)) return 1;
    WSACleanup();
    return 0;
#else
    return 1;
#endif
}

static void StopSockets(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void CloseSocket(Socket fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

static int Interrupted(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEINTR;
#else
    return errno == EINTR;
#endif
}

static int WouldBlock(void) {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EAGAIN || errno == EWOULDBLOCK;
#endif
}

static int Nonblocking(MpClient *client) {
    if (client->nonblocking) return 1;
#ifdef _WIN32
    u_long enabled = 1;
    if (ioctlsocket(client->fd, FIONBIO, &enabled) != 0) return 0;
#else
    int flags = fcntl(client->fd, F_GETFL);
    if (flags < 0 || fcntl(client->fd, F_SETFL, flags | O_NONBLOCK) != 0) return 0;
#endif
    client->nonblocking = 1;
    return 1;
}

static void PutLE16(uint8_t *out, int16_t v) {
    out[0] = (uint8_t)v;
    out[1] = (uint8_t)((uint16_t)v >> 8);
}
static uint32_t GetLE32(const uint8_t *in) {
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}
static int32_t GetLE32Signed(const uint8_t *in) { return (int32_t)GetLE32(in); }

void MpEncodeInput(const DriverInput *input, uint8_t out[MP_INPUT_WIRE_SIZE]) {
    out[0] = MP_C2S_INPUT;
    out[1] = (uint8_t)input->steering.mode;
    out[2] = input->steering.left ? 1 : 0;
    out[3] = input->steering.right ? 1 : 0;
    /* Steering angle can exceed +/-32767 in principle; the retail range
     * (+/-13*512, see SetRaceInput) fits comfortably in 16 bits. */
    PutLE16(out + 4, (int16_t)input->steering.angle);
    PutLE16(out + 6, input->throttle);
    PutLE16(out + 8, input->brake);
    out[10] = input->shiftUp ? 1 : 0;
    out[11] = input->shiftDown ? 1 : 0;
    out[12] = 0; /* Reserved byte in the server's 12-byte input body. */
}

static int ValidStart(const MpStart *start) {
    if (!start || start->course >= 4 || start->classIndex >= 6 ||
        start->laps < 1 || start->laps > PLAYER_LAP_TIME_CAPACITY || start->reverse > 1 ||
        !memchr(start->boot, 0, sizeof(start->boot))) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
        if (start->seats[seat].model >= CAR_MODEL_VARIANT_COUNT ||
            start->seats[seat].manual > 1) return 0;
    return 1;
}

int MpDecodeStart(const uint8_t *body, size_t size, MpStart *out) {
    if (!body || !out || size != MP_START_BODY_SIZE ||
        body[0] != MP_PROTOCOL_VERSION || body[9] != MP_SEAT_LIMIT) return 0;
    MpStart result = {.course = body[1], .classIndex = body[2],
        .laps = body[3], .reverse = body[4], .countdown = GetLE32(body + 5)};
    memcpy(result.boot, body + 10, sizeof(result.boot));
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const uint8_t *entry = body + 26 + seat * 6;
        result.seats[seat] = (MpSeat){entry[0], entry[1], GetLE32(entry + 2)};
    }
    const uint8_t *hash = body + 26 + MP_SEAT_LIMIT * 6;
    result.fingerprint = (uint64_t)GetLE32(hash) | ((uint64_t)GetLE32(hash + 4) << 32);
    result.executable = (uint64_t)GetLE32(hash + 8) | ((uint64_t)GetLE32(hash + 12) << 32);
    if (!ValidStart(&result)) return 0;
    *out = result;
    return 1;
}

int MpMatchesArchive(const MpStart *start, const RaceData *archive) {
    return ValidStart(start) && archive && archive->data && archive->size &&
        start->boot[0] && archive->boot[0] &&
        memchr(archive->boot, 0, sizeof(archive->boot)) &&
        strcmp(start->boot, archive->boot) == 0 &&
        start->executable && start->executable == archive->executable &&
        start->fingerprint == ArchiveFingerprint(archive->data, archive->size);
}

int MpBuildSetup(const MpStart *start, RaceSetup *out) {
    if (!out || !ValidStart(start)) return 0;
    RaceSetup setup = {.classIndex = start->classIndex, .courseIndex = start->course,
                       .laps = start->laps, .reverse = start->reverse};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const MpSeat *entry = &start->seats[seat];
        setup.entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = seat,
            .model = entry->model, .manual = entry->manual, .seed = entry->seed};
        setup.looks[seat].variant = entry->model;
    }
    *out = setup;
    return 1;
}

int MpDecodeResult(const uint8_t *body, size_t size, MpResult *out) {
    if (!body || !out || size != MP_RESULT_BODY_SIZE) return 0;
    MpResult result = {0};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const uint8_t *entry = body + seat * 6;
        MpFinish finish = {entry[0], entry[1], GetLE32Signed(entry + 2)};
        if (finish.finished > 1 || (finish.finished
                ? finish.place < 1 || finish.place > DRIVER_SEAT_LIMIT || finish.milliseconds < 0
                : finish.place != 0 || finish.milliseconds != -1)) return 0;
        for (int earlier = 0; earlier < seat; ++earlier)
            if (finish.finished && result.seats[earlier].finished &&
                finish.place == result.seats[earlier].place) return 0;
        result.seats[seat] = finish;
    }
    *out = result;
    return 1;
}

static int ValidPose(const MpCarPose *pose) {
    return pose->status >= 0 && pose->status <= MP_FINISHED && pose->brake >= 0 && pose->brake <= 256 &&
        pose->throttle >= 0 && pose->throttle <= 256 &&
        pose->clutch >= INT16_MIN && pose->clutch <= INT16_MAX &&
        pose->gear >= 0 && pose->gear <= CAR_FORWARD_GEAR_COUNT;
}

static int32_t BlendValue(int32_t before, int32_t after, uint32_t fraction) {
    return (int32_t)(before + ((int64_t)after - before) * fraction / 65536);
}

static int32_t BlendPoseAngle(int32_t before, int32_t after, uint32_t fraction) {
    int64_t delta = ((int64_t)after - before) % 4096;
    if (delta > 2048) delta -= 4096;
    if (delta < -2048) delta += 4096;
    return (int32_t)(((int64_t)before + delta * fraction / 65536) & 4095);
}

int MpBlendPose(const MpCarPose *before, const MpCarPose *after,
                uint32_t fraction, MpCarPose *out) {
    if (!before || !after || !out || fraction > 65536 ||
        !ValidPose(before) || !ValidPose(after)) return 0;
    MpCarPose pose = *after;
    if (before->status == MP_DRIVING && after->status == MP_DRIVING) {
        pose.x = BlendValue(before->x, after->x, fraction);
        pose.y = BlendValue(before->y, after->y, fraction);
        pose.z = BlendValue(before->z, after->z, fraction);
        pose.ground = BlendValue(before->ground, after->ground, fraction);
        pose.steering = BlendValue(before->steering, after->steering, fraction);
        pose.rollSpeed = BlendValue(before->rollSpeed, after->rollSpeed, fraction);
        pose.yaw = BlendPoseAngle(before->yaw, after->yaw, fraction);
        pose.pitch = BlendPoseAngle(before->pitch, after->pitch, fraction);
        pose.roll = BlendPoseAngle(before->roll, after->roll, fraction);
        pose.wheels = BlendPoseAngle(before->wheels, after->wheels, fraction) |
                      (after->wheels & CAR_WHEEL_BLUR_FLAG);
    }
    *out = pose;
    return 1;
}

int MpDecodeSnapshot(const uint8_t *body, size_t size, MpSnapshot *out) {
    if (!body || !out) return 0;
    size_t expect = MP_SNAPSHOT_HEADER_SIZE + MP_SEAT_LIMIT * MP_SNAPSHOT_SEAT_SIZE;
    if (size != expect || body[4] > SIM_FINISHED) return 0;
    MpSnapshot result = {0};
    result.tick = GetLE32(body);
    result.phase = body[4];
    const uint8_t *cursor = body + MP_SNAPSHOT_HEADER_SIZE;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        if (cursor[0] > MP_FINISHED) return 0;
        result.seats[seat].status = cursor[0];
        result.seats[seat].x = GetLE32Signed(cursor + 1);
        result.seats[seat].y = GetLE32Signed(cursor + 5);
        result.seats[seat].z = GetLE32Signed(cursor + 9);
        result.seats[seat].yaw = GetLE32Signed(cursor + 13);
        result.seats[seat].pitch = GetLE32Signed(cursor + 17);
        result.seats[seat].roll = GetLE32Signed(cursor + 21);
        result.seats[seat].steering = GetLE32Signed(cursor + 25);
        result.seats[seat].wheels = GetLE32Signed(cursor + 29);
        result.seats[seat].brake = GetLE32Signed(cursor + 33);
        result.seats[seat].progress = GetLE32Signed(cursor + 37);
        result.seats[seat].rpm = GetLE32Signed(cursor + 41);
        result.seats[seat].throttle = GetLE32Signed(cursor + 45);
        result.seats[seat].clutch = GetLE32Signed(cursor + 49);
        result.seats[seat].gear = GetLE32Signed(cursor + 53);
        result.seats[seat].ground = GetLE32Signed(cursor + 57);
        result.seats[seat].rollSpeed = GetLE32Signed(cursor + 61);
        if (!ValidPose(&result.seats[seat])) return 0;
        cursor += MP_SNAPSHOT_SEAT_SIZE;
    }
    *out = result;
    return 1;
}

int MpApplySnapshot(RaceSim *race, const MpSnapshot *snapshot) {
    if (!race || !snapshot || snapshot->phase > SIM_FINISHED ||
        snapshot->phase < race->phase || snapshot->tick <= race->tick) return 0;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        const MpCarPose *pose = &snapshot->seats[seat];
        if (!ValidPose(pose) || driver->rival ||
            driver->status == SIM_EMPTY ||
            (pose->status != MP_RETIRED && driver->status == SIM_RETIRED) ||
            (driver->status == SIM_DRIVER_FINISHED && pose->status != MP_FINISHED)) return 0;
    }
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        SimDriver *driver = &race->drivers[seat];
        const MpCarPose *pose = &snapshot->seats[seat];
        if (!pose->status) {
            driver->car.activeFlag = -1;
            driver->status = SIM_RETIRED;
            continue;
        }
        driver->status = pose->status == MP_FINISHED ? SIM_DRIVER_FINISHED : SIM_DRIVING;
        driver->car.x = pose->x;
        driver->car.y = pose->y;
        driver->car.z = pose->z;
        driver->car.bodyYaw = pose->yaw;
        driver->car.bodyPitch = pose->pitch;
        driver->car.bodyRoll = pose->roll;
        driver->car.modelY = pose->ground;
        driver->car.bodyRollVelocity = pose->rollSpeed;
        driver->car.steeringAngle = pose->steering;
        driver->car.wheelRotation = pose->wheels;
        driver->car.drive.brakeInput = (s16)pose->brake;
        driver->car.trackProgress = pose->progress;
        driver->car.drive.engineRpm = pose->rpm;
        driver->car.drive.acceleratorInput.value = (s16)pose->throttle;
        driver->car.drive.clutch = (s16)pose->clutch;
        driver->car.drive.gear = (s16)pose->gear;
    }
    race->tick = snapshot->tick;
    race->phase = (SimRacePhase)snapshot->phase;
    return 1;
}

static int ReadFull(Socket fd, void *buffer, size_t size) {
    uint8_t *p = buffer;
    size_t got = 0;
    while (got < size) {
        int chunk = size - got > INT_MAX ? INT_MAX : (int)(size - got);
        IoCount n = recv(fd, (char *)p + got, chunk, 0);
        if (n < 0 && Interrupted()) continue;
        if (n <= 0) return 0;
        got += (size_t)n;
    }
    return 1;
}

static int WriteFull(Socket fd, const void *buffer, size_t size) {
    const uint8_t *p = buffer;
    size_t sent = 0;
    while (sent < size) {
        int chunk = size - sent > INT_MAX ? INT_MAX : (int)(size - sent);
#ifdef MSG_NOSIGNAL
        IoCount n = send(fd, (const char *)p + sent, chunk, MSG_NOSIGNAL);
#else
        IoCount n = send(fd, (const char *)p + sent, chunk, 0);
#endif
        if (n < 0 && Interrupted()) continue;
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

static int ConfigureSocket(Socket fd) {
    int one = 1;
#ifdef SO_NOSIGPIPE
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)) != 0) return 0;
#endif
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&one, sizeof(one));
    return 1;
}

MpClient *MpClientConnect(const char *host, uint16_t port) {
    if (!host || !port || !StartSockets()) return NULL;
    Socket fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_FD) {
        StopSockets();
        return NULL;
    }
#ifndef _WIN32
    if (fd >= FD_SETSIZE) {
        CloseSocket(fd);
        return NULL;
    }
#endif
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (!ConfigureSocket(fd) || inet_pton(AF_INET, host, &addr.sin_addr) != 1 ||
        connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        CloseSocket(fd);
        StopSockets();
        return NULL;
    }
    MpClient *client = calloc(1, sizeof(*client));
    if (!client) {
        CloseSocket(fd);
        StopSockets();
        return NULL;
    }
    client->fd = fd;
    return client;
}

void MpClientClose(MpClient *client) {
    if (!client) return;
    CloseSocket(client->fd);
    StopSockets();
    free(client);
}

int MpClientSendHello(MpClient *client, const char *name) {
    if (!client || !name) return 0;
    size_t len = strlen(name);
    if (len > MP_NAME_CAPACITY) len = MP_NAME_CAPACITY;
    uint8_t hello[2 + MP_NAME_CAPACITY];
    hello[0] = MP_C2S_HELLO;
    hello[1] = (uint8_t)len;
    memcpy(hello + 2, name, len);
    return WriteFull(client->fd, hello, 2 + len);
}

int MpClientRecvWelcome(MpClient *client, int *seat) {
    if (!client || !seat) return 0;
    uint8_t body[3];
    if (!ReadFull(client->fd, body, sizeof(body)) || body[0] != MP_S2C_WELCOME ||
        body[1] != MP_PROTOCOL_VERSION || body[2] >= MP_SEAT_LIMIT) return 0;
    *seat = body[2];
    return 1;
}

int MpClientRecvStart(MpClient *client, MpStart *start) {
    if (!client || !start) return 0;
    uint8_t body[1 + MP_START_BODY_SIZE];
    if (!ReadFull(client->fd, body, sizeof(body)) || body[0] != MP_S2C_START) return 0;
    return MpDecodeStart(body + 1, MP_START_BODY_SIZE, start);
}

int MpClientSendLoaded(MpClient *client) {
    const uint8_t message = MP_C2S_LOADED;
    return client && WriteFull(client->fd, &message, 1);
}

int MpClientSendInput(MpClient *client, const DriverInput *input) {
    if (!client || !input) return 0;
    uint8_t wire[MP_INPUT_WIRE_SIZE];
    MpEncodeInput(input, wire);
    return WriteFull(client->fd, wire, sizeof(wire));
}

int MpClientPollInput(MpClient *client, const DriverInput *input) {
    if (!client || !input || client->failed) return 0;
    if (!Nonblocking(client)) goto failed;
    DriverInput latest = *input;
    if (client->queued) {
        latest.shiftUp |= client->latest.shiftUp;
        latest.shiftDown |= client->latest.shiftDown;
    }
    client->latest = latest;
    client->queued = 1;
    /* At most the in-flight packet and one latest packet per call. Never
     * rewrite a packet prefix already on the wire or accumulate frame history. */
    for (int packet = 0; packet < 2; ++packet) {
        if (!client->sending) {
            if (!client->queued) break;
            MpEncodeInput(&client->latest, client->outgoing);
            client->queued = 0;
            client->sending = 1;
            client->sent = 0;
        }
#ifdef MSG_NOSIGNAL
        const int flags = MSG_NOSIGNAL;
#else
        const int flags = 0;
#endif
        IoCount sent = send(client->fd, (const char *)client->outgoing + client->sent,
                            (int)(sizeof(client->outgoing) - client->sent), flags);
        if (sent < 0 && (Interrupted() || WouldBlock())) return 1;
        if (sent <= 0) goto failed;
        client->sent += (size_t)sent;
        if (client->sent < sizeof(client->outgoing)) return 1;
        client->sending = 0;
    }
    return 1;
failed:
    client->failed = 1;
    return 0;
}

/* Reads only available bytes. The header and body can arrive in separate
 * frames; decoded output remains untouched until a whole message is valid. */
static int ReceiveMessage(MpClient *client, MpSnapshot *out, MpResult *result, int block) {
    if (!client || client->failed) return 0;
    if (client->resultPending) {
        if (result) *result = client->result;
        client->resultPending = 0;
        return 2;
    }
    if (!client->needed) client->needed = 1;
read_more:;
    fd_set readers;
    FD_ZERO(&readers);
    FD_SET(client->fd, &readers);
    struct timeval timeout = {0};
#ifdef _WIN32
    int count = select(0, &readers, NULL, NULL, block ? NULL : &timeout);
#else
    int count = select(client->fd + 1, &readers, NULL, NULL, block ? NULL : &timeout);
#endif
    if (count < 0 && Interrupted()) return 3;
    if (count == 0) return 3;
    if (count < 0) goto failed;
    IoCount got = recv(client->fd, (char *)client->incoming + client->received,
                       (int)(client->needed - client->received), 0);
    if (got < 0 && (Interrupted() || WouldBlock())) return 3;
    if (got <= 0) goto failed;
    client->received += (size_t)got;
    if (client->received < client->needed) goto read_more;
    if (client->needed == 1) {
        if (client->incoming[0] == MP_S2C_RESULT) client->needed = 1 + MP_RESULT_BODY_SIZE;
        else if (client->incoming[0] == MP_S2C_SNAPSHOT) client->needed = sizeof(client->incoming);
        else goto failed;
        goto read_more;
    }
    int message;
    if (client->incoming[0] == MP_S2C_RESULT) {
        MpResult discarded;
        if (!MpDecodeResult(client->incoming + 1, MP_RESULT_BODY_SIZE,
                            result ? result : &discarded)) goto failed;
        message = 2;
    } else {
        MpSnapshot discarded;
        if (!MpDecodeSnapshot(client->incoming + 1, sizeof(client->incoming) - 1,
                              out ? out : &discarded)) goto failed;
        message = 1;
    }
    client->received = client->needed = 0;
    return message;
failed:
    client->failed = 1;
    return 0;
}

int MpClientPollMessage(MpClient *client, MpSnapshot *out, MpResult *result) {
    MpSnapshot latest = {0};
    int haveSnapshot = 0;
    /* Bound work per frame even when a peer continuously supplies data. */
    for (int messages = 0; messages < 32; ++messages) {
        MpSnapshot next;
        MpResult finish;
        int message = ReceiveMessage(client, &next, &finish, 0);
        if (!message) return 0;
        if (message == 1) {
            if (haveSnapshot && (next.tick <= latest.tick || next.phase < latest.phase)) {
                client->failed = 1;
                return 0;
            }
            for (int seat = 0; haveSnapshot && seat < MP_SEAT_LIMIT; ++seat) {
                if (latest.seats[seat].status != MP_DRIVING &&
                    next.seats[seat].status != latest.seats[seat].status) {
                    client->failed = 1;
                    return 0;
                }
            }
            latest = next;
            haveSnapshot = 1;
        } else if (message == 2) {
            if (!haveSnapshot) {
                if (result) *result = finish;
                return 2;
            }
            /* Publish the final pose before delivering the result next time. */
            client->result = finish;
            client->resultPending = 1;
            break;
        } else break;
    }
    if (!haveSnapshot) return 3;
    if (out) *out = latest;
    return 1;
}

int MpClientRecvMessage(MpClient *client, MpSnapshot *out, MpResult *result) {
    int message;
    do { message = ReceiveMessage(client, out, result, 1); } while (message == 3);
    return message;
}
