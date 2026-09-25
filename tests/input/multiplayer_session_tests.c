#include "multiplayer_session.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

static void Check(int condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static MultiplayerSession Choice(int course, int classIndex, int car) {
    MultiplayerSession session;

    session.course = course;
    session.classIndex = classIndex;
    session.car = car;
    return session;
}

static void TestServerWaitsUntilJoin(void) {
    MultiplayerServer server;
    MultiplayerClient client;
    MultiplayerSession decided;
    MultiplayerSession wire;
    char line[128];
    char *serverArgv[] = {"rage-racer", "--host", "--set", "race.course=2"};
    char *clientArgv[] = {
        "rage-racer", "--join", "127.0.0.1", "--set", "race.course=0",
        "--set", "race.class=0", "--set", "race.car=1"};
    MultiplayerCommand command;

    decided = Choice(2, 3, 9);
    MultiplayerServerBegin(&server, decided);
    Check(server.waiting && !server.started, "server has not started before a join");
    Check(!MultiplayerServerSession(&server, &wire),
          "an unjoined server has no live session");

    MultiplayerClientBegin(&client, Choice(0, 0, 1));
    Check(!client.joined, "client has not joined yet");
    Check(MultiplayerSessionSame(&client.playing, &client.wish),
          "before the join the client is still on its own choice");

    Check(MultiplayerSessionFormat(&server.decision, line, sizeof(line)),
          "server decision becomes the wire text");
    Check(MultiplayerSessionParse(line, &wire), "wire text is the server decision");
    Check(MultiplayerSessionSame(&wire, &decided),
          "parsed wire text keeps the server decision");

    Check(MultiplayerServerJoin(&server), "the join starts the server session");
    Check(server.started && !server.waiting, "server is live only after the join");
    Check(!MultiplayerServerJoin(&server), "a second join does not open another session");
    Check(MultiplayerServerSession(&server, &wire) &&
              MultiplayerSessionSame(&wire, &decided),
          "the live server session is the decision made before the join");

    MultiplayerClientApply(&client, wire);
    Check(client.joined, "client records the join");
    Check(MultiplayerSessionSame(&client.playing, &decided),
          "client plays the server decision");
    Check(!MultiplayerSessionSame(&client.playing, &client.wish),
          "the client's own choice does not replace the server session");

    Check(MultiplayerParseCommand(4, serverArgv, &command) &&
              command.role == MULTIPLAYER_ROLE_SERVER,
          "host mode is selected from the command line");
    Check(MultiplayerParseCommand(8, clientArgv, &command) &&
              command.role == MULTIPLAYER_ROLE_CLIENT &&
              strcmp(command.host, "127.0.0.1") == 0,
          "client mode takes the host IP from the command line");
    Check(!MultiplayerParseCommand(3, (char *[]){"rage-racer", "--join", "host-name"},
                                   &command),
          "a client argument must be an IPv4 address");
}

int main(void) {
    TestServerWaitsUntilJoin();
    if (s_failures != 0) {
        printf("%d multiplayer session checks failed\n", s_failures);
        return 1;
    }
    puts("multiplayer session waits for a join and follows the server");
    return 0;
}
