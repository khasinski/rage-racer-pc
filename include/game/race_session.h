#ifndef GAME_RACE_SESSION_H
#define GAME_RACE_SESSION_H

#include "game/race_end.h"
#include "game/race_pause.h"

typedef struct RaceSession {
    RacePauseState pause;
    RaceEndState end;
} RaceSession;

typedef struct RaceSessionCommands {
    RacePauseCommands pause;
    RaceEndCommands end;
} RaceSessionCommands;

void RaceSessionStep(RaceSession *session, u16 pressed,
                     RaceSessionCommands *commands);

#endif
