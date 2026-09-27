#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include "mp_client.h"

#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <time.h>
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
    uint64_t room;
    int listing;
    int roomChosen;
    MpLobby lobby;
    Socket fd;
    uint8_t incoming[MP_PUBLICATION_WIRE_SIZE];
    size_t received;
    size_t needed;
    int failed;
    int nonblocking;
    uint32_t sequence;
    int seat, assigned;
    MpCommands commands;
    DriverInput transmitting;
    uint32_t transmittingTick, latestTick;
    uint32_t acknowledged[MP_SEAT_LIMIT];
    struct { uint32_t tick, elapsed; uint8_t phase; int received; } correctionClock;
    uint8_t outgoing[MP_COMMAND_WIRE_SIZE];
    size_t sent;
    int sending;
    DriverInput latest;
    int queued;
    MpResult result;
    int resultPending;
    int raceStarted;
    uint64_t setupDeadline;
    int setupType;
    uint64_t connectDeadline;
    int connecting;
    uint8_t setupWire[2 + MP_NAME_CAPACITY];
    size_t setupSize, setupSent;
    uint64_t writeDeadline;
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

static int Milliseconds(uint64_t *now) {
#ifdef _WIN32
    *now = GetTickCount64();
#else
    struct timespec stamp;
    if (clock_gettime(CLOCK_MONOTONIC, &stamp)) return 0;
    *now = (uint64_t)stamp.tv_sec * 1000 + (uint64_t)stamp.tv_nsec / 1000000;
#endif
    return 1;
}

