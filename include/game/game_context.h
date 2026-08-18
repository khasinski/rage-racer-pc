#ifndef GAME_GAME_CONTEXT_H
#define GAME_GAME_CONTEXT_H

#include "game/game_events.h"
#include "game/scene_manager.h"

typedef struct GameContext {
    SceneManager scenes;
    GameEventQueue events;
} GameContext;

void GameContextInit(GameContext *game, s32 *sceneId, s32 *sceneTimer,
                     SceneHandler *handlers, u32 handlerCount);

#endif
