#ifndef GAME_SCENERY_H
#define GAME_SCENERY_H
#include "game/vector.h"

typedef struct SceneryPlacement {
    LVec position;
    s32 yaw;
} SceneryPlacement;

typedef struct StaticSceneryState {
    SceneryPlacement standard;
    SceneryPlacement highClass;
} StaticSceneryState;

/* Copies built-in placement without borrowing mutable game state. */
StaticSceneryState RetailLandmarks(void);
#endif