static int Blocking(MpClient *client) {
#ifdef _WIN32
    u_long enabled = 0;
    if (ioctlsocket(client->fd, FIONBIO, &enabled) != 0) return 0;
#else
    int flags = fcntl(client->fd, F_GETFL);
    if (flags < 0 || fcntl(client->fd, F_SETFL, flags & ~O_NONBLOCK) != 0) return 0;
#endif
    client->nonblocking = 0;
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

static MpClient *Connect(const char *host, uint16_t port, int asynchronous) {
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
    if (!ConfigureSocket(fd) || inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
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
    if (asynchronous && (!Nonblocking(client) || !Milliseconds(&client->connectDeadline))) {
        MpClientClose(client);
        return NULL;
    }
    int result = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (result != 0) {
#ifdef _WIN32
        int pending = WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == WSAEINPROGRESS;
#else
        int pending = errno == EINPROGRESS || errno == EINTR;
#endif
        if (!asynchronous || !pending) {
            MpClientClose(client);
            return NULL;
        }
    }
    if (asynchronous) {
        client->connecting = 1;
        client->connectDeadline += 5000;
    }
    return client;
}

MpClient *MpClientConnect(const char *host, uint16_t port) {
    return Connect(host, port, 0);
}

MpClient *MpClientBeginConnect(const char *host, uint16_t port) {
    return Connect(host, port, 1);
}

int MpClientPollConnect(MpClient *client) {
    if (!client || client->failed) return 0;
    if (!client->connecting) return 1;
    uint64_t now;
    if (!Milliseconds(&now) || now >= client->connectDeadline) goto failed;
    fd_set writes, errors;
    FD_ZERO(&writes);
    FD_ZERO(&errors);
    FD_SET(client->fd, &writes);
    FD_SET(client->fd, &errors);
    struct timeval timeout = {0, 0};
#ifdef _WIN32
    int ready = select(0, NULL, &writes, &errors, &timeout);
#else
    int ready = select(client->fd + 1, NULL, &writes, &errors, &timeout);
#endif
    if (!ready || (ready < 0 && Interrupted())) return 3;
    if (ready < 0) goto failed;
    int error = 0;
#ifdef _WIN32
    int size = sizeof(error);
#else
    socklen_t size = sizeof(error);
#endif
    if (getsockopt(client->fd, SOL_SOCKET, SO_ERROR, (char *)&error, &size) ||
        error || !Blocking(client)) goto failed;
    client->connecting = 0;
    return 1;
failed:
    client->failed = 1;
    return 0;
}

void MpClientClose(MpClient *client) {
    if (!client) return;
    CloseSocket(client->fd);
    StopSockets();
    free(client);
}

int MpClientSendHello(MpClient *client, const char *name) {
    if (!client || !name || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed) return 0;
    size_t len = strlen(name);
    if (len > MP_NAME_CAPACITY) len = MP_NAME_CAPACITY;
    uint8_t hello[2 + MP_NAME_CAPACITY];
    hello[0] = MP_C2S_HELLO;
    hello[1] = (uint8_t)len;
    memcpy(hello + 2, name, len);
    return WriteFull(client->fd, hello, 2 + len);
}

static int ReadLimited(Socket fd, void *buffer, size_t size, uint32_t milliseconds) {
    uint64_t now;
    if (!Milliseconds(&now)) return 0;
    uint64_t deadline = now + milliseconds;
    uint8_t *cursor = buffer;
    while (size) {
        if (!Milliseconds(&now)) return 0;
        if (now >= deadline) return 0;
        uint64_t remaining = deadline - now;
        struct timeval timeout = {(long)(remaining / 1000), (long)(remaining % 1000) * 1000};
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(fd, &reads);
#ifdef _WIN32
        int ready = select(0, &reads, NULL, NULL, &timeout);
#else
        int ready = select(fd + 1, &reads, NULL, NULL, &timeout);
#endif
        if (ready < 0 && Interrupted()) continue;
        if (ready <= 0) return 0;
        IoCount received = recv(fd, (char *)cursor, size > INT_MAX ? INT_MAX : (int)size, 0);
        if (received < 0 && Interrupted()) continue;
        if (received <= 0) return 0;
        cursor += received;
        size -= (size_t)received;
    }
    return 1;
}

int MpClientRecvWelcome(MpClient *client, int *seat) {
    if (!client || !seat || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed) return 0;
    uint8_t body[11];
    if (!ReadLimited(client->fd, body, sizeof(body), 5000) ||
        !MpDecodeWelcome(body, seat, &client->room)) return 0;
    client->seat = *seat;
    client->assigned = 1;
    return 1;
}

const MpCommands *MpClientCommands(const MpClient *client) { return client ? &client->commands : NULL; }

unsigned MpClientPendingCommands(const MpClient *client, MpCommand out[2]) {
    if (!client || !out || client->failed) return 0;
    unsigned count = 0;
    if (client->sending)
        out[count++] = (MpCommand){client->sequence, client->transmittingTick, client->transmitting};
    if (client->queued)
        out[count++] = (MpCommand){0, client->latestTick, client->latest};
    return count;
}

uint64_t MpClientRoom(const MpClient *client) { return client ? client->room : 0; }
const MpLobby *MpClientLobby(const MpClient *client) {
    return client && client->lobby.room.code ? &client->lobby : NULL;
}

static int AcceptLobby(MpClient *client, const uint8_t *wire) {
    MpLobby lobby;
    if (!MpDecodeLobby(wire, 53, &lobby) || lobby.room.code != client->room) return 0;
    client->lobby = lobby;
    return 1;
}

int MpClientRecvStart(MpClient *client, MpStart *start) {
    if (!client || !start || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed) return 0;
    uint8_t body[1 + MP_START_BODY_SIZE];
    uint64_t now;
    if (!Milliseconds(&now)) return 0;
    uint64_t deadline = now + 120000;
    for (;;) {
        if (!Milliseconds(&now) || now >= deadline ||
            !ReadLimited(client->fd, body, 1, (uint32_t)(deadline - now))) return 0;
        size_t size = body[0] == MP_S2C_START ? sizeof(body) : body[0] == MP_S2C_LOBBY ? 53 : 0;
        if (!size || !Milliseconds(&now) || now >= deadline ||
            !ReadLimited(client->fd, body + 1, size - 1, (uint32_t)(deadline - now))) return 0;
        if (body[0] == MP_S2C_START) return MpDecodeStart(body + 1, MP_START_BODY_SIZE, start);
        if (!AcceptLobby(client, body)) return 0;
    }
}

static int PollSetup(MpClient *client, int type, size_t size, uint32_t budget) {
    if (!client || client->failed || client->connecting || client->setupSize || client->needed ||
        (client->listing && type != MP_S2C_LIST)) return 0;
    uint64_t now;
    if (!Milliseconds(&now)) { client->failed = 1; return 0; }
    if (!client->setupType) {
        client->setupType = type;
        /* A room can wait for Ready indefinitely. Once Start bytes arrive,
         * receiving that bounded packet still has an absolute deadline. */
        client->setupDeadline = type == MP_S2C_START ? 0 : now + budget;
    }
    if (client->setupType != type || (client->setupDeadline && now >= client->setupDeadline)) {
        client->failed = 1;
        return 0;
    }
    while (client->received < size) {
        fd_set reads;
        FD_ZERO(&reads);
        FD_SET(client->fd, &reads);
        struct timeval timeout = {0, 0};
#ifdef _WIN32
        int ready = select(0, &reads, NULL, NULL, &timeout);
#else
        int ready = select(client->fd + 1, &reads, NULL, NULL, &timeout);
#endif
        if (ready == 0 || (ready < 0 && Interrupted())) return 3;
        if (ready < 0) break;
        IoCount n = recv(client->fd, (char *)client->incoming + client->received,
                         (int)(size - client->received), 0);
        if (n < 0 && (Interrupted() || WouldBlock())) return 3;
        if (n <= 0) break;
        if (!client->setupDeadline) client->setupDeadline = now + budget;
        client->received += (size_t)n;
    }
    if (client->received == size && (client->incoming[0] == type ||
        (type == MP_S2C_START && client->incoming[0] == MP_S2C_LOBBY))) return 1;
    client->failed = 1;
    return 0;
}

int MpClientRecvConfig(MpClient *client, MpCarConfig *config) {
    if (!client || !config || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed) return 0;
    uint8_t packet[MP_CONFIG_WIRE_SIZE];
    if (!ReadLimited(client->fd, packet, sizeof(packet), 5000) ||
        !MpDecodeConfig(packet, sizeof(packet), config)) {
        client->failed = 1;
        return 0;
    }
    return 1;
}

int MpClientRecvAvailability(MpClient *client, uint32_t *mask) {
    if (!client || !mask || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed) return 0;
    uint8_t packet[MP_AVAILABILITY_WIRE_SIZE];
    if (!ReadLimited(client->fd, packet, sizeof(packet), 5000) ||
        !MpDecodeAvailability(packet, sizeof(packet), mask)) {
        client->failed = 1; return 0;
    }
    return 1;
}
int MpClientPollAvailability(MpClient *client, uint32_t *mask) {
    if (!mask) return 0;
    int state = PollSetup(client, 0x89, MP_AVAILABILITY_WIRE_SIZE, 5000);
    if (state != 1) return state;
    if (!MpDecodeAvailability(client->incoming, MP_AVAILABILITY_WIRE_SIZE, mask)) {
        client->failed = 1; return 0;
    }
    client->received = client->setupType = 0;
    client->setupDeadline = 0;
    return 1;
}

int MpClientPollConfig(MpClient *client, MpCarConfig *config) {
    if (!config) return 0;
    int state = PollSetup(client, 0x88, MP_CONFIG_WIRE_SIZE, 5000);
    if (state != 1) return state;
    if (!MpDecodeConfig(client->incoming, MP_CONFIG_WIRE_SIZE, config)) {
        client->failed = 1;
        return 0;
    }
    client->received = client->setupType = 0;
    client->setupDeadline = 0;
    return 1;
}

int MpClientPollWelcome(MpClient *client, int *seat) {
    if (!seat) return 0;
    int state = PollSetup(client, MP_S2C_WELCOME, 11, 5000);
    if (state != 1) return state;
    if (!MpDecodeWelcome(client->incoming, seat, &client->room)) {
        client->failed = 1;
        return 0;
    }
    client->seat = *seat;
    client->assigned = 1;
    client->received = 0;
    client->setupType = 0;
    return 1;
}

int MpClientPollStart(MpClient *client, MpStart *start) {
    if (!start) return 0;
    int state;
    if (!client || client->received == 0) {
        state = PollSetup(client, MP_S2C_START, 1, 5000);
        if (state != 1) return state;
    }
    size_t size = client->incoming[0] == MP_S2C_LOBBY ? 53 : 1 + MP_START_BODY_SIZE;
    state = PollSetup(client, MP_S2C_START, size, 5000);
    if (state != 1) return state;
    if (client->incoming[0] == MP_S2C_LOBBY) {
        if (!AcceptLobby(client, client->incoming)) { client->failed = 1; return 0; }
        client->received = client->setupType = 0;
        client->setupDeadline = 0;
        return 2;
    }
    if (!MpDecodeStart(client->incoming + 1, MP_START_BODY_SIZE, start)) {
        client->failed = 1;
        return 0;
    }
    client->received = 0;
    client->setupType = 0;
    return 1;
}

int MpClientSendLoaded(MpClient *client) {
    const uint8_t message = MP_C2S_LOADED;
    return client && !client->failed && !client->nonblocking && !client->connecting &&
           !client->setupType && !client->setupSize && !client->needed &&
           WriteFull(client->fd, &message, 1);
}

int MpClientSendReady(MpClient *client, int ready) {
    if (!client || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed || (unsigned)ready > 1) return 0;
    const uint8_t wire[] = {MP_C2S_READY, (uint8_t)ready};
    return WriteFull(client->fd, wire, sizeof(wire));
}

int MpClientSendPick(MpClient *client, int variant, int manual) {
    if (!client || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed ||
        (unsigned)variant >= CAR_MODEL_VARIANT_COUNT || (unsigned)manual > 1) return 0;
    const uint8_t wire[] = {MP_C2S_PICK, (uint8_t)variant, (uint8_t)manual};
    return WriteFull(client->fd, wire, sizeof(wire));
}

static int PollSetupWrite(MpClient *client, const uint8_t *wire, size_t size) {
    if (!client || client->failed || client->connecting || client->needed ||
        (client->listing && wire[0] != MP_C2S_LIST) ||
        (client->setupType && !(client->setupType == MP_S2C_START &&
            (wire[0] == MP_C2S_READY || wire[0] == MP_C2S_PICK)))) return 0;
    uint64_t now;
    if (!Milliseconds(&now) || !Nonblocking(client)) goto failed;
    if (!client->setupSize) {
        memcpy(client->setupWire, wire, size);
        client->setupSize = size;
        client->setupSent = 0;
        client->writeDeadline = now + 5000;
    }
    if (client->setupWire[0] != wire[0] || now >= client->writeDeadline) goto failed;
#ifdef MSG_NOSIGNAL
    const int flags = MSG_NOSIGNAL;
#else
    const int flags = 0;
#endif
    IoCount n = send(client->fd, (const char *)client->setupWire + client->setupSent,
                     (int)(client->setupSize - client->setupSent), flags);
    if (n < 0 && (Interrupted() || WouldBlock())) return 3;
    if (n <= 0) goto failed;
    client->setupSent += (size_t)n;
    if (client->setupSent < client->setupSize) return 3;
    client->setupSize = 0;
    return 1;
failed:
    client->failed = 1;
    return 0;
}

int MpClientPollList(MpClient *client, MpRoomInfo rooms[MP_ROOM_LIMIT], size_t *count) {
    if (!client || !rooms || !count || client->failed || client->roomChosen || client->connecting || client->needed) return 0;
    if (!client->listing) {
        if (client->setupType || client->setupSize) return 0;
        client->listing = 1;
    }
    if (client->listing == 1) {
        const uint8_t request = MP_C2S_LIST;
        int sent = PollSetupWrite(client, &request, 1);
        if (sent != 1) return sent;
        client->listing = 2;
    }
    if (client->received < 3) {
        int received = PollSetup(client, MP_S2C_LIST, 3, 5000);
        if (received != 1) return received;
    }
    if (client->incoming[1] != MP_PROTOCOL_VERSION || client->incoming[2] > MP_ROOM_LIMIT) goto failed;
    size_t size = 3 + (size_t)client->incoming[2] * 15;
    int received = PollSetup(client, MP_S2C_LIST, size, 5000);
    if (received != 1) return received;
    if (!MpDecodeRoomList(client->incoming, size, rooms, count)) goto failed;
    client->listing = client->setupType = 0;
    client->received = 0;
    return 1;
failed:
    client->failed = 1;
    return 0;
}

int MpClientPollHello(MpClient *client, const char *name) {
    if (!name) return 0;
    size_t len = strlen(name);
    if (len > MP_NAME_CAPACITY) len = MP_NAME_CAPACITY;
    uint8_t wire[2 + MP_NAME_CAPACITY] = {MP_C2S_HELLO, (uint8_t)len};
    memcpy(wire + 2, name, len);
    return PollSetupWrite(client, wire, 2 + len);
}

int MpClientPollLoaded(MpClient *client) {
    const uint8_t wire = MP_C2S_LOADED;
    return PollSetupWrite(client, &wire, 1);
}

int MpClientPollReady(MpClient *client, int ready) {
    if ((unsigned)ready > 1) return 0;
    const uint8_t wire[] = {MP_C2S_READY, (uint8_t)ready};
    return PollSetupWrite(client, wire, sizeof(wire));
}

int MpClientPollPick(MpClient *client, int variant, int manual) {
    if ((unsigned)variant >= CAR_MODEL_VARIANT_COUNT || (unsigned)manual > 1) return 0;
    const uint8_t wire[] = {MP_C2S_PICK, (uint8_t)variant, (uint8_t)manual};
    return PollSetupWrite(client, wire, sizeof(wire));
}

int MpClientPollRace(MpClient *client, const MpRaceOptions *options) {
    if (!MpValidRaceOptions(options)) return 0;
    const uint8_t wire[] = {MP_C2S_RACE, options->classIndex, options->course,
                           options->laps, options->reverse};
    return PollSetupWrite(client, wire, sizeof(wire));
}

int MpClientPollRoom(MpClient *client, uint64_t code) {
    if (!client || client->roomChosen || (code > INT64_MAX && code != UINT64_MAX)) return 0;
    uint8_t wire[9] = {MP_C2S_ROOM};
    for (int i = 0; i < 8; ++i) wire[i + 1] = (uint8_t)(code >> (i * 8));
    int sent = PollSetupWrite(client, wire, sizeof(wire));
    if (sent == 1) client->roomChosen = 1;
    return sent;
}

int MpClientSendRoom(MpClient *client, uint64_t code) {
    if (!client || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize || client->needed ||
        client->roomChosen || (code > INT64_MAX && code != UINT64_MAX)) return 0;
    uint8_t wire[9] = {MP_C2S_ROOM};
    for (int i = 0; i < 8; ++i) wire[i + 1] = (uint8_t)(code >> (i * 8));
    int sent = WriteFull(client->fd, wire, sizeof(wire));
    if (sent) client->roomChosen = 1;
    return sent;
}

int MpClientSendInput(MpClient *client, const DriverInput *input) {
    if (!client || !ValidDriverInput(input) || client->failed || client->nonblocking || client->connecting ||
        client->setupType || client->setupSize) return 0;
    if (client->sequence == UINT32_MAX || client->commands.count == MP_COMMAND_CAPACITY) return 0;
    uint8_t wire[MP_COMMAND_WIRE_SIZE];
    MpEncodeCommand(input, client->sequence + 1, wire);
    if (!WriteFull(client->fd, wire, sizeof(wire))) {
        client->failed = 1;
        return 0;
    }
    if (!MpRememberCommand(&client->commands, ++client->sequence, 0, input)) {
        client->failed = 1;
        return 0;
    }
    return 1;
}

int MpClientPollInput(MpClient *client, const DriverInput *input, uint32_t tick) {
    if (!client || !ValidDriverInput(input) || client->failed ||
        tick < client->latestTick || tick < client->commands.lastTick) return 0;
    if (client->connecting || client->setupType || client->setupSize) goto failed;
    if (!Nonblocking(client)) goto failed;
    DriverInput latest = *input;
    if (client->queued) {
        latest.shiftUp |= client->latest.shiftUp;
        latest.shiftDown |= client->latest.shiftDown;
    }
    client->latest = latest;
    client->latestTick = tick;
    client->queued = 1;
    /* At most the in-flight packet and one latest packet per call. Never
     * rewrite a packet prefix already on the wire or accumulate frame history. */
    for (int packet = 0; packet < 2; ++packet) {
        if (!client->sending) {
            if (!client->queued) break;
            if (client->sequence == UINT32_MAX) goto failed;
            if (client->commands.count == MP_COMMAND_CAPACITY) return 1;
            client->transmitting = client->latest;
            client->transmittingTick = client->latestTick;
            MpEncodeCommand(&client->latest, ++client->sequence, client->outgoing);
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
        if (!MpRememberCommand(&client->commands, client->sequence, client->transmittingTick, &client->transmitting)) goto failed;
        client->sending = 0;
    }
    return 1;
failed:
    client->failed = 1;
    return 0;
}

/* Reads only available bytes. The header and body can arrive in separate
 * frames; decoded output remains untouched until a whole message is valid. */
static int ReceiveRaceState(MpClient *client, RaceSim *race, MpCorrection *correction,
                            MpSnapshot *out, MpResult *result, int block, uint64_t deadline) {
    if (!client || client->failed) return 0;
    if (client->connecting || client->setupType || client->setupSize) goto failed;
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
    if (block) {
        uint64_t now;
        if (!Milliseconds(&now) || now >= deadline) goto failed;
        uint64_t remaining = deadline - now;
        timeout.tv_sec = (long)(remaining / 1000);
        timeout.tv_usec = (long)(remaining % 1000) * 1000;
    }
#ifdef _WIN32
    int count = select(0, &readers, NULL, NULL, &timeout);
#else
    int count = select(client->fd + 1, &readers, NULL, NULL, &timeout);
#endif
    if (count < 0 && Interrupted()) return 3;
    if (count == 0) { if (block) goto failed; return 3; }
    if (count < 0) goto failed;
    IoCount got = recv(client->fd, (char *)client->incoming + client->received,
                       (int)(client->needed - client->received), 0);
    if (got < 0 && (Interrupted() || WouldBlock())) return 3;
    if (got <= 0) goto failed;
    client->received += (size_t)got;
    if (client->received < client->needed) goto read_more;
    if (client->needed == 1) {
        if (client->incoming[0] == MP_S2C_RESULT) client->needed = 1 + MP_RESULT_BODY_SIZE;
        else if (client->incoming[0] == MP_S2C_SNAPSHOT) client->needed = 1 + MP_SNAPSHOT_BODY_SIZE;
        else if (client->incoming[0] == MP_S2C_CORRECTION)
            client->needed = MP_PUBLICATION_WIRE_SIZE;
        else goto failed;
        goto read_more;
    }
    int message;
    if (client->incoming[0] == MP_S2C_RESULT) {
        MpResult discarded;
        if (!MpDecodeResult(client->incoming + 1, MP_RESULT_BODY_SIZE,
                            result ? result : &discarded)) goto failed;
        message = 2;
    } else if (client->incoming[0] == MP_S2C_CORRECTION) {
        MpCorrection decoded;
        MpSnapshot poses;
        if (race) {
            if (!client->assigned || !MpDecodePublication(race, client->incoming, client->needed, &decoded, &poses)) goto failed;
        } else {
            if (!MpDecodePublicationSnapshot(client->incoming, client->needed, &poses)) goto failed;
            memset(&decoded, 0, sizeof(decoded));
            decoded.frame.tick = poses.tick;
            decoded.frame.elapsed = poses.elapsed;
            decoded.frame.phase = (SimRacePhase)poses.phase;
            memcpy(decoded.acknowledged, poses.acknowledged, sizeof(decoded.acknowledged));
        }
        if (client->correctionClock.received &&
            (decoded.frame.tick <= client->correctionClock.tick ||
             decoded.frame.elapsed < client->correctionClock.elapsed ||
             decoded.frame.phase < client->correctionClock.phase)) goto failed;
        for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
            if (decoded.acknowledged[seat] < client->acknowledged[seat]) goto failed;
        if (race) {
            if (!MpApplyCorrection(race, &client->commands, client->seat, &decoded)) goto failed;
        } else if (client->assigned && !MpAcknowledgeCommands(&client->commands,
                    decoded.acknowledged[client->seat])) goto failed;
        memcpy(client->acknowledged, decoded.acknowledged, sizeof(client->acknowledged));
        client->correctionClock.tick = decoded.frame.tick;
        client->correctionClock.elapsed = decoded.frame.elapsed;
        client->correctionClock.phase = (uint8_t)decoded.frame.phase;
        client->correctionClock.received = 1;
        if (correction && race) *correction = decoded;
        if (out) *out = poses;
        client->raceStarted = 1;
        message = race ? 4 : 1;
    } else {
        MpSnapshot decoded;
        if (!MpDecodeSnapshot(client->incoming + 1, MP_SNAPSHOT_BODY_SIZE, &decoded)) goto failed;
        for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat)
            if (decoded.acknowledged[seat] < client->acknowledged[seat]) goto failed;
        if (client->assigned && !MpAcknowledgeCommands(&client->commands,
            decoded.acknowledged[client->seat])) goto failed;
        memcpy(client->acknowledged, decoded.acknowledged, sizeof(client->acknowledged));
        if (out) *out = decoded;
        client->raceStarted = 1;
        message = 1;
    }
    client->received = client->needed = 0;
    return message;
failed:
    client->failed = 1;
    return 0;
}

static int ReceiveMessage(MpClient *client, MpSnapshot *out, MpResult *result, int block, uint64_t deadline) {
    return ReceiveRaceState(client, NULL, NULL, out, result, block, deadline);
}

static int PollMessages(MpClient *client, RaceSim *race, MpSnapshot *out,
                         MpResult *result, MpCorrection *correction) {
    MpSnapshot latest = {0};
    int haveSnapshot = 0;
    int applied = 0;
    MpCorrection latestCorrection;
    /* Bound work per frame even when a peer continuously supplies data. */
    for (int messages = 0; messages < 32; ++messages) {
        MpSnapshot next;
        MpResult finish;
        MpCorrection nextCorrection;
        int message = ReceiveRaceState(client, race, &nextCorrection, &next, &finish, 0, 0);
        if (!message) return 0;
        if (message == 1 || message == 4) {
            if (haveSnapshot && (next.tick <= latest.tick || next.phase < latest.phase ||
                                 next.elapsed < latest.elapsed)) {
                client->failed = 1;
                return 0;
            }
            for (int seat = 0; haveSnapshot && seat < MP_FIELD_LIMIT; ++seat) {
                if (latest.seats[seat].status != MP_DRIVING &&
                    next.seats[seat].status != latest.seats[seat].status) {
                    client->failed = 1;
                    return 0;
                }
            }
            latest = next;
            haveSnapshot = 1;
            applied = message == 4;
            if (applied) latestCorrection = nextCorrection;
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
    if (applied && correction) *correction = latestCorrection;
    return applied ? 4 : 1;
}

int MpClientPollRaceState(MpClient *client, RaceSim *race, MpSnapshot *out,
                          MpResult *result, MpCorrection *correction) {
    return PollMessages(client, race, out, result, correction);
}

int MpClientPollMessage(MpClient *client, MpSnapshot *out, MpResult *result) {
    return PollMessages(client, NULL, out, result, NULL);
}

int MpClientRecvMessage(MpClient *client, MpSnapshot *out, MpResult *result) {
    uint64_t now;
    if (!client || !Milliseconds(&now)) return 0;
    uint64_t deadline = now + (client->raceStarted ? 5000 : 65000);
    int message;
    do { message = ReceiveMessage(client, out, result, 1, deadline); } while (message == 3);
    return message;
}
