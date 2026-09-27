/* A local socket pair exercises the real private transfer loops and setup,
 * without a listening port, game assets or a test-only production API. */
#include "../../src/port/mp_client.c"
#include "game/race_sim.h"
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>

static volatile sig_atomic_t interrupted;
static void InterruptRead(int number) { (void)number; interrupted = 1; }

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

static int TestCorrectionTransfer(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0], .assigned = 1, .seat = 0};
    const DriverInput controls = {.throttle = 128};
    CHECK(MpClientSendInput(&client, &controls));
    CHECK(MpClientSendInput(&client, &controls));
    const GameTrackPoint points[3] = {0};
    RaceSim race = {.route = {.points = points, .count = 3, .length = 3000},
        .laps = 1, .phase = SIM_RACING, .tick = 100, .elapsed = 80};
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) race.drivers[seat].variant = -1;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        race.drivers[seat].status = SIM_DRIVING;
        race.drivers[seat].inputTick = 98;
        race.drivers[seat].car.drive.gear = 1;
    }
    uint32_t acknowledged[2] = {1, 42};
    uint8_t packet[MP_PUBLICATION_WIRE_SIZE] = {0};
    CHECK(MpEncodeCorrection(&race, acknowledged, packet, MP_CORRECTION_WIRE_SIZE));
    packet[MP_CORRECTION_WIRE_SIZE] = MP_S2C_SNAPSHOT;
    uint8_t *body = packet + MP_CORRECTION_WIRE_SIZE + 1;
    body[0] = 100; body[4] = 80; body[8] = SIM_RACING;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE] = MP_DRIVING;
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE + 53] = 1;
    }
    memcpy(body + MP_SNAPSHOT_BODY_SIZE - 8, packet + 2, 8);
    race.drivers[0].car.x = 1234;
    const RaceSim original = race;
    const MpCommands commands = client.commands;
    MpCorrection correction;
    memset(&correction, 0xA5, sizeof(correction));
    const MpCorrection untouched = correction;
    for (size_t byte = 0; byte < sizeof(packet); ++byte) {
        CHECK(WriteFull(pair[1], packet + byte, 1));
        CHECK(MpClientPollRaceState(&client, &race, NULL, NULL, &correction) ==
              (byte + 1 == sizeof(packet) ? 4 : 3));
        if (byte + 1 < sizeof(packet)) {
            CHECK(memcmp(&race, &original, sizeof(race)) == 0);
            CHECK(memcmp(&client.commands, &commands, sizeof(commands)) == 0);
            CHECK(memcmp(&correction, &untouched, sizeof(correction)) == 0);
        }
    }
    CHECK(race.drivers[0].car.x == 0);
    CHECK(correction.acknowledged[0] == 1 && correction.acknowledged[1] == 42);
    CHECK(client.commands.count == 1 && MpCommandAt(&client.commands, 0)->sequence == 2);
    const RaceSim accepted = race;
    const MpCommands pending = client.commands;
    const MpCorrection acceptedCorrection = correction;
    const MpClient acceptedClient = client;
    for (int bad = 0; bad < 3; ++bad) {
        uint8_t invalid[sizeof(packet)];
        memcpy(invalid, packet, sizeof(invalid));
        if (bad != 0) { invalid[10 + 13] = 101; invalid[MP_CORRECTION_WIRE_SIZE + 1] = 101; }
        if (bad == 1) invalid[2] = 3; /* Local acknowledgement exceeds sent commands. */
        if (bad == 2) invalid[6] = 41; /* Remote acknowledgement regresses. */
        memcpy(invalid + sizeof(invalid) - 8, invalid + 2, 8);
        CHECK(WriteFull(pair[1], invalid, sizeof(invalid)));
        CHECK(!MpClientPollRaceState(&client, &race, NULL, NULL, &correction));
        CHECK(memcmp(&race, &accepted, sizeof(race)) == 0);
        CHECK(memcmp(&client.commands, &pending, sizeof(pending)) == 0);
        CHECK(memcmp(&correction, &acceptedCorrection, sizeof(correction)) == 0);
        client = acceptedClient; /* Independent failed-packet fixture on the drained stream. */
    }
    packet[10 + 13] = 101; /* Next authoritative tick, so test field validation. */
    body[0] = 101;
    packet[10 + 29 + 116 + 306] = 7; /* Valid envelope, invalid drivetrain. */
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    CHECK(!MpClientPollRaceState(&client, &race, NULL, NULL, &correction));
    CHECK(memcmp(&race, &accepted, sizeof(race)) == 0);
    CHECK(memcmp(&client.commands, &pending, sizeof(pending)) == 0);
    CHECK(memcmp(&correction, &acceptedCorrection, sizeof(correction)) == 0);
    CHECK(client.failed);
    client = acceptedClient;
    race = accepted;
    race.tick = 101; race.phase = SIM_FINISHED;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        race.drivers[seat].status = SIM_RETIRED;
        race.drivers[seat].car.activeFlag = -1;
        body[MP_SNAPSHOT_HEADER_SIZE + seat * MP_SNAPSHOT_SEAT_SIZE] = MP_RETIRED;
    }
    body[0] = 101; body[8] = SIM_FINISHED;
    CHECK(MpEncodeCorrection(&race, acknowledged, packet, MP_CORRECTION_WIRE_SIZE));
    race = accepted;
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    const uint8_t finish[] = {MP_S2C_RESULT, 0, 0, 255, 255, 255, 255,
                                           0, 0, 255, 255, 255, 255};
    CHECK(WriteFull(pair[1], finish, sizeof(finish)));
    close(pair[1]);
    MpSnapshot snapshot;
    MpResult result;
    CHECK(MpClientPollRaceState(&client, &race, &snapshot, &result, NULL) == 4);
    CHECK(snapshot.tick == 101 && snapshot.phase == SIM_FINISHED && race.phase == SIM_FINISHED);
    CHECK(MpClientPollRaceState(&client, &race, &snapshot, &result, NULL) == 2);
    close(pair[0]);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    client = (MpClient){.fd = pair[0]};
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    CHECK(WriteFull(pair[1], finish, sizeof(finish)));
    close(pair[1]);
    CHECK(MpClientPollMessage(&client, &snapshot, &result) == 1);
    CHECK(snapshot.tick == 101 && snapshot.phase == SIM_FINISHED);
    CHECK(MpClientPollMessage(&client, &snapshot, &result) == 2);
    close(pair[0]);
    return 0;
}

