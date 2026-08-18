#include "game/race_pause.h"
#include "game/state.h"

static void RacePauseQueueSound(RacePauseCommands *commands, s32 cue) {
    if (commands->soundCueCount < 2) {
        commands->soundCues[commands->soundCueCount++] = cue;
    }
}

void RacePauseStep(RacePauseState *state, u16 pressed,
                   RacePauseCommands *commands) {
    u32 pausePhase;

    *commands = (RacePauseCommands){0};
    commands->exitRaceScene = -1;
    if (state->debounce > 0) state->debounce--;

    pausePhase = (u16)state->phase - 1;
    if (pausePhase < 2 && (pressed & PAD_START) && state->debounce <= 0) {
        state->debounce = 5;
        state->paused = state->paused < 1;
        if (state->paused) {
            commands->pauseCd = 1;
            commands->setEffectVoices = 1;
            commands->effectVoicesEnabled = 0;
            state->optionCursor = 0;
            RacePauseQueueSound(commands, 2);
        } else if (state->optionCursor == 2 - state->grandPrixMode) {
            state->fadeTimer = 0;
            if (!state->grandPrixMode || state->phase < 2) {
                state->phase = 7;
                commands->updateTimeAttackRecord = !state->grandPrixMode;
            } else {
                state->phase = 5;
                state->retireCameraActive = 1;
                if (state->retriesRemaining != 0)
                    RacePauseQueueSound(commands, 0x3D);
            }
            commands->seedFinishCamera = !state->retireCameraActive;
            commands->startCdFadeFrames = 8;
        } else if (state->optionCursor == 1 && !state->grandPrixMode) {
            commands->exitRaceScene = 0xB;
            state->phase = 8;
        } else {
            state->debounce = 0x1E;
            commands->setEffectVoices = 1;
            commands->effectVoicesEnabled = 1;
            commands->resumeCd = state->phase >= 2;
        }
    }

    if (!state->paused) return;

    commands->setPauseReverb = 1;
    if ((pressed & PAD_UP) && state->optionCursor > 0) {
        state->optionCursor--;
        RacePauseQueueSound(commands, 1);
    }
    if ((pressed & PAD_DOWN) &&
        state->optionCursor < 2 - state->grandPrixMode) {
        state->optionCursor++;
        RacePauseQueueSound(commands, 1);
    }
    state->sceneTimer--;
}
