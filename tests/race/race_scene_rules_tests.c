#include "game/race_scene_internal.h"
#include "game/state.h"

#include <limits.h>
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

    BuildRaceSectorEnds(100, sectors);
    Check(sectors[0] == 33 && sectors[1] == 66 && sectors[2] == 100,
          "sector boundaries preserve retail integer thirds");
    BuildRaceSectorEnds(100, NULL);
}

static void TestInputRules(void) {
    u16 mappings[16] = {0};

    mappings[6] = PAD_R2;
    mappings[14] = PAD_L2;
    Check(RaceCameraButtonMask(PAD_TYPE_DIGITAL, mappings) == PAD_R2,
          "digital pad uses its camera mapping bank");
    Check(RaceCameraButtonMask(PAD_TYPE_NEGCON, mappings) == PAD_L2,
          "NeGcon uses its camera mapping bank");
    Check(RaceCameraButtonMask(PAD_TYPE_DIGITAL, NULL) == 0,
          "missing camera mapping has no button");

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

static void TestPauseToggle(void) {
    RacePauseToggleResult toggle;

    toggle = DecideRacePauseToggle(0, 0, 1, 0, 1, 0);
    Check(!toggle.toggled && !toggle.paused,
          "intro phase rejects pause toggles");
    toggle = DecideRacePauseToggle(2, 0, 0, 0, 1, 0);
    Check(!toggle.toggled, "pause toggle requires the start button");
    toggle = DecideRacePauseToggle(2, 0, 1, 1, 1, 0);
    Check(!toggle.toggled, "pause debounce blocks repeated toggles");

    toggle = DecideRacePauseToggle(2, 0, 1, 0, 1, 0);
    Check(toggle.toggled && toggle.paused,
          "live race enters pause on an accepted toggle");
    toggle = DecideRacePauseToggle(2, 1, 1, 0, 1, 1);
    Check(toggle.toggled && !toggle.paused &&
              toggle.action == RACE_PAUSE_RETIRE,
          "leaving Grand Prix pause carries the selected retire action");
    toggle = DecideRacePauseToggle(1, 1, 1, 0, 1, 1);
    Check(toggle.action == RACE_PAUSE_QUIT,
          "leaving pre-start pause carries the quit action");
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
    result = MoveRacePauseCursor(0, SHRT_MIN, 0);
    Check(result.cursor == 0, "negative pause cursor resets");
    result = MoveRacePauseCursor(0, SHRT_MAX, 1);
    Check(result.cursor == 1, "past-end pause cursor is clamped");
}

static void TestRaceEndPresentation(void) {
    Check(ChooseRaceEndPresentation(0, 0) == RACE_END_PRESENTATION_FINAL &&
              ChooseRaceEndPresentation(0, 3) == RACE_END_PRESENTATION_FINAL,
          "time attack always uses the final result presentation");
    Check(ChooseRaceEndPresentation(1, 0) == RACE_END_PRESENTATION_FINAL,
          "Grand Prix without retries uses the final result presentation");
    Check(ChooseRaceEndPresentation(1, -1) == RACE_END_PRESENTATION_FINAL,
          "corrupt negative retry count cannot stall the final presentation");
    Check(ChooseRaceEndPresentation(1, 2) == RACE_END_PRESENTATION_RETRY,
          "Grand Prix with retries offers another attempt");
    Check(ChooseRaceEndPresentation(2, 0) == RACE_END_PRESENTATION_NONE &&
              ChooseRaceEndPresentation(-1, 2) == RACE_END_PRESENTATION_NONE,
          "non-race modes preserve the retail no-presentation path");
}

static void TestRaceEndFrames(void) {
    RaceEndFrame frame;

    frame = BuildRaceEndFrame(2, 1, 1, 50);
    Check(!frame.advanceTimer && frame.exitScene == -1,
          "live race has no end presentation frame");
    frame = BuildRaceEndFrame(7, 1, 1, 50);
    Check(!frame.advanceTimer && frame.exitScene == 6,
          "quit phase exits immediately without advancing the fade");

    frame = BuildRaceEndFrame(5, 0, 0, 10);
    Check(frame.advanceTimer && frame.startMusic &&
              !frame.drawPresentation && frame.exitScene == -1,
          "final presentation starts music on frame ten");
    frame = BuildRaceEndFrame(5, 0, 0, 20);
    Check(!frame.drawPresentation,
          "final presentation waits through its fade baseline");
    frame = BuildRaceEndFrame(5, 0, 0, 21);
    Check(frame.drawPresentation && frame.fade == 3,
          "final banner begins one frame after the fade baseline");
    frame = BuildRaceEndFrame(5, 0, 0, 101);
    Check(frame.exitScene == 15,
          "final presentation exits to the result scene");

    frame = BuildRaceEndFrame(5, 1, 2, 0);
    Check(frame.presentation == RACE_END_PRESENTATION_RETRY &&
              frame.drawPresentation && frame.fade == 0,
          "retry presentation draws from its first frame");
    frame = BuildRaceEndFrame(5, 1, 2, 126);
    Check(frame.fade == 252 && frame.exitScene == 13,
          "retry presentation exits on its authored boundary");

    frame = BuildRaceEndFrame(5, 2, 0, 30);
    Check(frame.advanceTimer && !frame.drawPresentation &&
              frame.exitScene == -1,
          "unsupported race modes still advance the retail fade timer");
    frame = BuildRaceEndFrame(5, 0, 0, INT_MAX);
    Check(frame.fade == INT_MAX && frame.exitScene == 15,
          "final fade saturates for corrupt timers");
    frame = BuildRaceEndFrame(5, 1, 2, INT_MIN);
    Check(frame.fade == 0,
          "retry fade does not expose negative brightness");
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
    update = UpdateWrongWayState(SHRT_MAX, 1, 2, 100);
    Check(update.timer == 10 && update.drawWarning,
          "corrupt wrong-way counter returns to its baseline");

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

static void TestRaceClock(void) {
    RaceClockUpdate update;

    update = UpdateRaceClock(2, 2, 1);
    Check(update.remaining == 1 && !update.expired,
          "Grand Prix clock advances during the live race");
    update = UpdateRaceClock(1, 2, 1);
    Check(update.remaining == 0 && update.expired,
          "Grand Prix expires on the frame its clock reaches zero");

    update = UpdateRaceClock(10, 1, 1);
    Check(update.remaining == 10 && !update.expired,
          "grid phase does not consume race time");
    update = UpdateRaceClock(0, 1, 1);
    Check(update.remaining == 0 && update.expired,
          "an exhausted grid clock retains the retail timeout path");
    update = UpdateRaceClock(0, 4, 1);
    Check(update.remaining == -1 && !update.expired,
          "finish presentation cannot trigger another timeout");

    update = UpdateRaceClock(5, 2, 0);
    Check(update.remaining == 5 && !update.expired,
          "time attack does not consume its compatibility clock");
    update = UpdateRaceClock(0, 2, 0);
    Check(update.remaining == 0 && update.expired,
          "time attack preserves the retail zero-clock fallback");
    update = UpdateRaceClock(INT_MIN, 2, 1);
    Check(update.remaining == INT_MIN && update.expired,
          "race clock saturates at its lower bound");
}

static void TestRaceViewSelection(void) {
    RaceViewSelection view;

    view = SelectRaceView(0, 0, CAMERA_VIEW_CHASE);
    Check(view.cameraAction == RACE_CAMERA_ACTION_NONE &&
              !view.useFinishTextureSection,
          "intro phase owns its camera and uses the player texture section");

    view = SelectRaceView(2, 0, CAMERA_VIEW_CHASE);
    Check(view.cameraAction == RACE_CAMERA_ACTION_FOLLOW_PLAYER &&
              view.cameraView == CAMERA_VIEW_CHASE &&
              !view.useFinishTextureSection,
          "live race follows the selected player camera");
    view = SelectRaceView(5, 0, CAMERA_VIEW_CHASE);
    Check(view.cameraAction == RACE_CAMERA_ACTION_FINISH &&
              view.useFinishTextureSection,
          "normal finish advances its autonomous camera and texture section");

    view = SelectRaceView(5, 1, CAMERA_VIEW_CHASE);
    Check(view.cameraAction == RACE_CAMERA_ACTION_FOLLOW_PLAYER &&
              view.cameraView == CAMERA_VIEW_CAR &&
              !view.useFinishTextureSection,
          "retirement keeps the car camera and player texture section");
    view = SelectRaceView(7, 0, CAMERA_VIEW_TRACK);
    Check(view.cameraAction == RACE_CAMERA_ACTION_FOLLOW_PLAYER &&
              view.cameraView == CAMERA_VIEW_TRACK &&
              !view.useFinishTextureSection,
          "quit transition preserves the selected player camera");
    view = SelectRaceView(2, 0, CAMERA_VIEW_INVALID);
    Check(view.cameraView == CAMERA_VIEW_CAR,
          "invalid camera mode falls back to the car view");
}

static void TestFinishFollowupQueue(void) {
    s32 queuedCue = -1;

    Check(ReleaseFinishFollowupCue(&queuedCue, 0) == -1 && queuedCue == -1,
          "empty finish cue queue stays empty");

    queuedCue = 0x2B;
    Check(ReleaseFinishFollowupCue(&queuedCue, 1) == -1 &&
              queuedCue == 0x2B,
          "busy special voices keep the finish cue queued");
    Check(ReleaseFinishFollowupCue(&queuedCue, 0) == 0x2B &&
              queuedCue == -1,
          "idle special voices release and clear the finish cue");
    Check(ReleaseFinishFollowupCue(NULL, 0) == -1,
          "missing finish cue queue is empty");
}

static void TestRaceTimers(void) {
    Check(NormalizeRaceSceneTimer(-1) == 0 &&
              NormalizeRaceSceneTimer(24) == 24,
          "race scene timer rejects corrupt negative values");
    Check(NextRaceSceneTimer(-1) == 1 &&
              NextRaceSceneTimer(24) == 25 &&
              NextRaceSceneTimer(INT_MAX) == INT_MAX,
          "race scene timer advances without signed overflow");
    Check(NextRaceAnimationTimer(-1) == 0 &&
              NextRaceAnimationTimer(24) == 25 &&
              NextRaceAnimationTimer(INT_MAX) == 0,
          "cyclic race animation timer restarts safely");
    Check(NextRaceFadeTimer(-1) == 0 &&
              NextRaceFadeTimer(24) == 25 &&
              NextRaceFadeTimer(SHRT_MAX) == SHRT_MAX,
          "race end fade timer stays within its storage type");
}

int main(void) {
    TestRaceGeometry();
    TestInputRules();
    TestPauseActions();
    TestPauseToggle();
    TestPauseCursor();
    TestRaceEndPresentation();
    TestRaceEndFrames();
    TestWrongWayState();
    TestRaceStartState();
    TestRaceClock();
    TestRaceViewSelection();
    TestFinishFollowupQueue();
    TestRaceTimers();

    if (s_failures != 0) {
        return 1;
    }
    puts("race scene initialization and input rules are stable");
    return 0;
}
