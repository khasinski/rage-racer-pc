#ifndef GAME_RACE_SCENE_INTERNAL_H
#define GAME_RACE_SCENE_INTERNAL_H

#include "common.h"

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

typedef struct RacePauseCursorResult {
    s16 cursor;
    s16 moveCount;
} RacePauseCursorResult;

typedef struct WrongWayUpdate {
    s16 timer;
    u8 drawWarning;
    u8 playCue;
} WrongWayUpdate;

s32 RaceLapCount(s32 courseIndex);
void BuildRaceSectorEnds(s32 trackLength, s32 sectorEnds[3]);
u16 RaceCameraButtonMask(u8 padType, const u16 buttonMapping[16]);
s32 CanPauseRace(s16 phase);
s32 CanToggleRaceCamera(s16 phase);
s32 LastRacePauseOption(s16 grandPrixMode);
RacePauseAction DecideRacePauseAction(s16 phase, s16 grandPrixMode,
                                      s16 cursor);
RaceEndPresentation ChooseRaceEndPresentation(s16 grandPrixMode,
                                              s32 retriesRemaining);
RacePauseCursorResult MoveRacePauseCursor(u16 pressed, s16 cursor,
                                          s16 grandPrixMode);
s32 WrongWayWarningVisible(s16 timer);
WrongWayUpdate UpdateWrongWayState(s16 timer, s32 facingWrongWay, s16 phase,
                                   u32 sceneTimer);

#endif
