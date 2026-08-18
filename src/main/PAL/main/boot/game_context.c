#include "game/game_context.h"

void GameContextInit(GameContext *game, s32 *sceneId, s32 *sceneTimer,
                     SceneHandler *handlers, u32 handlerCount) {
    GameEventQueueInit(&game->events);
    SceneManagerInit(&game->scenes, sceneId, sceneTimer, handlers,
                     handlerCount, &game->events);
}
