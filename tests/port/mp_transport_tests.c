/* Exercise the actual headless client process against a bounded TCP fixture. */
#include "port/mp_client.h"
#include "game/race_sim.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static int Transfer(int fd, void *buffer, size_t size, int sending) {
    unsigned char *bytes = buffer;
    while (size) {
        ssize_t n = sending ? write(fd, bytes, size) : read(fd, bytes, size);
        if (n <= 0) return 0;
        bytes += n;
        size -= (size_t)n;
    }
    return 1;
}

enum { SAMPLE, BAD_SNAPSHOT, FINISH, BAD_RESULT };

static int Run(const char *executable, int scenario) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) { perror("socket"); return 0; }
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t length = sizeof(address);
    if (bind(listener, (struct sockaddr *)&address, length) ||
        getsockname(listener, (struct sockaddr *)&address, &length) ||
        listen(listener, 1)) {
        int denied = errno == EPERM || errno == EACCES;
        perror("listen fixture");
        close(listener);
        return denied ? 77 : 0;
    }
    char port[8];
    snprintf(port, sizeof(port), "%u", ntohs(address.sin_port));
    pid_t child = fork();
    if (child == 0) {
        close(listener);
        execl(executable, executable, "127.0.0.1", port, "driver",
              scenario >= FINISH ? "2" : "1", (char *)NULL);
        _exit(127);
    }
    if (child < 0) {
        close(listener);
        return 0;
    }
    int fd = accept(listener, NULL, NULL);
    close(listener);
    int ok = fd >= 0;
    if (ok) {
        struct timeval timeout = {.tv_sec = 3};
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        unsigned char hello[8];
        unsigned char expectedHello[] = {MP_C2S_HELLO, 6, 'd', 'r', 'i', 'v', 'e', 'r'};
        ok = Transfer(fd, hello, sizeof(hello), 0) &&
             memcmp(hello, expectedHello, sizeof(hello)) == 0;
        unsigned char start[11 + 1 + MP_START_BODY_SIZE] = {
            MP_S2C_WELCOME, MP_PROTOCOL_VERSION, 0, 42, 0, 0, 0, 0, 0, 0, 0,
            MP_S2C_START, MP_PROTOCOL_VERSION, 0, 0, 3, 0, 150, 0, 0, 0, MP_SEAT_LIMIT
        };
        unsigned char room[9];
        const unsigned char expectedRoom[] = {MP_C2S_ROOM, 255, 255, 255, 255, 255, 255, 255, 255};
        ok = ok && Transfer(fd, room, sizeof(room), 0) &&
             memcmp(room, expectedRoom, sizeof(room)) == 0;
        ok = ok && Transfer(fd, start, 11, 1);
        unsigned char availability[MP_AVAILABILITY_WIRE_SIZE] = {0x89, 1};
        ok = ok && Transfer(fd, availability, sizeof(availability), 1);
        unsigned char ready[2];
        ok = ok && Transfer(fd, ready, sizeof(ready), 0) &&
             ready[0] == MP_C2S_READY && ready[1] == 1;
        unsigned char lobby[53] = {MP_S2C_LOBBY, MP_PROTOCOL_VERSION, 42};
        lobby[12] = 3; lobby[14] = 3; lobby[15] = 3;
        ok = ok && Transfer(fd, lobby, sizeof(lobby), 1);
        ok = ok && Transfer(fd, start + 11, sizeof(start) - 11, 1);
        unsigned char configPacket[MP_CONFIG_WIRE_SIZE] = {0x88, 1};
        ok = ok && Transfer(fd, configPacket, sizeof(configPacket), 1);
        unsigned char loaded;
        ok = ok && Transfer(fd, &loaded, 1, 0) && loaded == MP_C2S_LOADED;
        unsigned char input[MP_COMMAND_WIRE_SIZE];
        unsigned char expectedInput[MP_COMMAND_WIRE_SIZE] = {MP_C2S_COMMAND, 1};
        expectedInput[11] = 1; /* Full throttle: little-endian 256. */
        ok = ok && Transfer(fd, input, sizeof(input), 0) &&
             memcmp(input, expectedInput, sizeof(input)) == 0;
        unsigned char snapshot[1 + MP_SNAPSHOT_BODY_SIZE] = {MP_S2C_SNAPSHOT};
        snapshot[1] = 1;
        snapshot[9] = scenario == BAD_SNAPSHOT ? 255 :
                      scenario >= FINISH ? SIM_FINISHED : SIM_COUNTDOWN;
        /* Send one byte at a time: socket reads need not match messages. */
        for (size_t i = 0; ok && i < sizeof(snapshot); ++i)
            ok = Transfer(fd, &snapshot[i], 1, 1);
        if (scenario >= FINISH) {
            unsigned char result[] = {MP_S2C_RESULT, 0, 0, 255, 255, 255, 255,
                                                    0, 0, 255, 255, 255, 255};
            if (scenario == BAD_RESULT) {
                result[1] = result[2] = 1;
                memset(result + 3, 0, 4); /* Valid packet contradicts retired snapshot. */
            }
            ok = ok && Transfer(fd, result, sizeof(result), 1);
        }
        close(fd);
    }
    if (!ok) kill(child, SIGKILL);
    int status;
    if (waitpid(child, &status, 0) != child) return 0;
    return ok && WIFEXITED(status) && WEXITSTATUS(status) ==
        ((scenario == BAD_SNAPSHOT || scenario == BAD_RESULT) ? 1 : 0);
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    alarm(10); /* Bound failures before accept or waitpid. */
    if (argc != 2) return 1;
    int result = Run(argv[1], SAMPLE);
    if (result == 77) return 77;
    if (!result || Run(argv[1], BAD_SNAPSHOT) != 1 ||
        Run(argv[1], FINISH) != 1 || Run(argv[1], BAD_RESULT) != 1) {
        fprintf(stderr, "mp_transport: client wire or exit status mismatch\n");
        return 1;
    }
    puts("mp_transport: shared client sends initialized input and rejects invalid snapshots");
    return 0;
}
