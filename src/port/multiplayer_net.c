#include "multiplayer_net.h"

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET NetSocket;
#define NET_INVALID INVALID_SOCKET
#define NET_CLOSE closesocket
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int NetSocket;
#define NET_INVALID (-1)
#define NET_CLOSE close
#endif

#include "runtime_config.h"

static void NetStartup(void) {
#ifdef _WIN32
    static int ready;
    WSADATA data;

    if (ready) return;
    if (WSAStartup(MAKEWORD(2, 2), &data) == 0) ready = 1;
#else
#endif
}

static int SendAll(NetSocket socket, const char *text) {
    size_t length = strlen(text);
    size_t sent = 0;

    while (sent < length) {
#ifdef _WIN32
        int wrote = send(socket, text + sent, (int)(length - sent), 0);
#else
        ssize_t wrote = send(socket, text + sent, length - sent, 0);
#endif
        if (wrote <= 0) return 0;
        sent += (size_t)wrote;
    }
    return 1;
}

static int ReadLine(NetSocket socket, char *text, size_t size) {
    size_t used = 0;

    if (text == NULL || size == 0) return 0;
    while (used + 1 < size) {
        char character;
#ifdef _WIN32
        int got = recv(socket, &character, 1, 0);
#else
        ssize_t got = recv(socket, &character, 1, 0);
#endif
        if (got <= 0) return 0;
        if (character == '\n') {
            text[used] = '\0';
            return 1;
        }
        if (character != '\r') text[used++] = character;
    }
    return 0;
}

static MultiplayerSession DecisionFromConfig(void) {
    MultiplayerSession session;

    session.course = RuntimeConfigInt("race.course", 2, 0, 3);
    session.classIndex = RuntimeConfigInt("race.class", 3, 0, 5);
    session.car = RuntimeConfigInt("race.car", 9, 0, 12);
    return session;
}

void MultiplayerInstallSession(const MultiplayerSession *session) {
    char value[16];

    if (session == NULL) return;
    snprintf(value, sizeof(value), "%d", session->course);
    RuntimeConfigSet("race.course", value);
    snprintf(value, sizeof(value), "%d", session->classIndex);
    RuntimeConfigSet("race.class", value);
    snprintf(value, sizeof(value), "%d", session->car);
    RuntimeConfigSet("race.car", value);
    RuntimeConfigSet("race.enabled", "true");
    RuntimeConfigSet("race.mode", "grand-prix");
    RuntimeConfigSet("race.series", "grand-prix");
}

int MultiplayerServerAccept(MultiplayerServer *server, int port) {
    NetSocket listener = NET_INVALID;
    NetSocket client = NET_INVALID;
    struct sockaddr_in address;
    char line[128];
    int reuse = 1;
    int accepted = 0;

    if (server == NULL || !server->waiting || server->started) return 0;
    if (port < 1 || port > 65535) return 0;
    NetStartup();
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == NET_INVALID) {
        fprintf(stderr, "rage-port: multiplayer server cannot open a socket\n");
        return 0;
    }
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse,
               sizeof(reuse));
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);
    if (bind(listener, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        listen(listener, 1) != 0) {
        fprintf(stderr, "rage-port: multiplayer server cannot listen on %d\n",
                port);
        NET_CLOSE(listener);
        return 0;
    }
    fprintf(stderr, "rage-port: multiplayer server waiting for a client\n");
    fflush(stderr);
    client = accept(listener, NULL, NULL);
    NET_CLOSE(listener);
    if (client == NET_INVALID) {
        fprintf(stderr, "rage-port: multiplayer server failed to accept\n");
        return 0;
    }
    /* The socket is connected, but the session stays unstarted until the
     * join rule accepts it. A client that arrives does not invent the race. */
    if (!MultiplayerServerJoin(server) ||
        !MultiplayerSessionFormat(&server->decision, line, sizeof(line))) {
        NET_CLOSE(client);
        return 0;
    }
    accepted = SendAll(client, line) && SendAll(client, "\n");
    NET_CLOSE(client);
    if (!accepted) {
        fprintf(stderr, "rage-port: multiplayer server could not publish the session\n");
        return 0;
    }
    fprintf(stderr,
            "rage-port: multiplayer server accepted session course=%d class=%d car=%d\n",
            server->decision.course, server->decision.classIndex,
            server->decision.car);
    fflush(stderr);
    return 1;
}

int MultiplayerClientConnect(MultiplayerClient *client, const char *host,
                             int port) {
    NetSocket connection = NET_INVALID;
    struct sockaddr_in address;
    MultiplayerSession decision;
    char line[128];

    if (client == NULL || host == NULL || host[0] == '\0') return 0;
    if (client->joined || port < 1 || port > 65535) return 0;
    NetStartup();
    connection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connection == NET_INVALID) return 0;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, host, &address.sin_addr) != 1 ||
        connect(connection, (struct sockaddr *)&address, sizeof(address)) != 0 ||
        !ReadLine(connection, line, sizeof(line))) {
        fprintf(stderr, "rage-port: multiplayer client cannot join %s\n", host);
        NET_CLOSE(connection);
        return 0;
    }
    NET_CLOSE(connection);
    if (!MultiplayerSessionParse(line, &decision)) {
        fprintf(stderr, "rage-port: multiplayer client rejected the server session\n");
        return 0;
    }
    MultiplayerClientApply(client, decision);
    if (!client->joined) return 0;
    fprintf(stderr,
            "rage-port: multiplayer client joined %s session course=%d class=%d car=%d"
            " (local course=%d class=%d car=%d was not used)\n",
            host, client->playing.course, client->playing.classIndex,
            client->playing.car, client->wish.course, client->wish.classIndex,
            client->wish.car);
    fflush(stderr);
    return 1;
}

int MultiplayerRunCommand(const MultiplayerCommand *command) {
    if (command == NULL || command->role == MULTIPLAYER_ROLE_NONE) return 1;
    if (command->role == MULTIPLAYER_ROLE_SERVER) {
        MultiplayerServer server;

        MultiplayerServerBegin(&server, DecisionFromConfig());
        if (!server.waiting || server.started) return 0;
        if (!MultiplayerServerAccept(&server, command->port)) return 0;
        MultiplayerInstallSession(&server.decision);
        return 1;
    }
    if (command->role == MULTIPLAYER_ROLE_CLIENT) {
        MultiplayerClient client;

        MultiplayerClientBegin(&client, DecisionFromConfig());
        if (client.joined) return 0;
        if (!MultiplayerClientConnect(&client, command->host, command->port) ||
            !client.joined) {
            return 0;
        }
        MultiplayerInstallSession(&client.playing);
        return 1;
    }
    return 0;
}
