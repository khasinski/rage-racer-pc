#ifndef GAME_GAME_RUNTIME_H
#define GAME_GAME_RUNTIME_H

#include "game/game_context.h"

typedef void (*GameRuntimePhase)(void *user);
typedef s32 (*GameRuntimeExitCheck)(void *user);

typedef struct GameRuntimeServices {
    void *user;
    GameRuntimePhase prepareFrame;
    GameRuntimePhase serviceSystems;
    GameRuntimePhase beforeScene;
    GameRuntimePhase captureInput;
    GameRuntimePhase afterScene;
    GameRuntimePhase presentFrame;
    GameRuntimeExitCheck shouldExit;
} GameRuntimeServices;

typedef struct GameRuntime {
    GameContext game;
    GameRuntimeServices services;
} GameRuntime;

typedef enum GameRuntimeStepResult {
    GAME_RUNTIME_INVALID_SCENE = -1,
    GAME_RUNTIME_CONTINUE = 0,
    GAME_RUNTIME_EXIT = 1
} GameRuntimeStepResult;

void GameRuntimeInit(GameRuntime *runtime, s32 *sceneId, s32 *sceneTimer,
                     SceneHandler *handlers, u32 handlerCount,
                     const GameRuntimeServices *services);
GameRuntimeStepResult GameRuntimeStep(GameRuntime *runtime);

#endif
