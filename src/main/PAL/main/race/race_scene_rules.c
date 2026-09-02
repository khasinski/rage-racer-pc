#include "game/race_scene_internal.h"
#include "game/state.h"

enum {
    CAMERA_MAPPING_INDEX = 6,
    NEGCON_MAPPING_OFFSET = 8,
    WRONG_WAY_WARNING_FRAMES = 10,
    WRONG_WAY_COUNTER_RESET = 81,
    RACE_INTRO_END_FRAME = 90,
    RACE_START_FRAME = 211,
    RACE_END_MUSIC_FRAME = 10,
    RACE_END_FADE_FRAME = 20,
    RACE_END_BANNER_FRAME = 21,
    RACE_END_EXIT_FRAME = 101,
    RACE_RETRY_EXIT_FRAME = 126,
    RACE_QUIT_SCENE = 6,
    RACE_RESULT_SCENE = 15,
    RACE_RETRY_SCENE = 13,
};

void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]) {
    sectorEnds[0] = trackLength / 3;
    sectorEnds[1] = sectorEnds[0] * 2;
    sectorEnds[2] = trackLength;
}

u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]) {
    s32 mappingOffset =
        padType == PAD_TYPE_NEGCON ? NEGCON_MAPPING_OFFSET : 0;

    return buttonMapping[CAMERA_MAPPING_INDEX + mappingOffset];
}

s32 CanPauseRace(s16 phase) { return phase == 1 || phase == 2; }

s32 CanToggleRaceCamera(s16 phase) { return phase == 2 || phase == 3; }

s32 LastRacePauseOption(s16 grandPrixMode) {
    return grandPrixMode != 0 ? 1 : 2;
}

RacePauseAction DecideRacePauseAction(s16 phase, s16 grandPrixMode,
                                      s16 cursor) {
    if (cursor == LastRacePauseOption(grandPrixMode)) {
        if (grandPrixMode == 0 || phase < 2) {
            return RACE_PAUSE_QUIT;
        }
        return RACE_PAUSE_RETIRE;
    }
    if (cursor == 1 && grandPrixMode == 0) {
        return RACE_PAUSE_RESTART;
    }
    return RACE_PAUSE_RESUME;
}

RacePauseToggleResult DecideRacePauseToggle(s16 phase, s32 paused,
                                            s32 startPressed, s32 debounce,
                                            s16 grandPrixMode, s16 cursor) {
    RacePauseToggleResult result = {paused, RACE_PAUSE_RESUME, 0};

    if (!CanPauseRace(phase) || !startPressed || debounce > 0) {
        return result;
    }

    result.toggled = 1;
    result.paused = paused < 1;
    if (!result.paused) {
        result.action =
            DecideRacePauseAction(phase, grandPrixMode, cursor);
    }
    return result;
}

RaceEndPresentation ChooseRaceEndPresentation(s16 grandPrixMode,
                                              s32 retriesRemaining) {
    if (grandPrixMode == 0 ||
        (grandPrixMode == 1 && retriesRemaining == 0)) {
        return RACE_END_PRESENTATION_FINAL;
    }
    if (grandPrixMode == 1 && retriesRemaining > 0) {
        return RACE_END_PRESENTATION_RETRY;
    }
    return RACE_END_PRESENTATION_NONE;
}

