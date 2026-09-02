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

int main(void) {
    TestRaceGeometry();
    TestInputRules();
    TestPauseActions();

    if (s_failures != 0) {
        return 1;
    }
    puts("race scene initialization and input rules are stable");
    return 0;
}
