/* A local socket pair exercises the real private transfer loops and setup,
 * without a listening port, game assets or a test-only production API. */
#include "../../src/port/mp_client.c"
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>

static volatile sig_atomic_t interrupted;
static void InterruptRead(int number) { (void)number; interrupted = 1; }

#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

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
    CHECK(MpClientPollInput(&client, &first));
    CHECK(client.sending && client.sent == 0);
    DriverInput latest = {0};
    for (int frame = 0; frame < 10000; ++frame) {
        latest = (DriverInput){.throttle = frame % 257,
                              .shiftUp = frame == 17, .shiftDown = frame == 19};
        CHECK(MpClientPollInput(&client, &latest));
        CHECK(client.sending && client.sent == 0 && client.queued);
    }
    while (filled) {
        size_t chunk = filled < sizeof(padding) ? filled : sizeof(padding);
        CHECK(ReadFull(pair[1], padding, chunk));
        filled -= chunk;
    }
    CHECK(MpClientPollInput(&client, &latest));
    uint8_t actual[2 * MP_INPUT_WIRE_SIZE], expected[2 * MP_INPUT_WIRE_SIZE];
    MpEncodeInput(&first, expected);
    latest.shiftUp = latest.shiftDown = 1;
    MpEncodeInput(&latest, expected + MP_INPUT_WIRE_SIZE);
    CHECK(ReadFull(pair[1], actual, sizeof(actual)));
    CHECK(memcmp(actual, expected, sizeof(actual)) == 0);
    CHECK(!client.sending && !client.queued);

    /* A previously written prefix must stay intact when newer controls arrive. */
    MpEncodeInput(&first, client.outgoing);
    CHECK(WriteFull(pair[0], client.outgoing, 5));
    client.sent = 5;
    client.sending = 1;
    latest = (DriverInput){.brake = 200};
    CHECK(MpClientPollInput(&client, &latest));
    MpEncodeInput(&latest, expected + MP_INPUT_WIRE_SIZE);
    CHECK(ReadFull(pair[1], actual, sizeof(actual)));
    CHECK(memcmp(actual, expected, sizeof(actual)) == 0);
uint8_t snapshot[1 + MP_SNAPSHOT_HEADER_SIZE + MP_SEAT_LIMIT * MP_SNAPSHOT_SEAT_SIZE] = {MP_S2C_SNAPSHOT};
snapshot[1] = 42;
CHECK(MpClientPollMessage(&client, NULL, NULL) == 3);
CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
MpSnapshot decoded;
CHECK(MpClientPollMessage(&client, &decoded, NULL) == 1 && decoded.tick == 42);
    close(pair[1]);
    CHECK(!MpClientPollInput(&client, &latest));
    CHECK(!MpClientPollInput(&client, &latest));
    close(pair[0]);
    return 0;
}

int main(void) {
    CHECK(TestSlowPeer() == 0);
    int pair[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair)) {
        perror("socketpair");
        return errno == EPERM || errno == EACCES ? 77 : 1;
    }
    CHECK(ConfigureSocket(pair[0]));
    CHECK(ConfigureSocket(pair[1]));
    MpClient client = {.fd = pair[0]};
    CHECK(!MpClientSendLoaded(NULL));
    CHECK(MpClientSendLoaded(&client));
    uint8_t loaded;
    CHECK(ReadFull(pair[1], &loaded, 1) && loaded == MP_C2S_LOADED);
    DriverInput input = {.throttle = 256, .shiftUp = 1};
    uint8_t expected[MP_INPUT_WIRE_SIZE], received[MP_INPUT_WIRE_SIZE];
    MpEncodeInput(&input, expected);
    CHECK(MpClientSendInput(&client, &input));
    CHECK(ReadFull(pair[1], received, sizeof(received)));
    CHECK(memcmp(received, expected, sizeof(received)) == 0);

    int seat = -1;
    uint8_t welcome[] = {MP_S2C_WELCOME, MP_PROTOCOL_VERSION, MP_SEAT_LIMIT};
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

    uint8_t snapshot[1 + MP_SNAPSHOT_HEADER_SIZE + MP_SEAT_LIMIT * MP_SNAPSHOT_SEAT_SIZE] = {MP_S2C_SNAPSHOT};
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
        snapshot[5] = 2;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        snapshot[1] = invalid ? 51 : 49;
        snapshot[5] = invalid ? 1 : 2;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        untouched = decoded;
        CHECK(!MpClientPollMessage(&client, &decoded, NULL));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        client = (MpClient){.fd = pair[0]};
    }

    for (int terminal = MP_RETIRED; terminal <= MP_FINISHED; terminal += 2) {
        snapshot[1] = 60;
        snapshot[5] = SIM_RACING;
        snapshot[6] = terminal;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        snapshot[1] = 61;
        snapshot[6] = MP_DRIVING;
        CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
        untouched = decoded;
        CHECK(!MpClientPollMessage(&client, &decoded, NULL));
        CHECK(memcmp(&decoded, &untouched, sizeof(decoded)) == 0);
        client = (MpClient){.fd = pair[0]};
    }
    snapshot[6] = MP_RETIRED;
    snapshot[5] = 0;
    CHECK(WriteFull(pair[1], snapshot, sizeof(snapshot)));
    snapshot[5] = 255;
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
    puts("mp_socket: wire transfer, EOF and broken-peer safety pass");
    return 0;
}