static int TestRaceReadDeadline(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    uint8_t header = MP_S2C_SNAPSHOT;
    CHECK(WriteFull(pair[1], &header, 1));
    MpSnapshot snapshot;
    memset(&snapshot, 0xA5, sizeof(snapshot));
    MpSnapshot unchanged = snapshot;
    uint64_t now;
    CHECK(Milliseconds(&now));
    /* Consume a partial packet, then expire the same absolute deadline. */
    CHECK(ReceiveMessage(&client, &snapshot, NULL, 0, 0) == 3);
    CHECK(client.received == 1 && !client.raceStarted);
    CHECK(!ReceiveMessage(&client, &snapshot, NULL, 1, now));
    CHECK(memcmp(&snapshot, &unchanged, sizeof(snapshot)) == 0);
    CHECK(client.failed);
    CHECK(!MpClientRecvMessage(&client, &snapshot, NULL));
    client = (MpClient){.fd = pair[0]};
    CHECK(Milliseconds(&now));
    CHECK(!ReceiveMessage(&client, &snapshot, NULL, 1, now + 2));
    CHECK(client.failed && memcmp(&snapshot, &unchanged, sizeof(snapshot)) == 0);
    close(pair[0]);
    close(pair[1]);
    return 0;
}

static int TestResultAfterPeerCloses(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    CHECK(ConfigureSocket(pair[0]));
    MpClient client = {.fd = pair[0]};
    CHECK(Nonblocking(&client));
    uint8_t snapshot[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT};
    snapshot[1] = 42;
    snapshot[9] = SIM_FINISHED;
    const uint8_t result[] = {MP_S2C_RESULT, 0, 0, 255, 255, 255, 255,
                                          0, 0, 255, 255, 255, 255};
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    CHECK(WriteFull(pair[1], result, sizeof(result)));
    close(pair[1]);
    MpSnapshot final;
    MpResult finish;
    CHECK(MpClientPollMessage(&client, &final, &finish) == 1);
    CHECK(final.tick == 42 && final.phase == SIM_FINISHED);
    CHECK(MpClientPollMessage(&client, &final, &finish) == 2);
    CHECK(!finish.seats[0].finished && finish.seats[0].milliseconds == -1);
    CHECK(!finish.seats[1].finished && finish.seats[1].milliseconds == -1);
    close(pair[0]);
    return 0;
}

static int TestCommandHistoryBackpressure(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    const uint8_t welcome[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 1, 42, 0, 0, 0, 0, 0, 0, 0};
    CHECK(WriteFull(pair[1], welcome, sizeof(welcome)));
    int seat = -1;
    CHECK(MpClientPollWelcome(&client, &seat) == 1 && seat == 1);
    DriverInput input = {0};
    uint8_t wire[MP_COMMAND_WIRE_SIZE];
    input.shiftUp = 2;
    CHECK(!MpClientPollInput(&client, &input, 0));
    CHECK(!client.failed && !client.queued && !client.sequence && !client.commands.count);
    wire[0] = 0xA5;
    CHECK(!ReadLimited(pair[1], wire, 1, 2) && wire[0] == 0xA5);
    input.shiftUp = 0;
    for (unsigned sequence = 1; sequence <= MP_COMMAND_CAPACITY; ++sequence) {
        input.throttle = sequence % 257;
        CHECK(MpClientPollInput(&client, &input, 0));
        CHECK(ReadLimited(pair[1], wire, sizeof(wire), 5000));
        CHECK(client.commands.sent == sequence && client.commands.count == sequence);
    }
    for (unsigned frame = 0; frame < 10000; ++frame) {
        input = (DriverInput){.throttle = frame % 257, .shiftUp = frame == 10, .shiftDown = frame == 20};
        CHECK(MpClientPollInput(&client, &input, 0));
        CHECK(client.sequence == MP_COMMAND_CAPACITY && client.commands.count == MP_COMMAND_CAPACITY);
        CHECK(!client.sending && client.queued);
    }
    uint8_t snapshot[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT, 1};
    const size_t ack = sizeof(snapshot) - MP_SEAT_LIMIT * 4;
    snapshot[ack + 4] = MP_COMMAND_CAPACITY / 2;
    MpSnapshot decoded;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1);
    CHECK(client.commands.count == MP_COMMAND_CAPACITY / 2);
    input = (DriverInput){.throttle = 222};
    CHECK(MpClientPollInput(&client, &input, 0));
    CHECK(ReadLimited(pair[1], wire, sizeof(wire), 5000));
    CHECK(client.commands.count == MP_COMMAND_CAPACITY / 2 + 1);
    CHECK(MpCommandAt(MpClientCommands(&client), 0)->sequence == MP_COMMAND_CAPACITY / 2 + 1);
    const MpCommand *last = MpCommandAt(&client.commands, client.commands.count - 1);
    CHECK(last->sequence == MP_COMMAND_CAPACITY + 1 && last->input.throttle == 222);
    CHECK(last->input.shiftUp && last->input.shiftDown);
    input.shiftUp = input.shiftDown = 1;
    uint8_t expected[MP_COMMAND_WIRE_SIZE];
    MpEncodeCommand(&input, MP_COMMAND_CAPACITY + 1, expected);
    CHECK(memcmp(wire, expected, sizeof(wire)) == 0);
    // Reject an acknowledgement of a command that has never completed transmission.
    const MpCommands saved = client.commands;
    const MpSnapshot unchanged = decoded;
    snapshot[1] = 2; snapshot[ack + 4] = 2; snapshot[ack + 5] = 1;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    CHECK(!MpClientPollMessage(&client, &decoded, NULL));
    CHECK(memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
    CHECK(memcmp(&client.commands, &saved, sizeof(saved)) == 0);
    close(pair[0]); close(pair[1]);
    return 0;
}

