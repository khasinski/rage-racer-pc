#ifndef RAGE_MULTIPLAYER_NET_H
#define RAGE_MULTIPLAYER_NET_H

#include "multiplayer_session.h"

/* Block until one client joins, then publish the server's session.
 * Returns 0 if the socket fails or the session was not waiting. */
int MultiplayerServerAccept(MultiplayerServer *server, int port);

/* Connect to host, read the server's session, and adopt it over `client`'s
 * own wish. `host` is the address from the command line. */
int MultiplayerClientConnect(MultiplayerClient *client, const char *host,
                             int port);

/* Copy a joined session into the runtime the rest of the game already reads. */
void MultiplayerInstallSession(const MultiplayerSession *session);

/* Run the command parsed from the real process arguments. A process that is
 * neither host nor client returns success without touching the network. */
int MultiplayerRunCommand(const MultiplayerCommand *command);

#endif
