#ifndef GAME_RACE_SCENE_INTERNAL_H
#define GAME_RACE_SCENE_INTERNAL_H

#include "common.h"
#include "game/camera_types.h"

typedef enum RacePauseAction {
    RACE_PAUSE_RESUME,
    RACE_PAUSE_QUIT,
    RACE_PAUSE_RESTART,
    RACE_PAUSE_RETIRE,
} RacePauseAction;

typedef enum RaceEndPresentation {
    RACE_END_PRESENTATION_NONE,
    RACE_END_PRESENTATION_FINAL,
    RACE_END_PRESENTATION_RETRY,
} RaceEndPresentation;

typedef struct RaceEndFrame {
    RaceEndPresentation presentation;
    s32 fade;
    s16 exitScene;
    u8 drawPresentation;
    u8 startMusic;
    u8 advanceTimer;
} RaceEndFrame;

typedef struct RacePauseCursorResult {
    s16 cursor;
    s16 moveCount;
} RacePauseCursorResult;

typedef struct RacePauseToggleResult {
    s32 paused;
    RacePauseAction action;
    u8 toggled;
} RacePauseToggleResult;

typedef struct WrongWayUpdate {
    s16 timer;
    u8 drawWarning;
    u8 playCue;
} WrongWayUpdate;

typedef enum RaceStartAction {
    RACE_START_ACTION_NONE,
    RACE_START_ACTION_UPDATE_INTRO_CAMERA,
    RACE_START_ACTION_BEGIN,
} RaceStartAction;

typedef struct RaceStartUpdate {
    s16 phase;
    RaceStartAction action;
} RaceStartUpdate;

typedef struct RaceClockUpdate {
    s32 remaining;
    u8 expired;
} RaceClockUpdate;

typedef enum RaceCameraAction {
    RACE_CAMERA_ACTION_NONE,
    RACE_CAMERA_ACTION_FOLLOW_PLAYER,
    RACE_CAMERA_ACTION_FINISH,
} RaceCameraAction;

typedef struct RaceViewSelection {
    RaceCameraAction cameraAction;
    CameraViewMode cameraView;
    u8 useFinishTextureSection;
} RaceViewSelection;

void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]);
u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]);
s32 CanPauseRace(s16 phase);
s32 CanToggleRaceCamera(s16 phase);
s32 LastRacePauseOption(s16 grandPrixMode);
RacePauseAction DecideRacePauseAction(s16 phase, s16 grandPrixMode,
                                      s16 cursor);
RacePauseToggleResult DecideRacePauseToggle(s16 phase, s32 paused,
                                            s32 startPressed, s32 debounce,
                                            s16 grandPrixMode, s16 cursor);
RaceEndPresentation ChooseRaceEndPresentation(s16 grandPrixMode,
                                              s32 retriesRemaining);
RaceEndFrame BuildRaceEndFrame(s16 phase, s16 grandPrixMode,
                               s32 retriesRemaining, s32 fadeTimer);
RacePauseCursorResult MoveRacePauseCursor(u16 pressed, s16 cursor,
                                          s16 grandPrixMode);
s32 WrongWayWarningVisible(s16 timer);
WrongWayUpdate UpdateWrongWayState(s16 timer, s32 facingWrongWay, s16 phase,
                                   u32 sceneTimer);
RaceStartUpdate UpdateRaceStartState(s16 phase, u32 sceneTimer);
RaceClockUpdate UpdateRaceClock(s32 remaining, s16 phase,
                                s16 grandPrixMode);
RaceViewSelection SelectRaceView(s16 phase, s32 retiring,
                                 CameraViewMode selectedView);
s32 ReleaseFinishFollowupCue(s32 *queuedCue, s32 specialVoicesActive);

#endif