static int TestAcknowledgements(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    CHECK(Nonblocking(&client));
    uint8_t packet[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT, 1};
    const size_t ack = sizeof(packet) - MP_SEAT_LIMIT * 4;
    packet[ack] = 7; packet[ack + 4] = 9;
    MpSnapshot decoded = {0};
    for (size_t byte = 0; byte < sizeof(packet); ++byte) {
        CHECK(WriteFull(pair[1], packet + byte, 1));
        CHECK(MpClientPollMessage(&client, &decoded, NULL) == (byte + 1 == sizeof(packet) ? 1 : 3));
        if (byte + 1 < sizeof(packet)) CHECK(decoded.tick == 0);
    }
    CHECK(decoded.acknowledged[0] == 7 && decoded.acknowledged[1] == 9);
    MpSnapshot unchanged = decoded;
    packet[1] = 2; packet[ack + 4] = 8;
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    CHECK(!MpClientPollMessage(&client, &decoded, NULL));
    CHECK(client.failed && memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
    CHECK(client.acknowledged[0] == 7 && client.acknowledged[1] == 9);
    close(pair[0]); close(pair[1]);
    return 0;
}

static int TestSlowPeer(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    CHECK(ConfigureSocket(pair[0]));
    MpClient client = {.fd = pair[0]};
    CHECK(Nonblocking(&client));
    char padding[1024] = {0};
    size_t filled = 0;
    for (;;) {
        IoCount n = send(pair[0], padding, sizeof(padding), 0);
        if (n < 0 && Interrupted()) continue;
        if (n < 0 && WouldBlock()) break;
        CHECK(n > 0);
        filled += (size_t)n;
        CHECK(filled < 16 * 1024 * 1024);
    }
    DriverInput first = {.throttle = 256, .shiftUp = 1};
    CHECK(MpClientPollInput(&client, &first, 10));
    CHECK(client.sending && client.sent == 0);
    MpCommand pending[2];
    CHECK(MpClientPendingCommands(&client, pending) == 1);
    CHECK(pending[0].sequence == 1 && pending[0].tick == 10 && pending[0].input.shiftUp);
    DriverInput latest = {0};
    for (int frame = 0; frame < 10000; ++frame) {
        latest = (DriverInput){.throttle = frame % 257,
                              .shiftUp = frame == 17, .shiftDown = frame == 19};
        CHECK(MpClientPollInput(&client, &latest, 11 + (uint32_t)frame));
        CHECK(client.sending && client.sent == 0 && client.queued && client.sequence == 1);
        CHECK(client.commands.count == 0);
    }
    CHECK(MpClientPendingCommands(&client, pending) == 2);
    CHECK(pending[0].sequence == 1 && pending[0].tick == 10);
    CHECK(pending[1].sequence == 0 && pending[1].tick == 10010);
    CHECK(pending[1].input.shiftUp && pending[1].input.shiftDown);
    const MpClient pendingOwner = client;
    CHECK(MpClientPendingCommands(&client, pending) == 2);
    CHECK(memcmp(&client, &pendingOwner, sizeof(client)) == 0);
    CHECK(!MpClientPendingCommands(NULL, pending));
    CHECK(!MpClientPendingCommands(&client, NULL));
    while (filled) {
        size_t chunk = filled < sizeof(padding) ? filled : sizeof(padding);
        CHECK(ReadLimited(pair[1], padding, chunk, 5000));
        filled -= chunk;
    }
    CHECK(MpClientPollInput(&client, &latest, 10011));
    uint8_t actual[2 * MP_COMMAND_WIRE_SIZE], expected[2 * MP_COMMAND_WIRE_SIZE];
    MpEncodeCommand(&first, 1, expected);
    latest.shiftUp = latest.shiftDown = 1;
    MpEncodeCommand(&latest, 2, expected + MP_COMMAND_WIRE_SIZE);
    CHECK(ReadLimited(pair[1], actual, sizeof(actual), 5000));
    CHECK(memcmp(actual, expected, sizeof(actual)) == 0);
    CHECK(!MpClientPollRoom(&client, (uint64_t)INT64_MAX + 1));
    CHECK(MpClientPollRoom(&client, UINT64_C(0x0102030405060708)) == 1);
    const uint8_t expectedRoom[] = {MP_C2S_ROOM, 8, 7, 6, 5, 4, 3, 2, 1};
    uint8_t room[sizeof(expectedRoom)];
    CHECK(ReadLimited(pair[1], room, sizeof(room), 5000));
    CHECK(memcmp(room, expectedRoom, sizeof(room)) == 0);
    CHECK(!client.sending && !client.queued);
    CHECK(client.commands.count == 2);
    memset(pending, 0xA5, sizeof(pending));
    const MpCommand emptyPending[2] = {pending[0], pending[1]};
    CHECK(MpClientPendingCommands(&client, pending) == 0);
    CHECK(memcmp(pending, emptyPending, sizeof(pending)) == 0);
    CHECK(MpCommandAt(&client.commands, 0)->tick == 10);
    CHECK(MpCommandAt(&client.commands, 1)->tick == 10011);
    CHECK(MpCommandAt(&client.commands, 0)->input.shiftUp);
    CHECK(MpCommandAt(&client.commands, 1)->input.shiftUp && MpCommandAt(&client.commands, 1)->input.shiftDown);

    /* A previously written prefix must stay intact when newer controls arrive. */
    client.sequence = 3;
    client.transmitting = first;
    client.transmittingTick = 10012;
    MpEncodeCommand(&first, 3, client.outgoing);
    MpEncodeCommand(&first, 3, expected);
    CHECK(WriteFull(pair[0], client.outgoing, 5));
    client.sent = 5;
    client.sending = 1;
    CHECK(MpClientPendingCommands(&client, pending) == 1);
    CHECK(pending[0].sequence == 3 && pending[0].tick == 10012 && pending[0].input.shiftUp);
    CHECK(client.sent == 5 && client.sending);
    latest = (DriverInput){.brake = 200};
    CHECK(MpClientPollInput(&client, &latest, 10013));
    MpEncodeCommand(&latest, 4, expected + MP_COMMAND_WIRE_SIZE);
    CHECK(ReadLimited(pair[1], actual, sizeof(actual), 5000));
    CHECK(memcmp(actual, expected, sizeof(actual)) == 0);
    CHECK(MpCommandAt(&client.commands, 2)->tick == 10012);
    CHECK(MpCommandAt(&client.commands, 3)->tick == 10013);
    const MpCommands timed = client.commands;
    CHECK(!MpClientPollInput(&client, &latest, 10012));
    CHECK(memcmp(&client.commands, &timed, sizeof(timed)) == 0 && !client.failed);
uint8_t snapshot[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT};
snapshot[1] = 42;
CHECK(MpClientPollMessage(&client, NULL, NULL) == 3);
CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
MpSnapshot decoded;
CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1 && decoded.tick == 42);
    close(pair[1]);
    CHECK(!MpClientPollInput(&client, &latest, 10013));
    CHECK(!MpClientPollInput(&client, &latest, 10013));
    close(pair[0]);
    return 0;
}

