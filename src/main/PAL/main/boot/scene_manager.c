#include "game/scene_manager.h"

static void SceneManagerEmitTransition(SceneManager *manager, s32 previous,
                                       s32 current) {
    GameEvent event;

    if (manager->events == 0 || previous == current) return;
    event.type = GAME_EVENT_SCENE_CHANGED;
    event.data.sceneChanged.previous = previous;
    event.data.sceneChanged.current = current;
    GameEventQueuePush(manager->events, &event);
}

void SceneManagerInit(SceneManager *manager, s32 *sceneId, s32 *sceneTimer,
                      SceneHandler *handlers, u32 handlerCount,
                      GameEventQueue *events) {
    manager->sceneId = sceneId;
    manager->sceneTimer = sceneTimer;
    manager->handlers = handlers;
    manager->handlerCount = handlerCount;
    manager->observedSceneId = *sceneId;
    manager->events = events;
}

s32 SceneManagerIsValid(const SceneManager *manager, s32 sceneId) {
    return sceneId > SCENE_NONE && (u32)sceneId < manager->handlerCount &&
           manager->handlers[sceneId] != 0;
}

s32 SceneManagerTransition(SceneManager *manager, SceneId next) {
    s32 previous;

    if (!SceneManagerIsValid(manager, next)) return 0;
    previous = *manager->sceneId;
    *manager->sceneId = next;
    *manager->sceneTimer = 0;
    manager->observedSceneId = next;
    SceneManagerEmitTransition(manager, previous, next);
    return 1;
}

s32 SceneManagerObserveTransition(SceneManager *manager) {
    s32 current = *manager->sceneId;

    if (current == manager->observedSceneId) return 0;
    SceneManagerEmitTransition(manager, manager->observedSceneId, current);
    manager->observedSceneId = current;
    return 1;
}

s32 SceneManagerDispatch(SceneManager *manager) {
    s32 current;

    SceneManagerObserveTransition(manager);
    current = *manager->sceneId;
    if (!SceneManagerIsValid(manager, current)) return 0;
    manager->handlers[current]();
    SceneManagerObserveTransition(manager);
    return 1;
}
