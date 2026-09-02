#include "game/race_scene_internal.h"
#include "game/state.h"

#include <stdio.h>

static s32 s_failures;

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestRaceGeometry(void) {
    s32 sectors[3];

    Check(RaceLapCount(0) == 3 && RaceLapCount(2) == 3,
          "standard courses run three laps");
    Check(RaceLapCount(3) == 6,
          "the long final course runs six laps");

    BuildRaceSectorEnds(100, sectors);
    Check(sectors[0] == 33 && sectors[1] == 66 && sectors[2] == 100,
          "sector boundaries preserve retail integer thirds");
}

static void TestInputRules(void) {
    u16 mappings[16] = {0};

    mappings[6] = PAD_R2;
    mappings[14] = PAD_L2;
    Check(RaceCameraButtonMask(PAD_TYPE_DIGITAL, mappings) == PAD_R2,
          "digital pad uses its camera mapping bank");
    Check(RaceCameraButtonMask(PAD_TYPE_NEGCON, mappings) == PAD_L2,
          "NeGcon uses its camera mapping bank");

    Check(!CanPauseRace(0) && CanPauseRace(1) && CanPauseRace(2) &&
              !CanPauseRace(3),
          "pause is limited to grid and live race phases");
    Check(!CanToggleRaceCamera(1) && CanToggleRaceCamera(2) &&
              CanToggleRaceCamera(3) && !CanToggleRaceCamera(4),
          "camera toggle is limited to active driving phases");
    Check(LastRacePauseOption(0) == 2 && LastRacePauseOption(1) == 1,
          "time attack exposes one more pause option than Grand Prix");
}

static void TestPauseActions(void) {
    Check(DecideRacePauseAction(2, 0, 0) == RACE_PAUSE_RESUME,
          "time attack resume row resumes");
    Check(DecideRacePauseAction(2, 0, 1) == RACE_PAUSE_RESTART,
          "time attack middle row restarts");
    Check(DecideRacePauseAction(1, 0, 2) == RACE_PAUSE_QUIT &&
              DecideRacePauseAction(2, 0, 2) == RACE_PAUSE_QUIT,
          "time attack final row quits before and during the race");

    Check(DecideRacePauseAction(1, 1, 0) == RACE_PAUSE_RESUME,
          "Grand Prix resume row resumes");
    Check(DecideRacePauseAction(1, 1, 1) == RACE_PAUSE_QUIT,
          "Grand Prix final row quits before the start");
    Check(DecideRacePauseAction(2, 1, 1) == RACE_PAUSE_RETIRE,
          "Grand Prix final row retires after the start");
}

static void TestPauseCursor(void) {
    RacePauseCursorResult result;

    result = MoveRacePauseCursor(PAD_UP, 0, 0);
    Check(result.cursor == 0 && result.moveCount == 0,
          "pause cursor does not move above the first row");
    result = MoveRacePauseCursor(PAD_DOWN, 2, 0);
    Check(result.cursor == 2 && result.moveCount == 0,
          "time attack cursor does not move below its final row");
    result = MoveRacePauseCursor(PAD_DOWN, 0, 1);
    Check(result.cursor == 1 && result.moveCount == 1,
          "Grand Prix cursor reaches its second and final row");
    result = MoveRacePauseCursor(PAD_UP | PAD_DOWN, 2, 0);
    Check(result.cursor == 2 && result.moveCount == 2,
          "simultaneous directions retain sequential retail input");
}

static void TestRaceEndPresentation(void) {
    Check(ChooseRaceEndPresentation(0, 0) == RACE_END_PRESENTATION_FINAL &&
              ChooseRaceEndPresentation(0, 3) == RACE_END_PRESENTATION_FINAL,
          "time attack always uses the final result presentation");
    Check(ChooseRaceEndPresentation(1, 0) == RACE_END_PRESENTATION_FINAL,
          "Grand Prix without retries uses the final result presentation");
    Check(ChooseRaceEndPresentation(1, 2) == RACE_END_PRESENTATION_RETRY,
          "Grand Prix with retries offers another attempt");
    Check(ChooseRaceEndPresentation(2, 0) == RACE_END_PRESENTATION_NONE &&
              ChooseRaceEndPresentation(-1, 2) == RACE_END_PRESENTATION_NONE,
          "non-race modes preserve the retail no-presentation path");
}

static void TestWrongWayState(void) {
    WrongWayUpdate update;

    update = UpdateWrongWayState(8, 1, 2, 100);
    Check(update.timer == 9 && !update.drawWarning && !update.playCue,
          "wrong-way warning waits for ten frames");
    update = UpdateWrongWayState(9, 1, 2, 100);
    Check(update.timer == 10 && update.drawWarning && !update.playCue,
          "wrong-way warning appears on its tenth frame");
    update = UpdateWrongWayState(80, 1, 2, 100);
    Check(update.timer == 10 && update.drawWarning,
          "wrong-way counter returns to its visible baseline");

    update = UpdateWrongWayState(20, 1, 2, 256);
    Check(update.playCue,
          "wrong-way cue follows the low byte of the scene timer");
    update = UpdateWrongWayState(20, 1, 2, 255);
    Check(!update.playCue,
          "wrong-way cue remains silent between timer boundaries");

    update = UpdateWrongWayState(20, 0, 2, 256);
    Check(update.timer == 0 && !update.drawWarning && !update.playCue,
          "correct direction clears the warning");
    update = UpdateWrongWayState(20, 1, 4, 256);
    Check(update.timer == 0 && !update.drawWarning && !update.playCue,
          "finish phase clears the warning");
    Check(!WrongWayWarningVisible(9) && WrongWayWarningVisible(10),
          "paused rendering uses the same visibility threshold");
}

static void TestRaceStartState(void) {
    RaceStartUpdate update;

    update = UpdateRaceStartState(0, 89);
    Check(update.phase == 0 &&
              update.action == RACE_START_ACTION_UPDATE_INTRO_CAMERA,
          "intro camera runs through its final frame");
    update = UpdateRaceStartState(0, 90);
    Check(update.phase == 1 && update.action == RACE_START_ACTION_NONE,
          "grid phase begins at the end of the intro");

    update = UpdateRaceStartState(1, 210);
    Check(update.phase == 1 && update.action == RACE_START_ACTION_NONE,
          "grid remains armed before the standing start");
    update = UpdateRaceStartState(1, 211);
    Check(update.phase == 2 && update.action == RACE_START_ACTION_BEGIN,
          "standing start fires on its authored frame");

    update = UpdateRaceStartState(2, 300);
    Check(update.phase == 2 && update.action == RACE_START_ACTION_NONE,
          "live race state is left unchanged");
}

int main(void) {
    TestRaceGeometry();
    TestInputRules();
    TestPauseActions();
    TestPauseCursor();
    TestRaceEndPresentation();
    TestWrongWayState();
    TestRaceStartState();

    if (s_failures != 0) {
        return 1;
    }
    puts("race scene initialization and input rules are stable");
    return 0;
}
