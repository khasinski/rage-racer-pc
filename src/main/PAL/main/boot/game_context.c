#include "game/game_context.h"

static GameContext *s_activeGame;

void GameContextInit(GameContext *game, s32 *sceneId, s32 *sceneTimer,
                     SceneHandler *handlers, u32 handlerCount) {
    GameEventQueueInit(&game->events);
    SceneManagerInit(&game->scenes, sceneId, sceneTimer, handlers,
                     handlerCount, &game->events);
}

void GameContextSetActive(GameContext *game) { s_activeGame = game; }

s32 GameSceneSet(SceneId next) {
    if (s_activeGame == 0) return 0;
    return SceneManagerSet(&s_activeGame->scenes, next);
}

s32 GameSceneEnter(SceneId next, s32 initialTimer) {
    if (s_activeGame == 0) return 0;
    return SceneManagerEnter(&s_activeGame->scenes, next, initialTimer);
}
