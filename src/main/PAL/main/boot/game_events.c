#include "game/game_events.h"

void GameEventQueueInit(GameEventQueue *queue) {
    queue->readIndex = 0;
    queue->count = 0;
    queue->dropped = 0;
}

s32 GameEventQueuePush(GameEventQueue *queue, const GameEvent *event) {
    u32 writeIndex;

    if (queue->count == GAME_EVENT_CAPACITY) {
        queue->dropped++;
        return 0;
    }
    writeIndex = (queue->readIndex + queue->count) % GAME_EVENT_CAPACITY;
    queue->events[writeIndex] = *event;
    queue->count++;
    return 1;
}

s32 GameEventQueuePop(GameEventQueue *queue, GameEvent *event) {
    if (queue->count == 0) return 0;
    *event = queue->events[queue->readIndex];
    queue->readIndex = (queue->readIndex + 1) % GAME_EVENT_CAPACITY;
    queue->count--;
    return 1;
}
