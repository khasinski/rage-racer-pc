/* Headless driver for the same protocol adapter used by the game.
 * usage: rage-mp-test-client <IPv4 address> <port> <name> [snapshots] */
#include "port/mp_client.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int PositiveNumber(const char *text, long maximum, int *out) {
    char *end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno || end == text || *end || value < 1 || value > maximum) return 0;
    *out = (int)value;
    return 1;
}

int main(int argc, char **argv) {
    int port, maxSnapshots = 200;
    if (argc < 4 || argc > 5 || !PositiveNumber(argv[2], 65535, &port) ||
        (argc == 5 && !PositiveNumber(argv[4], 1000000, &maxSnapshots))) {
        fprintf(stderr, "usage: %s <IPv4 address> <port> <name> [snapshots]\n", argv[0]);
        return 1;
    }
    const char *name = argv[3];
    MpClient *client = MpClientConnect(argv[1], (uint16_t)port);
    if (!client) {
        fprintf(stderr, "%s: connection failed\n", name);
        return 1;
    }
    int status = 1, seat;
    MpStart start;
    if (!MpClientSendHello(client, name) || !MpClientRecvWelcome(client, &seat) ||
        !MpClientRecvStart(client, &start)) {
        fprintf(stderr, "%s: handshake failed\n", name);
        goto done;
    }
    fprintf(stderr, "%s: seat=%d laps=%d reverse=%d countdown=%u\n",
            name, seat, start.laps, start.reverse, start.countdown);
    if (!MpClientSendLoaded(client)) goto done;
    const DriverInput input = {.steering = {.mode = STEERING_CENTER}, .throttle = 256};
    for (int received = 0; received < maxSnapshots; ++received) {
        MpSnapshot snapshot;
        MpResult result;
        if (!MpClientSendInput(client, &input)) {
            fprintf(stderr, "%s: input send failed\n", name);
            goto done;
        }
        int message = MpClientRecvMessage(client, &snapshot, &result);
        if (!message) {
            fprintf(stderr, "%s: disconnected or invalid snapshot\n", name);
            goto done;
        }
        if (message == 2) {
            fprintf(stderr, "%s: race finished, place=%d time=%d ms\n", name, result.seats[seat].place, result.seats[seat].milliseconds);
            status = 0;
            goto done;
        }
        if (received % 20 == 0) {
            fprintf(stderr, "%s: tick=%u phase=%u seat0(status=%d x=%d z=%d) "
                    "seat1(status=%d x=%d z=%d)\n", name, snapshot.tick,
                    snapshot.phase, snapshot.seats[0].status, snapshot.seats[0].x,
                    snapshot.seats[0].z, snapshot.seats[1].status,
                    snapshot.seats[1].x, snapshot.seats[1].z);
        }
    }
    status = 0;
done:
    MpClientClose(client);
    return status;
}
