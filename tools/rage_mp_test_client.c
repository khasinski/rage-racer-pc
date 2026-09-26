/*
 * rage-mp-test-client - a minimal test client for rage-racer-server (see
 * docs/multiplayer.md, "Order of work" step 2/3). It does not render
 * anything: it connects, sends a Hello, drives straight ahead with a fixed
 * input, and prints every received snapshot so two instances of this tool
 * can prove the server actually steps two independently-connected human
 * seats and reports back real, moving positions.
 *
 * usage: rage-mp-test-client <host> <port> <name> [ticks]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

enum {
    C2S_HELLO = 0x01,
    C2S_INPUT = 0x02,
    S2C_WELCOME = 0x81,
    S2C_START = 0x82,
    S2C_SNAPSHOT = 0x83,
    S2C_RESULT = 0x84
};

static int ReadFull(int fd, void *buffer, size_t size) {
    uint8_t *p = buffer;
    size_t got = 0;
    while (got < size) {
        ssize_t n = read(fd, p + got, size - got);
        if (n <= 0) return 0;
        got += (size_t)n;
    }
    return 1;
}

static int WriteFull(int fd, const void *buffer, size_t size) {
    const uint8_t *p = buffer;
    size_t sent = 0;
    while (sent < size) {
        ssize_t n = write(fd, p + sent, size - sent);
        if (n <= 0) return 0;
        sent += (size_t)n;
    }
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

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "usage: %s <host> <port> <name> [ticks]\n", argv[0]);
        return 1;
    }
    const char *host = argv[1];
    int port = atoi(argv[2]);
    const char *name = argv[3];
    int maxTicks = argc > 4 ? atoi(argv[4]) : 200;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid host: %s\n", host);
        return 1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("connect");
        return 1;
    }
    fprintf(stderr, "%s: connected to %s:%d\n", name, host, port);

    size_t nameLen = strlen(name);
    if (nameLen > 15) nameLen = 15;
    uint8_t hello[2 + 15];
    hello[0] = C2S_HELLO;
    hello[1] = (uint8_t)nameLen;
    memcpy(hello + 2, name, nameLen);
    if (!WriteFull(fd, hello, 2 + nameLen)) { perror("write hello"); return 1; }

    uint8_t welcome[2];
    if (!ReadFull(fd, welcome, sizeof(welcome)) || welcome[0] != S2C_WELCOME) {
        fprintf(stderr, "%s: did not receive welcome\n", name);
        return 1;
    }
    int seat = welcome[1];
    fprintf(stderr, "%s: welcomed as seat %d\n", name, seat);

    uint8_t start[7];
    if (!ReadFull(fd, start, sizeof(start)) || start[0] != S2C_START) {
        fprintf(stderr, "%s: did not receive start\n", name);
        return 1;
    }
    fprintf(stderr, "%s: race starting, laps=%u reverse=%u countdown=%u ticks\n",
            name, start[1], start[2], GetLE32(start + 3));

    /* Drive straight ahead at full throttle, no steering, no braking. This
     * proves the pipe carries real input and real physics respond to it;
     * it is not meant to finish a lap cleanly. */
    uint8_t input[1 + 12];
    input[0] = C2S_INPUT;
    input[1] = 0;   /* steering mode: center */
    input[2] = 0;   /* steering left */
    input[3] = 0;   /* steering right */
    PutLE16(input + 4, 0);      /* steering angle */
    PutLE16(input + 6, 256);    /* throttle: full */
    PutLE16(input + 8, 0);      /* brake: none */
    input[10] = 0;  /* shift up */
    input[11] = 0;  /* shift down */

    int ticks = 0;
    while (ticks < maxTicks) {
        if (!WriteFull(fd, input, sizeof(input))) {
            fprintf(stderr, "%s: write input failed\n", name);
            break;
        }
        uint8_t header;
        if (!ReadFull(fd, &header, 1)) {
            fprintf(stderr, "%s: connection closed\n", name);
            break;
        }
        if (header == S2C_RESULT) {
            fprintf(stderr, "%s: race finished\n", name);
            break;
        }
        if (header != S2C_SNAPSHOT) {
            fprintf(stderr, "%s: unexpected message %#x\n", name, header);
            break;
        }
        uint8_t body[4 + 1 + 2 * 17];
        if (!ReadFull(fd, body, sizeof(body))) {
            fprintf(stderr, "%s: short snapshot\n", name);
            break;
        }
        uint32_t tick = GetLE32(body);
        uint8_t phase = body[4];
        const uint8_t *seat0 = body + 5;
        const uint8_t *seat1 = body + 5 + 17;
        if (ticks % 20 == 0) {
            fprintf(stderr,
                    "%s: tick=%u phase=%u seat0(active=%u x=%d z=%d) "
                    "seat1(active=%u x=%d z=%d)\n",
                    name, tick, phase, seat0[0], GetLE32Signed(seat0 + 1),
                    GetLE32Signed(seat0 + 9), seat1[0], GetLE32Signed(seat1 + 1),
                    GetLE32Signed(seat1 + 9));
        }
        ++ticks;
    }
    close(fd);
    fprintf(stderr, "%s: done after %d ticks\n", name, ticks);
    return 0;
}