static int TestReadDeadline(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    uint8_t bytes[3] = {0xA5, 0xA5, 0xA5};
    CHECK(!ReadLimited(pair[0], bytes, sizeof(bytes), 2));
    CHECK(bytes[0] == 0xA5 && bytes[1] == 0xA5 && bytes[2] == 0xA5);
    const uint8_t prefix[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION};
    CHECK(WriteFull(pair[1], prefix, sizeof(prefix)));
    CHECK(!ReadLimited(pair[0], bytes, sizeof(bytes), 20));
    CHECK(bytes[0] == prefix[0] && bytes[1] == prefix[1] && bytes[2] == 0xA5);
    const uint8_t complete[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 1, 42, 0, 0, 0, 0, 0, 0, 0};
    CHECK(WriteFull(pair[1], complete, sizeof(complete)));
    MpClient client = {.fd = pair[0]};
    int seat = -1;
    CHECK(MpClientRecvWelcome(&client, &seat) && seat == 1);
    close(pair[0]);
    close(pair[1]);
    return 0;
}

static int TestLobbyBeforeStart(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0], .room = 42};
    MpStart start;
    memset(&start, 0xA5, sizeof(start));
    const MpStart original = start;
    uint8_t lobby[53] = {MP_S2C_LOBBY, MP_PROTOCOL_VERSION, 42};
    lobby[12] = 3; lobby[14] = 3; lobby[15] = 2;
    lobby[19] = 4; memcpy(lobby + 20, "Host", 4);
    lobby[35] = 31; lobby[36] = 1; lobby[37] = 5; memcpy(lobby + 38, "Guest", 5);
    for (size_t byte = 0; byte < sizeof(lobby); ++byte) {
        CHECK(WriteFull(pair[1], lobby + byte, 1));
        CHECK(MpClientPollStart(&client, &start) == (byte + 1 == sizeof(lobby) ? 2 : 3));
        CHECK(memcmp(&start, &original, sizeof(start)) == 0);
        if (byte + 1 < sizeof(lobby)) CHECK(!MpClientLobby(&client));
    }
    CHECK(MpClientLobby(&client)->room.ready == 2);
    CHECK(strcmp(MpClientLobby(&client)->seats[1].name, "Guest") == 0);
    CHECK(client.setupDeadline == 0 && client.received == 0);
    CHECK(MpClientPollReady(&client, 1) == 1);
    uint8_t ready[2];
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 1);
    uint8_t packet[1 + MP_START_BODY_SIZE] = {MP_S2C_START, MP_PROTOCOL_VERSION};
    packet[4] = 3; packet[10] = MP_SEAT_LIMIT;
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    CHECK(MpClientPollStart(&client, &start) == 1 && start.laps == 3);
    CHECK(MpClientPollLoaded(&client) == 1);
    uint8_t loaded;
    CHECK(ReadLimited(pair[1], &loaded, 1, 5000) && loaded == MP_C2S_LOADED);
    close(pair[0]); close(pair[1]);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    client = (MpClient){.fd = pair[0], .room = 42};
    CHECK(WriteFull(pair[1], lobby, sizeof(lobby)));
    CHECK(WriteFull(pair[1], packet, sizeof(packet)));
    CHECK(MpClientRecvStart(&client, &start) && start.laps == 3);
    CHECK(MpClientLobby(&client)->seats[1].variant == 31);
    const MpLobby previous = *MpClientLobby(&client);
    lobby[2] = 43; /* A lobby cannot move this connection into another room. */
    CHECK(WriteFull(pair[1], lobby, sizeof(lobby)));
    CHECK(!MpClientPollStart(&client, &start));
    CHECK(memcmp(MpClientLobby(&client), &previous, sizeof(previous)) == 0);
    close(pair[0]); close(pair[1]);
    return 0;
}

static int TestDirectory(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    MpRoomInfo rooms[MP_ROOM_LIMIT];
    memset(rooms, 0xA5, sizeof(rooms));
    MpRoomInfo original[MP_ROOM_LIMIT];
    memcpy(original, rooms, sizeof(rooms));
    size_t count = 99;
    CHECK(MpClientPollList(&client, rooms, &count) == 3);
    uint8_t request;
    CHECK(ReadLimited(pair[1], &request, 1, 5000) && request == MP_C2S_LIST);
    CHECK(!MpClientPollRoom(&client, 0));
    const uint64_t deadline = client.setupDeadline;
    const uint8_t response[] = {MP_S2C_LIST, MP_PROTOCOL_VERSION, 1,
        42, 0, 0, 0, 0, 0, 0, 0, 5, 3, 6, 1, 1, 1, 0};
    for (size_t byte = 0; byte < sizeof(response); ++byte) {
        CHECK(WriteFull(pair[1], response + byte, 1));
        int status = MpClientPollList(&client, rooms, &count);
        CHECK(status == (byte + 1 == sizeof(response) ? 1 : 3));
        CHECK(client.setupDeadline == deadline);
        if (status == 3) CHECK(count == 99 && memcmp(rooms, original, sizeof(rooms)) == 0);
    }
    CHECK(count == 1 && rooms[0].code == 42 && rooms[0].ready == 1);
    CHECK(!ReadLimited(pair[1], &request, 1, 2)); /* No repeated requests. */
    CHECK(MpClientPollList(&client, rooms, &count) == 3);
    CHECK(ReadLimited(pair[1], &request, 1, 5000) && request == MP_C2S_LIST);
    const uint8_t empty[] = {MP_S2C_LIST, MP_PROTOCOL_VERSION, 0};
    CHECK(WriteFull(pair[1], empty, sizeof(empty)));
    CHECK(MpClientPollList(&client, rooms, &count) == 1 && count == 0);
    CHECK(MpClientPollRoom(&client, 42) == 1);
    uint8_t roomRequest[9];
    CHECK(ReadLimited(pair[1], roomRequest, sizeof(roomRequest), 5000));
    CHECK(roomRequest[0] == MP_C2S_ROOM && roomRequest[1] == 42);
    CHECK(!MpClientPollList(&client, rooms, &count));
    close(pair[0]);
    close(pair[1]);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    client = (MpClient){.fd = pair[0]};
    CHECK(MpClientPollList(&client, rooms, &count) == 3);
    memcpy(original, rooms, sizeof(rooms));
    close(pair[1]);
    CHECK(!MpClientPollList(&client, rooms, &count));
    CHECK(count == 0 && memcmp(rooms, original, sizeof(rooms)) == 0);
    close(pair[0]);
    for (int invalid = 0; invalid < 4; ++invalid) {
        CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
        client = (MpClient){.fd = pair[0]};
        count = 99;
        CHECK(MpClientPollList(&client, rooms, &count) == 3);
        uint8_t bad[sizeof(response)];
        memcpy(bad, response, sizeof(bad));
        if (invalid == 0) bad[2] = MP_ROOM_LIMIT + 1;
        if (invalid == 1) bad[1]++;
        if (invalid == 2) bad[16] = 2; /* Ready seat absent. */
        if (invalid == 3) client.setupDeadline = 1;
        CHECK(WriteFull(pair[1], bad, sizeof(bad)));
        CHECK(!MpClientPollList(&client, rooms, &count));
        CHECK(count == 99 && memcmp(rooms, original, sizeof(rooms)) == 0);
        close(pair[0]);
        close(pair[1]);
    }
    return 0;
}

