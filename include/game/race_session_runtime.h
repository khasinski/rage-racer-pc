#ifndef GAME_RACE_SESSION_RUNTIME_H
#define GAME_RACE_SESSION_RUNTIME_H

#include "game/race_session.h"

void CaptureRaceSession(RaceSession *session, s32 retireCameraActive);
void ApplyRaceSession(
    const RaceSession *session,
    const RaceSessionCommands *commands,
    s32 *retireCameraActive);

#endif
