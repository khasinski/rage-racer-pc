#ifndef GAME_SCENE_STATE_H
#define GAME_SCENE_STATE_H

#include "common.h"

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
    s16 pauseDelay;
    s16 optionCursor;
} RaceScene;

#endif