static int TestRoomWait(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    MpStart start;
    memset(&start, 0xA5, sizeof(start));
    const MpStart unchanged = start;
    CHECK(MpClientPollStart(&client, &start) == 3);
    CHECK(client.setupDeadline == 0 && client.received == 0);
    CHECK(MpClientPollStart(&client, &start) == 3 && client.setupDeadline == 0);
    CHECK(MpClientPollReady(&client, 1) == 1);
    uint8_t ready[2];
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 1);
    CHECK(client.setupType == MP_S2C_START && client.setupDeadline == 0);
    const uint8_t header = MP_S2C_START;
    CHECK(WriteFull(pair[1], &header, 1));
    CHECK(MpClientPollStart(&client, &start) == 3);
    CHECK(client.setupDeadline != 0 && client.received == 1);
    const uint64_t deadline = client.setupDeadline;
    CHECK(MpClientPollReady(&client, 0) == 1);
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 0);
    CHECK(client.received == 1 && client.setupDeadline == deadline);
    CHECK(MpClientPollPick(&client, 1, 0) == 1);
    uint8_t pick[3];
    CHECK(ReadLimited(pair[1], pick, sizeof(pick), 5000));
    CHECK(pick[0] == MP_C2S_PICK && pick[1] == 1 && pick[2] == 0);
    CHECK(!client.failed && client.received == 1);
    const uint8_t version = MP_PROTOCOL_VERSION;
    CHECK(WriteFull(pair[1], &version, 1));
    CHECK(MpClientPollStart(&client, &start) == 3 && client.setupDeadline == deadline);
    client.setupDeadline = 1;
    CHECK(MpClientPollStart(&client, &start) == 0);
    CHECK(memcmp(&start, &unchanged, sizeof(start)) == 0);
    client = (MpClient){.fd = pair[0]};
    close(pair[1]);
    CHECK(MpClientPollStart(&client, &start) == 0);
    CHECK(memcmp(&start, &unchanged, sizeof(start)) == 0);
    close(pair[0]);
    return 0;
}

static int TestSetupPolling(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    int seat = -1;
    CHECK(MpClientPollWelcome(&client, &seat) == 3 && seat == -1);
    uint64_t deadline = client.setupDeadline;
    const uint8_t welcome[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 1, 42, 0, 0, 0, 0, 0, 0, 0};
    for (size_t byte = 0; byte < sizeof(welcome); ++byte) {
        CHECK(WriteFull(pair[1], welcome + byte, 1));
        CHECK(MpClientPollWelcome(&client, &seat) == (byte == sizeof(welcome) - 1 ? 1 : 3));
        CHECK(client.setupDeadline == deadline);
        CHECK(seat == (byte == sizeof(welcome) - 1 ? 1 : -1));
        CHECK(MpClientRoom(&client) == (byte == sizeof(welcome) - 1 ? 42 : 0));
    }
    uint8_t start[1 + MP_START_BODY_SIZE] = {MP_S2C_START};
    start[1] = MP_PROTOCOL_VERSION;
    start[4] = 3; /* laps */
    start[10] = MP_SEAT_LIMIT;
    MpStart decoded;
    memset(&decoded, 0xA5, sizeof(decoded));
    MpStart unchanged = decoded;
    for (size_t byte = 0; byte < sizeof(start); ++byte) {
        CHECK(WriteFull(pair[1], start + byte, 1));
        int state = MpClientPollStart(&client, &decoded);
        CHECK(state == (byte + 1 == sizeof(start) ? 1 : 3));
        if (state == 3) CHECK(memcmp(&decoded, &unchanged, sizeof(decoded)) == 0);
    }
    CHECK(decoded.laps == 3);
    CHECK(MpClientPollWelcome(&client, &seat) == 3);
    client.setupDeadline = 1; /* Expired monotonic deadline, no sleep needed. */
    CHECK(MpClientPollWelcome(&client, &seat) == 0 && seat == 1);
    CHECK(MpClientPollWelcome(&client, &seat) == 0);
    client = (MpClient){.fd = pair[0]};
    uint8_t invalid[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION + 1, 0, 42, 0, 0, 0, 0, 0, 0, 0};
    CHECK(WriteFull(pair[1], invalid, sizeof(invalid)));
    CHECK(MpClientPollWelcome(&client, &seat) == 0 && seat == 1);
    client = (MpClient){.fd = pair[0]};
    CHECK(WriteFull(pair[1], welcome, 1));
    CHECK(MpClientPollWelcome(&client, &seat) == 3);
    close(pair[1]);
    CHECK(MpClientPollWelcome(&client, &seat) == 0 && seat == 1);
    close(pair[0]);
    return 0;
}

static int TestConnectPolling(void) {
    CHECK(!MpClientPollConnect(NULL));
    CHECK(!MpClientBeginConnect(NULL, 7878));
    CHECK(!MpClientBeginConnect("127.0.0.1", 0));
    CHECK(!MpClientBeginConnect("999.1.1.1", 7878));
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0], .connecting = 1};
    CHECK(Nonblocking(&client));
    CHECK(Milliseconds(&client.connectDeadline));
    client.connectDeadline += 5000;
    CHECK(MpClientPollConnect(&client) == 1);
    CHECK(!client.connecting && !client.nonblocking);
    CHECK(!(fcntl(pair[0], F_GETFL) & O_NONBLOCK));
    CHECK(MpClientPollConnect(&client) == 1);

    /* A full send buffer gives the same not-ready select branch as a pending
     * connection, without requiring permission to bind a TCP listener. */
    CHECK(Nonblocking(&client));
    char padding[1024] = {0};
    for (;;) {
        IoCount n = send(pair[0], padding, sizeof(padding), 0);
        if (n < 0 && Interrupted()) continue;
        if (n < 0 && WouldBlock()) break;
        CHECK(n > 0);
    }
    client.connecting = 1;
    CHECK(Milliseconds(&client.connectDeadline));
    client.connectDeadline += 5000;
    uint64_t deadline = client.connectDeadline;
    CHECK(MpClientPollConnect(&client) == 3);
    CHECK(client.connectDeadline == deadline && client.connecting);
    client.connectDeadline = 1;
    CHECK(MpClientPollConnect(&client) == 0);
    CHECK(MpClientPollConnect(&client) == 0);
    close(pair[0]);
    close(pair[1]);
    return 0;
}

