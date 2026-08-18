#ifndef GAME_SCENE_MANAGER_H
#define GAME_SCENE_MANAGER_H

#include "common.h"
#include "game/game_events.h"

typedef enum SceneId {
    SCENE_NONE = 0,
    SCENE_BOOT_LOGO = 1,
    SCENE_FRONTEND_ENTER = 2,
    SCENE_TITLE_ENTER = 3,
    SCENE_FRONTEND = 4,
    SCENE_FMV = 5,
    SCENE_MENU_ENTER = 6,
    SCENE_CLASS_FMV_RETURN = 7,
    SCENE_MENU = 8,
    SCENE_ROUND_ENTER = 9,
    SCENE_ROUND = 10,
    SCENE_RACE_ENTER = 11,
    SCENE_RACE = 12,
    SCENE_LOST_RACE_ENTER = 13,
    SCENE_LOST_RACE = 14,
    SCENE_RACE_END_ENTER = 15,
    SCENE_RACE_END = 16,
    SCENE_REPLAY = 17,
    SCENE_PRIZE_ENTER = 18,
    SCENE_PRIZE = 19,
    SCENE_RECORD_ENTER = 20,
    SCENE_RECORD = 21,
    SCENE_ATTRACT_ENTER = 22,
    SCENE_OPTIONS = 23,
    SCENE_MEMORY_CARD_ENTER = 24,
    SCENE_MEMORY_CARD_LOAD_ENTER = 25,
    SCENE_MEMORY_CARD = 26,
    SCENE_BGM_SELECT_ENTER = 27,
    SCENE_BGM_SELECT = 28,
    SCENE_ATTRACT_DEMO_ENTER = 29,
    SCENE_ATTRACT_DEMO = 30,
    SCENE_PROLOGUE_ENTER = 31,
    SCENE_PROLOGUE = 32,
    SCENE_ENDING_FMV_RETURN = 33,
    SCENE_ENDING_STILL = 34,
    SCENE_COUNT = 40
} SceneId;

typedef void (*SceneHandler)(void);

typedef struct SceneManager {
    s32 *sceneId;
    s32 *sceneTimer;
    SceneHandler *handlers;
    u32 handlerCount;
    s32 observedSceneId;
    GameEventQueue *events;
} SceneManager;

void SceneManagerInit(SceneManager *manager, s32 *sceneId, s32 *sceneTimer,
                      SceneHandler *handlers, u32 handlerCount,
                      GameEventQueue *events);
s32 SceneManagerSet(SceneManager *manager, SceneId next);
s32 SceneManagerEnter(SceneManager *manager, SceneId next, s32 initialTimer);
s32 SceneManagerObserveTransition(SceneManager *manager);
s32 SceneManagerDispatch(SceneManager *manager);
s32 SceneManagerIsValid(const SceneManager *manager, s32 sceneId);

#endif
