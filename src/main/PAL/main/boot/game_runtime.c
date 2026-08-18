#include "game/game_runtime.h"

static void GameRuntimeCall(GameRuntimePhase phase, void *user) {
    if (phase != 0) phase(user);
}

void GameRuntimeInit(GameRuntime *runtime, s32 *sceneId, s32 *sceneTimer,
                     SceneHandler *handlers, u32 handlerCount,
                     const GameRuntimeServices *services) {
    GameContextInit(&runtime->game, sceneId, sceneTimer, handlers, handlerCount);
    GameContextSetActive(&runtime->game);
    runtime->services = *services;
}

GameRuntimeStepResult GameRuntimeStep(GameRuntime *runtime) {
    GameRuntimeServices *services = &runtime->services;

    GameRuntimeCall(services->prepareFrame, services->user);
    GameRuntimeCall(services->serviceSystems, services->user);
    GameRuntimeCall(services->beforeScene, services->user);
    if (!SceneManagerDispatch(&runtime->game.scenes)) {
        return GAME_RUNTIME_INVALID_SCENE;
    }
    GameRuntimeCall(services->afterScene, services->user);
    GameRuntimeCall(services->presentFrame, services->user);
    if (services->shouldExit != 0 && services->shouldExit(services->user)) {
        return GAME_RUNTIME_EXIT;
    }
    return GAME_RUNTIME_CONTINUE;
}