static int TestSetupWrites(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    CHECK(ConfigureSocket(pair[0]));
    MpClient client = {.fd = pair[0]};
    CHECK(Nonblocking(&client));
    char padding[1024] = {0};
    size_t filled = 0;
    for (;;) {
        IoCount n = send(pair[0], padding, sizeof(padding), 0);
        if (n < 0 && Interrupted()) continue;
        if (n < 0 && WouldBlock()) break;
        CHECK(n > 0);
        filled += (size_t)n;
    }
    CHECK(MpClientPollHello(&client, "Driver") == 3);
    uint64_t deadline = client.writeDeadline;
    CHECK(MpClientPollHello(&client, "Changed") == 3);
    CHECK(client.writeDeadline == deadline);
    while (filled) {
        size_t chunk = filled < sizeof(padding) ? filled : sizeof(padding);
        CHECK(ReadLimited(pair[1], padding, chunk, 5000));
        filled -= chunk;
    }
    /* Simulate a prefix already sent before the next polling call. */
    CHECK(WriteFull(pair[0], client.setupWire, 3));
    client.setupSent = 3;
    CHECK(MpClientPollHello(&client, "Changed") == 1);
    const uint8_t expected[] = {MP_C2S_HELLO, 6, 'D', 'r', 'i', 'v', 'e', 'r'};
    uint8_t actual[sizeof(expected)];
    CHECK(ReadLimited(pair[1], actual, sizeof(actual), 5000));
    CHECK(memcmp(actual, expected, sizeof(actual)) == 0);
    const uint8_t welcome[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 0, 42, 0, 0, 0, 0, 0, 0, 0};
    CHECK(WriteFull(pair[1], welcome, sizeof(welcome)));
    int seat = -1;
    CHECK(MpClientPollWelcome(&client, &seat) == 1 && seat == 0);
    CHECK(!MpClientPollPick(&client, -1, 0));
    CHECK(!MpClientPollPick(&client, 32, 0));
    CHECK(!MpClientPollPick(&client, 31, 2));
    CHECK(MpClientPollPick(&client, 31, 1) == 1);
    uint8_t pick[3];
    CHECK(ReadLimited(pair[1], pick, sizeof(pick), 5000));
    CHECK(pick[0] == MP_C2S_PICK && pick[1] == 31 && pick[2] == 1);
    const MpRaceOptions options = {5, 3, 6, 1};
    CHECK(MpClientPollRace(&client, &options) == 1);
    uint8_t raceOptions[5];
    CHECK(ReadLimited(pair[1], raceOptions, sizeof(raceOptions), 5000));
    const uint8_t expectedRace[] = {MP_C2S_RACE, 5, 3, 6, 1};
    CHECK(memcmp(raceOptions, expectedRace, sizeof(expectedRace)) == 0);
    CHECK(!MpClientPollRace(&client, NULL));
    CHECK(!MpClientPollReady(&client, 2));
    CHECK(MpClientPollReady(&client, 1) == 1);
    uint8_t ready[2];
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 1);
    CHECK(MpClientPollReady(&client, 0) == 1);
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 0);
    CHECK(MpClientPollLoaded(&client) == 1);
    uint8_t loaded;
    CHECK(ReadLimited(pair[1], &loaded, 1, 5000) && loaded == MP_C2S_LOADED);
    CHECK(!ReadLimited(pair[1], &loaded, 1, 2)); /* No duplicate packets. */
    client.setupSize = 1;
    client.setupWire[0] = MP_C2S_LOADED;
    client.writeDeadline = 1;
    CHECK(MpClientPollLoaded(&client) == 0);
    CHECK(MpClientPollLoaded(&client) == 0);
    close(pair[0]);
    close(pair[1]);
    return 0;
}

static int TestTransferBoundaries(void) {
    int pair[2];
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    MpClient client = {.fd = pair[0]};
    const uint8_t prefix[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION};
    CHECK(WriteFull(pair[1], prefix, sizeof(prefix)));
    int seat = -1;
    CHECK(MpClientPollWelcome(&client, &seat) == 3);
    CHECK(client.received == 2);
    CHECK(!MpClientSendHello(&client, "Other"));
    CHECK(!MpClientSendLoaded(&client));
    MpStart start;
    CHECK(!MpClientRecvStart(&client, &start));
    CHECK(!MpClientPollLoaded(&client));
    MpSnapshot snapshot;
    memset(&snapshot, 0xA5, sizeof(snapshot));
    MpSnapshot unchanged = snapshot;
    /* The race parser must never interpret setup's received=2 with its own
     * needed=1: that subtraction used to produce an oversized recv length. */
    CHECK(!MpClientPollMessage(&client, &snapshot, NULL));
    CHECK(memcmp(&snapshot, &unchanged, sizeof(snapshot)) == 0);
    CHECK(client.failed && client.received == 2);

    DriverInput input = {.throttle = 256};
    for (int busy = 0; busy < 3; ++busy) {
        client = (MpClient){.fd = pair[0]};
        if (busy == 0) client.connecting = 1;
        if (busy == 1) client.setupSize = 2;
        if (busy == 2) client.setupType = MP_S2C_START;
        CHECK(!MpClientPollInput(&client, &input, 0));
        CHECK(client.failed && !client.sending && !client.queued);
    }
    uint8_t byte;
    CHECK(!ReadLimited(pair[1], &byte, 1, 2)); /* No interleaved output. */

    client = (MpClient){.fd = pair[0]};
    byte = MP_S2C_SNAPSHOT;
    CHECK(WriteFull(pair[1], &byte, 1));
    CHECK(MpClientPollMessage(&client, &snapshot, NULL) == 3);
    CHECK(client.needed > 1 && client.received == 1);
    CHECK(!MpClientPollWelcome(&client, &seat));
    CHECK(!MpClientPollHello(&client, "Other"));
    CHECK(!MpClientRecvWelcome(&client, &seat));
    CHECK(client.received == 1 && seat == -1);
    close(pair[0]);
    close(pair[1]);
    return 0;
}