RaceEndFrame BuildRaceEndFrame(s16 phase, s16 grandPrixMode,
                               s32 retriesRemaining, s32 fadeTimer) {
    RaceEndFrame frame = {
        RACE_END_PRESENTATION_NONE, 0, -1, 0, 0, 0};

    if (phase == 7) {
        frame.exitScene = RACE_QUIT_SCENE;
        return frame;
    }
    if (phase != 5) {
        return frame;
    }

    frame.advanceTimer = 1;
    frame.presentation =
        ChooseRaceEndPresentation(grandPrixMode, retriesRemaining);
    if (frame.presentation == RACE_END_PRESENTATION_FINAL) {
        if (fadeTimer >= RACE_END_BANNER_FRAME) {
            frame.drawPresentation = 1;
            frame.fade = (fadeTimer - RACE_END_FADE_FRAME) * 3;
        }
        frame.startMusic = fadeTimer == RACE_END_MUSIC_FRAME;
        if (fadeTimer >= RACE_END_EXIT_FRAME) {
            frame.exitScene = RACE_RESULT_SCENE;
        }
    } else if (frame.presentation == RACE_END_PRESENTATION_RETRY) {
        frame.drawPresentation = 1;
        frame.fade = fadeTimer * 2;
        if (fadeTimer >= RACE_RETRY_EXIT_FRAME) {
            frame.exitScene = RACE_RETRY_SCENE;
        }
    }
    return frame;
}

RacePauseCursorResult MoveRacePauseCursor(u16 pressed, s16 cursor,
                                          s16 grandPrixMode) {
    RacePauseCursorResult result = {cursor, 0};

    if ((pressed & PAD_UP) && result.cursor > 0) {
        result.cursor--;
        result.moveCount++;
    }
    if ((pressed & PAD_DOWN) &&
        result.cursor < LastRacePauseOption(grandPrixMode)) {
        result.cursor++;
        result.moveCount++;
    }
    return result;
}

s32 WrongWayWarningVisible(s16 timer) {
    return timer >= WRONG_WAY_WARNING_FRAMES;
}

WrongWayUpdate UpdateWrongWayState(s16 timer, s32 facingWrongWay, s16 phase,
                                   u32 sceneTimer) {
    WrongWayUpdate result = {0, 0, 0};

    if (!facingWrongWay || phase >= 4) {
        return result;
    }

    result.timer = timer + 1;
    result.drawWarning = WrongWayWarningVisible(result.timer);
    if (result.drawWarning) {
        if (result.timer >= WRONG_WAY_COUNTER_RESET) {
            result.timer = WRONG_WAY_WARNING_FRAMES;
        }
        result.playCue = (u8)sceneTimer == 0;
    }
    return result;
}

RaceStartUpdate UpdateRaceStartState(s16 phase, u32 sceneTimer) {
    RaceStartUpdate result = {phase, RACE_START_ACTION_NONE};

    if (phase == 0) {
        if (sceneTimer < RACE_INTRO_END_FRAME) {
            result.action = RACE_START_ACTION_UPDATE_INTRO_CAMERA;
        } else {
            result.phase = 1;
        }
    } else if (phase == 1 && sceneTimer >= RACE_START_FRAME) {
        result.phase = 2;
        result.action = RACE_START_ACTION_BEGIN;
    }
    return result;
}

RaceClockUpdate UpdateRaceClock(s32 remaining, s16 phase,
                                s16 grandPrixMode) {
    RaceClockUpdate result = {remaining, 0};

    if (phase >= 2 && grandPrixMode != 0) {
        result.remaining--;
    }
    result.expired = phase < 4 && result.remaining <= 0;
    return result;
}

RaceViewSelection SelectRaceView(s16 phase, s32 retiring,
                                 CameraViewMode selectedView) {
    RaceViewSelection result = {
        RACE_CAMERA_ACTION_NONE, selectedView, 0};

    if (phase == 5 && !retiring) {
        result.cameraAction = RACE_CAMERA_ACTION_FINISH;
        result.useFinishTextureSection = 1;
    } else if (phase > 0) {
        result.cameraAction = RACE_CAMERA_ACTION_FOLLOW_PLAYER;
        if (retiring) {
            result.cameraView = CAMERA_VIEW_CAR;
        }
    }
    return result;
}

s32 ReleaseFinishFollowupCue(s32 *queuedCue, s32 specialVoicesActive) {
    s32 cue;

    if (*queuedCue < 0 || specialVoicesActive) {
        return -1;
    }
    cue = *queuedCue;
    *queuedCue = -1;
    return cue;
}
