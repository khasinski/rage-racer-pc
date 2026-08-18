#include "game/race_end.h"

void RaceEndStep(RaceEndState *state, RaceEndCommands *commands) {
    *commands = (RaceEndCommands){-1, -1, -1, -1, 0, -1, 0};

    if (state->phase == 7) {
        commands->exitScene = 6;
        return;
    }
    if (state->phase != 5) return;

    if ((state->grandPrixMode == 1 && state->retriesRemaining == 0) ||
        state->grandPrixMode == 0) {
        if (state->fadeTimer >= 0x15) {
            commands->drawEndBannerIntensity = (state->fadeTimer - 0x14) * 3;
            commands->drawFadeIntensity = commands->drawEndBannerIntensity;
        }
        if (state->fadeTimer == 0xA) {
            commands->requestCdTrack = 0xF;
            commands->startCd = 1;
        }
        if (state->fadeTimer >= 0x65) commands->exitScene = 0xF;
    } else if (state->grandPrixMode == 1 && state->retriesRemaining > 0) {
        commands->drawLostCaptionIntensity = state->fadeTimer * 2;
        commands->drawFadeIntensity = commands->drawLostCaptionIntensity;
        if (state->fadeTimer >= 0x7E) commands->exitScene = 0xD;
    }
    commands->disableMirror = 1;
    state->fadeTimer++;
}
