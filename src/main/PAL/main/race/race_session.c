#include "game/race_session.h"

void RaceSessionStep(RaceSession *session, u16 pressed,
                     RaceSessionCommands *commands) {
    RacePauseStep(&session->pause, pressed, &commands->pause);

    session->end.phase = session->pause.phase;
    session->end.fadeTimer = session->pause.fadeTimer;
    session->end.grandPrixMode = session->pause.grandPrixMode;
    session->end.retriesRemaining = session->pause.retriesRemaining;
    RaceEndStep(&session->end, &commands->end);

    session->pause.phase = session->end.phase;
    session->pause.fadeTimer = session->end.fadeTimer;
}