int main(void) {
    CHECK(TestLobbyBeforeStart() == 0);
    CHECK(TestDirectory() == 0);
    CHECK(TestRoomWait() == 0);
    CHECK(TestReadDeadline() == 0);
    CHECK(TestSetupPolling() == 0);
    CHECK(TestConnectPolling() == 0);
    CHECK(TestSetupWrites() == 0);
    CHECK(TestTransferBoundaries() == 0);
    CHECK(TestCommandHistoryBackpressure() == 0);
    CHECK(TestAcknowledgements() == 0);
    CHECK(TestSlowPeer() == 0);
    CHECK(TestResultAfterPeerCloses() == 0);
    CHECK(TestRaceReadDeadline() == 0);
    CHECK(TestCorrectionTransfer() == 0);
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
        perror("socketpair");
        return errno == EPERM || errno == EACCES ? 77 : 1;
    }
    CHECK(ConfigureSocket(pair[0]));
    CHECK(ConfigureSocket(pair[1]));
    MpClient client = {.fd = pair[0]};
    CHECK(!MpClientSendLoaded(NULL));
    CHECK(!MpClientSendRoom(&client, (uint64_t)INT64_MAX + 1));
    CHECK(MpClientSendRoom(&client, UINT64_MAX));
    uint8_t room[9];
    CHECK(ReadLimited(pair[1], room, sizeof(room), 5000));
    CHECK(room[0] == MP_C2S_ROOM);
    for (size_t i = 1; i < sizeof(room); ++i) CHECK(room[i] == 255);
    CHECK(!MpClientSendReady(&client, -1));
    CHECK(!MpClientSendPick(&client, 32, 0));
    CHECK(MpClientSendPick(&client, 9, 0));
    uint8_t pick[3];
    CHECK(ReadLimited(pair[1], pick, sizeof(pick), 5000));
    CHECK(pick[0] == MP_C2S_PICK && pick[1] == 9 && pick[2] == 0);
    CHECK(MpClientSendReady(&client, 1));
    uint8_t ready[2];
    CHECK(ReadLimited(pair[1], ready, sizeof(ready), 5000));
    CHECK(ready[0] == MP_C2S_READY && ready[1] == 1);
    CHECK(MpClientSendLoaded(&client));
    uint8_t loaded;
    CHECK(ReadLimited(pair[1], &loaded, 1, 5000) && loaded == MP_C2S_LOADED);
    DriverInput input = {.throttle = 256, .shiftUp = 1};
    uint8_t expected[MP_COMMAND_WIRE_SIZE], received[MP_COMMAND_WIRE_SIZE];
    MpEncodeCommand(&input, 1, expected);
    CHECK(MpClientSendInput(&client, &input));
    CHECK(client.sequence == 1);
    MpClient exhausted = {.fd = pair[0], .sequence = UINT32_MAX};
    CHECK(!MpClientSendInput(&exhausted, &input));
    CHECK(exhausted.sequence == UINT32_MAX);
    CHECK(!MpClientPollInput(&exhausted, &input, 0));
    CHECK(exhausted.failed && exhausted.sequence == UINT32_MAX);
    CHECK(ReadLimited(pair[1], received, sizeof(received), 5000));
    CHECK(memcmp(received, expected, sizeof(received)) == 0);

    int seat = -1;
    uint8_t welcome[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, MP_SEAT_LIMIT, 42, 0, 0, 0, 0, 0, 0, 0};
    CHECK(WriteFull(pair[1], welcome, sizeof(welcome)));
    CHECK(!MpClientRecvWelcome(&client, &seat));
    CHECK(seat == -1);
    welcome[2] = 0;
    welcome[1]++;
    CHECK(WriteFull(pair[1], welcome, sizeof(welcome)));
    CHECK(!MpClientRecvWelcome(&client, &seat));
    CHECK(seat == -1);
    welcome[1] = MP_PROTOCOL_VERSION;
    CHECK(WriteFull(pair[1], welcome, sizeof(welcome)));
    CHECK(MpClientRecvWelcome(&client, &seat) && seat == 0);

    /* A non-restarting signal interrupts the blocked receive before data arrives. */
    struct sigaction action = {0}, previous;
    action.sa_handler = InterruptRead;
    sigemptyset(&action.sa_mask);
    CHECK(sigaction(SIGUSR1, &action, &previous) == 0);
    pid_t signaler = fork();
    CHECK(signaler >= 0);
    if (!signaler) {
        struct timespec pause = {.tv_nsec = 20000000};
        nanosleep(&pause, NULL);
        kill(getppid(), SIGUSR1);
        _exit(WriteFull(pair[1], welcome, sizeof(welcome)) ? 0 : 1);
    }
    CHECK(MpClientRecvWelcome(&client, &seat));
    CHECK(interrupted && seat == 0);
    int signalStatus;
    CHECK(waitpid(signaler, &signalStatus, 0) == signaler);
    CHECK(WIFEXITED(signalStatus) && WEXITSTATUS(signalStatus) == 0);
    CHECK(sigaction(SIGUSR1, &previous, NULL) == 0);

    uint8_t snapshot[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT};
    MpSnapshot decoded, untouched;
    memset(&decoded, 0xA5, sizeof(decoded));
    untouched = decoded;
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 3);
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    snapshot[1] = 1;
    for (size_t byte = 0; byte < sizeof(snapshot); ++byte) {
        CHECK(WriteFull(pair[1], snapshot + byte, 1));
        int message = MpClientPollMessage(&client, &decoded, NULL);
        CHECK(message == (byte + 1 == sizeof(snapshot) ? 1 : 3));
        if (message == 3) CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    }
    CHECK(decoded.tick == 1 && decoded.phase == 0);
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 3);

    /* One poll must consume a whole available message, including its header.
     * Back-to-back messages preserve boundaries without requiring frame waits. */
    snapshot[1] = 2;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    snapshot[1] = 3;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1 && decoded.tick == 3);
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 3);

    /* Queue the full coalescing fixture without blocking its single writer. */
    int sendCapacity = 128 * 1024;
    CHECK(setsockopt(pair[1], SOL_SOCKET, SO_SNDBUF, &sendCapacity, sizeof(sendCapacity)) == 0);
    for (unsigned tick = 4; tick <= 43; ++tick) {
        snapshot[1] = (uint8_t)tick;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    }
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1 && decoded.tick == 35);
    CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1 && decoded.tick == 43);
    const uint8_t finalResult[] = {MP_S2C_RESULT, 1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255};
    snapshot[1] = 44;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    CHECK(WriteFull(pair[1], finalResult, sizeof(finalResult)));
    MpResult deferred;
    memset(&deferred, 0xA5, sizeof(deferred));
    MpResult savedResult = deferred;
    CHECK(MpClientPollMessage(&client, &decoded, &deferred) == 1 && decoded.tick == 44);
    CHECK(memcmp(&deferred, &savedResult, sizeof(deferred)) == 0);
    CHECK(MpClientPollMessage(&client, &decoded, &deferred) == 2);
    CHECK(deferred.seats[0].place == 1 && deferred.seats[0].milliseconds == 1000);
    client = (MpClient){.fd = pair[0]}; /* A new independent session fixture. */

    /* Reject contradictory queued state without committing an earlier pose. */
    for (int invalid = 0; invalid < 2; ++invalid) {
        snapshot[1] = 50;
        snapshot[9] = 2;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        snapshot[1] = invalid ? 51 : 49;
        snapshot[9] = invalid ? 1 : 2;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        untouched = decoded;
        CHECK(!MpClientPollMessage(&client, &decoded, NULL));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        client = (MpClient){.fd = pair[0]};
    }

    for (int terminal = MP_RETIRED; terminal <= MP_FINISHED; terminal += 2) {
        snapshot[1] = 60;
        snapshot[9] = SIM_RACING;
        snapshot[1 + MP_SNAPSHOT_HEADER_SIZE] = terminal;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        snapshot[1] = 61;
        snapshot[1 + MP_SNAPSHOT_HEADER_SIZE] = MP_DRIVING;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        untouched = decoded;
        CHECK(!MpClientPollMessage(&client, &decoded, NULL));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        client = (MpClient){.fd = pair[0]};
    }
    snapshot[1 + MP_SNAPSHOT_HEADER_SIZE] = MP_RETIRED;
    snapshot[9] = 0;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    snapshot[9] = 255;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    untouched = decoded;
    CHECK(!MpClientPollMessage(&client, &decoded, NULL));
    CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
    CHECK(!MpClientPollMessage(&client, NULL, NULL)); /* Failure remains terminal. */
    client = (MpClient){.fd = pair[0]}; /* Independent decoder fixture. */
    uint8_t finish[] = {MP_S2C_RESULT, 1, 1, 0xE8, 3, 0, 0, 0, 0, 255, 255, 255, 255};
    MpResult result;
    CHECK(WriteFull(pair[1], finish, sizeof(finish)));
    CHECK(MpClientRecvMessage(&client, NULL, &result) == 2);
    CHECK(result.seats[0].place == 1 && result.seats[0].milliseconds == 1000);
    CHECK(!result.seats[1].finished && result.seats[1].milliseconds == -1);
    CHECK(WriteFull(pair[1], welcome, 2)); /* Incomplete welcome followed by EOF. */
    close(pair[1]);
    seat = -1;
    CHECK(!MpClientRecvWelcome(&client, &seat));
    CHECK(seat == -1);

    pid_t child = fork();
    CHECK(child >= 0);
    if (!child) {
        signal(SIGPIPE, SIG_DFL);
        _exit(MpClientSendInput(&client, &input) ? 1 : 0);
    }
    int status;
    CHECK(waitpid(child, &status, 0) == child);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    close(pair[0]);
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    client = (MpClient){.fd = pair[0]};
    MpCarConfig config = {.variant = {0, 9}}, receivedConfig;
    config.specs[1].automaticAccelerationScale = 1000;
    uint8_t configPacket[MP_CONFIG_WIRE_SIZE];
    CHECK(MpEncodeConfig(&config, configPacket, sizeof(configPacket)));
    memset(&receivedConfig, 0xA5, sizeof(receivedConfig));
    const MpCarConfig preservedConfig = receivedConfig;
    for (size_t byte = 0; byte < sizeof(configPacket); ++byte) {
        CHECK(WriteFull(pair[1], configPacket + byte, 1));
        int receivedState = MpClientPollConfig(&client, &receivedConfig);
        if (byte + 1 < sizeof(configPacket)) {
            CHECK(receivedState == 3);
            CHECK(memcmp(&receivedConfig, &preservedConfig, sizeof(receivedConfig)) == 0);
        } else CHECK(receivedState == 1);
    }
    CHECK(receivedConfig.variant[1] == 9 && receivedConfig.specs[1].automaticAccelerationScale == 1000);
    CHECK(client.received == 0 && client.setupType == 0 && client.setupDeadline == 0);
    MpCarConfig validConfig = receivedConfig;
    configPacket[1] = 2;
    CHECK(WriteFull(pair[1], configPacket, sizeof(configPacket)));
    CHECK(!MpClientPollConfig(&client, &receivedConfig));
    CHECK(client.failed);
    CHECK(memcmp(&receivedConfig, &validConfig, sizeof(receivedConfig)) == 0);
    CHECK(!MpClientPollConfig(&client, &receivedConfig));
    close(pair[0]); close(pair[1]);
    configPacket[1] = 1;
    for (int failure = 0; failure < 3; ++failure) {
        CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
        client = (MpClient){.fd = pair[0]};
        receivedConfig = validConfig;
        CHECK(WriteFull(pair[1], configPacket, sizeof(configPacket) / 2));
        if (failure == 0) {
            close(pair[1]);
            CHECK(!MpClientRecvConfig(&client, &receivedConfig));
        } else {
            CHECK(MpClientPollConfig(&client, &receivedConfig) == 3);
            uint64_t deadline = client.setupDeadline;
            CHECK(deadline != 0 && client.received == sizeof(configPacket) / 2);
            CHECK(MpClientPollConfig(&client, &receivedConfig) == 3);
            CHECK(client.setupDeadline == deadline);
            if (failure == 1) close(pair[1]);
            else client.setupDeadline = 1;
            CHECK(!MpClientPollConfig(&client, &receivedConfig));
            if (failure == 2) close(pair[1]);
        }
        CHECK(client.failed);
        CHECK(memcmp(&receivedConfig, &validConfig, sizeof(receivedConfig)) == 0);
        close(pair[0]);
    }
    CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    client = (MpClient){.fd = pair[0]};
    uint8_t availability[MP_AVAILABILITY_WIRE_SIZE];
    uint32_t allowed = 123;
    CHECK(MpEncodeAvailability(UINT32_C(0x80000001), availability, sizeof(availability)));
    for (size_t byte = 0; byte < sizeof(availability); ++byte) {
        CHECK(WriteFull(pair[1], availability + byte, 1));
        CHECK(MpClientPollAvailability(&client, &allowed) == (byte + 1 == sizeof(availability) ? 1 : 3));
        CHECK(allowed == (byte + 1 == sizeof(availability) ? UINT32_C(0x80000001) : 123));
    }
    CHECK(WriteFull(pair[1], availability, sizeof(availability)));
    CHECK(MpClientRecvAvailability(&client, &allowed));
    CHECK(allowed == UINT32_C(0x80000001));
    close(pair[0]); close(pair[1]);
    puts("mp_socket: wire transfer, EOF and broken-peer safety pass");
    return 0;
}
