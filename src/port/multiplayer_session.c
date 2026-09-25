#include "multiplayer_session.h"

#include <stdio.h>
#include <string.h>

enum {
    SESSION_COURSE_MAX = 3,
    SESSION_CLASS_MAX = 5,
    SESSION_CAR_MAX = 12
};

static int InRange(int value, int maximum) {
    return value >= 0 && value <= maximum;
}

static int ValidSession(const MultiplayerSession *session) {
    return session != NULL &&
           InRange(session->course, SESSION_COURSE_MAX) &&
           InRange(session->classIndex, SESSION_CLASS_MAX) &&
           InRange(session->car, SESSION_CAR_MAX);
}

void MultiplayerServerBegin(MultiplayerServer *server,
                            MultiplayerSession decision) {
    if (server == NULL) return;
    server->decision = decision;
    server->waiting = 1;
    server->started = 0;
}

int MultiplayerServerJoin(MultiplayerServer *server) {
    if (server == NULL || !server->waiting || server->started) return 0;
    if (!ValidSession(&server->decision)) return 0;
    server->waiting = 0;
    server->started = 1;
    return 1;
}

int MultiplayerServerSession(const MultiplayerServer *server,
                             MultiplayerSession *out) {
    if (server == NULL || out == NULL || !server->started) return 0;
    *out = server->decision;
    return 1;
}

void MultiplayerClientBegin(MultiplayerClient *client,
                            MultiplayerSession wish) {
    if (client == NULL) return;
    client->wish = wish;
    client->playing = wish;
    client->joined = 0;
}

void MultiplayerClientApply(MultiplayerClient *client,
                            MultiplayerSession decision) {
    if (client == NULL || !ValidSession(&decision)) return;
    client->playing = decision;
    client->joined = 1;
}

int MultiplayerSessionSame(const MultiplayerSession *left,
                           const MultiplayerSession *right) {
    if (left == NULL || right == NULL) return 0;
    return left->course == right->course &&
           left->classIndex == right->classIndex &&
           left->car == right->car;
}

int MultiplayerSessionFormat(const MultiplayerSession *session, char *text,
                             size_t size) {
    int written;

    if (!ValidSession(session) || text == NULL || size == 0) return 0;
    written = snprintf(text, size, "RAGE1 course=%d class=%d car=%d",
                       session->course, session->classIndex, session->car);
    if (written < 0 || (size_t)written >= size) {
        if (size > 0) text[0] = '\0';
        return 0;
    }
    return 1;
}

int MultiplayerSessionParse(const char *text, MultiplayerSession *session) {
    MultiplayerSession parsed;
    char extra;

    if (text == NULL || session == NULL) return 0;
    extra = '\0';
    if (sscanf(text, "RAGE1 course=%d class=%d car=%d %c",
               &parsed.course, &parsed.classIndex, &parsed.car, &extra) != 3) {
        return 0;
    }
    if (!ValidSession(&parsed)) return 0;
    *session = parsed;
    return 1;
}

static int IsIpv4(const char *text) {
    int parts = 0;
    const char *cursor = text;

    if (text == NULL || text[0] == '\0') return 0;
    while (*cursor != '\0') {
        int value = 0;
        int digits = 0;

        while (*cursor >= '0' && *cursor <= '9') {
            value = value * 10 + (*cursor - '0');
            digits++;
            cursor++;
            if (digits > 3 || value > 255) return 0;
        }
        if (digits == 0) return 0;
        parts++;
        if (*cursor == '.') {
            cursor++;
            if (*cursor == '\0') return 0;
        } else if (*cursor != '\0') {
            return 0;
        }
    }
    return parts == 4;
}

static int ParsePort(const char *text, int *port) {
    int value = 0;
    const char *cursor = text;

    if (text == NULL || text[0] == '\0') return 0;
    while (*cursor >= '0' && *cursor <= '9') {
        value = value * 10 + (*cursor - '0');
        cursor++;
        if (value > 65535) return 0;
    }
    if (*cursor != '\0' || value < 1) return 0;
    *port = value;
    return 1;
}

int MultiplayerParseCommand(int argc, char **argv,
                            MultiplayerCommand *command) {
    int index;

    if (command == NULL) return 0;
    command->role = MULTIPLAYER_ROLE_NONE;
    command->port = MULTIPLAYER_PORT_DEFAULT;
    command->host[0] = '\0';
    if (argc < 0 || (argc > 0 && argv == NULL)) return 0;
    for (index = 1; index < argc; index++) {
        const char *arg = argv[index];

        if (arg == NULL) return 0;
        if (!strcmp(arg, "--host")) {
            if (command->role != MULTIPLAYER_ROLE_NONE) return 0;
            command->role = MULTIPLAYER_ROLE_SERVER;
        } else if (!strcmp(arg, "--join")) {
            size_t length;

            if (command->role != MULTIPLAYER_ROLE_NONE) return 0;
            if (index + 1 >= argc || argv[index + 1] == NULL) return 0;
            arg = argv[++index];
            if (!IsIpv4(arg)) return 0;
            length = strlen(arg);
            if (length >= sizeof(command->host)) return 0;
            memcpy(command->host, arg, length + 1);
            command->role = MULTIPLAYER_ROLE_CLIENT;
        } else if (!strcmp(arg, "--port")) {
            if (index + 1 >= argc || !ParsePort(argv[index + 1], &command->port))
                return 0;
            index++;
        }
    }
    return 1;
}
