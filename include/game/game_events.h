#ifndef GAME_GAME_EVENTS_H
#define GAME_GAME_EVENTS_H

#include "common.h"

typedef enum GameEventType {
    GAME_EVENT_NONE = 0,
    GAME_EVENT_SCENE_CHANGED
} GameEventType;

typedef struct GameSceneChangedEvent {
    s32 previous;
    s32 current;
} GameSceneChangedEvent;

typedef struct GameEvent {
    GameEventType type;
    union {
        GameSceneChangedEvent sceneChanged;
    } data;
} GameEvent;

enum { GAME_EVENT_CAPACITY = 32 };

typedef struct GameEventQueue {
    GameEvent events[GAME_EVENT_CAPACITY];
    u32 readIndex;
    u32 count;
    u32 dropped;
} GameEventQueue;

void GameEventQueueInit(GameEventQueue *queue);
s32 GameEventQueuePush(GameEventQueue *queue, const GameEvent *event);
s32 GameEventQueuePop(GameEventQueue *queue, GameEvent *event);

#endif
