#ifndef RAGE_MULTIPLAYER_SESSION_H
#define RAGE_MULTIPLAYER_SESSION_H

#include <stddef.h>

/* Proof-of-concept authority for one client. The server keeps a session it
 * has already chosen, but that session is not live until a client joins.
 * The client may have picked a different course; after the join it plays
 * the server's choice. */

enum { MULTIPLAYER_PORT_DEFAULT = 27888 };

typedef struct MultiplayerSession {
    int course;
    int classIndex;
    int car;
} MultiplayerSession;

typedef struct MultiplayerServer {
    int waiting;
    int started;
    MultiplayerSession decision;
} MultiplayerServer;

typedef struct MultiplayerClient {
    int joined;
    MultiplayerSession wish;
    MultiplayerSession playing;
} MultiplayerClient;

typedef enum MultiplayerRole {
    MULTIPLAYER_ROLE_NONE = 0,
    MULTIPLAYER_ROLE_SERVER,
    MULTIPLAYER_ROLE_CLIENT
} MultiplayerRole;

typedef struct MultiplayerCommand {
    MultiplayerRole role;
    int port;
    char host[64];
} MultiplayerCommand;

void MultiplayerServerBegin(MultiplayerServer *server,
                            MultiplayerSession decision);
/* Returns 0 while the server is still waiting. The decision is not a live
 * session until this succeeds. */
int MultiplayerServerJoin(MultiplayerServer *server);
int MultiplayerServerSession(const MultiplayerServer *server,
                             MultiplayerSession *out);

void MultiplayerClientBegin(MultiplayerClient *client,
                            MultiplayerSession wish);
void MultiplayerClientApply(MultiplayerClient *client,
                            MultiplayerSession decision);

int MultiplayerSessionSame(const MultiplayerSession *left,
                           const MultiplayerSession *right);
int MultiplayerSessionFormat(const MultiplayerSession *session, char *text,
                             size_t size);
int MultiplayerSessionParse(const char *text, MultiplayerSession *session);

/* `--host` waits for one client. `--join <ipv4>` connects to that host.
 * `--port` overrides the default. Anything else is left for the existing
 * argument parser. */
int MultiplayerParseCommand(int argc, char **argv,
                            MultiplayerCommand *command);

#endif
