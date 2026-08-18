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
/* Temporary bridge for legacy parameterless scene handlers. New systems
 * should receive GameContext explicitly; recovered handlers can migrate one
 * at a time through these two checked entry points. */
void GameContextSetActive(GameContext *game);
s32 GameSceneSet(SceneId next);
s32 GameSceneEnter(SceneId next, s32 initialTimer);

#endif
