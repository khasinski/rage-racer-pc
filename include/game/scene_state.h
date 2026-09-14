#ifndef GAME_SCENE_STATE_H
#define GAME_SCENE_STATE_H

#include "common.h"
#include "game/race_time_types.h"

typedef enum BootLogoState {
    BOOT_LOGO_STATE_INVALID = -1,
    BOOT_LOGO_STATE_FADE_IN,
    BOOT_LOGO_STATE_HOLD,
    BOOT_LOGO_STATE_FADE_OUT,
    BOOT_LOGO_STATE_START_FMV
} BootLogoState;

typedef struct BootLogo {
    BootLogoState state;
    s32 timer;
    s32 holdTimer;
} BootLogo;

enum { BOOT_LOGO_INITIAL_HOLD_FRAMES = 150 };

typedef enum AttractDemoStep {
    ATTRACT_DEMO_STEP_INVALID = -1,
    ATTRACT_DEMO_STEP_LOAD,
    ATTRACT_DEMO_STEP_RACE
} AttractDemoStep;

typedef struct AttractDemo {
    AttractDemoStep step;
} AttractDemo;

typedef struct LostRace {
    s32 choice;
} LostRace;

typedef struct RaceScene {
    RaceTiming timing;
    s32 timeRemaining;
    s16 fadeTimer;
    s16 pauseDelay;
    s16 optionCursor;
} RaceScene;

#endif
