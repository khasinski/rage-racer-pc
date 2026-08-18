#include "game/game_context.h"

static int failures;
static int calls;

#define EXPECT_EQ(expected, actual) do {                                      \
    if ((int)(expected) != (int)(actual)) failures++;                         \
} while (0)

static void CountCall(void) { calls++; }

static void test_transition_and_dispatch(void) {
    SceneHandler handlers[SCENE_COUNT] = {0};
    GameContext game;
    GameEvent event;
    s32 scene = SCENE_BOOT_LOGO;
    s32 timer = 99;

    handlers[SCENE_BOOT_LOGO] = CountCall;
    handlers[SCENE_RACE] = CountCall;
    GameContextInit(&game, &scene, &timer, handlers, SCENE_COUNT);

    EXPECT_EQ(1, SceneManagerDispatch(&game.scenes));
    EXPECT_EQ(1, calls);
    EXPECT_EQ(1, SceneManagerEnter(&game.scenes, SCENE_RACE, 0));
    EXPECT_EQ(SCENE_RACE, scene);
    EXPECT_EQ(0, timer);
    EXPECT_EQ(1, GameEventQueuePop(&game.events, &event));
    EXPECT_EQ(GAME_EVENT_SCENE_CHANGED, event.type);
    EXPECT_EQ(SCENE_BOOT_LOGO, event.data.sceneChanged.previous);
    EXPECT_EQ(SCENE_RACE, event.data.sceneChanged.current);
    EXPECT_EQ(1, SceneManagerDispatch(&game.scenes));
    EXPECT_EQ(2, calls);
}

static void test_legacy_transition_is_observed(void) {
    SceneHandler handlers[SCENE_COUNT] = {0};
    GameContext game;
    GameEvent event;
    s32 scene = SCENE_BOOT_LOGO;
    s32 timer = 17;

    handlers[SCENE_BOOT_LOGO] = CountCall;
    handlers[SCENE_RACE] = CountCall;
    GameContextInit(&game, &scene, &timer, handlers, SCENE_COUNT);
    scene = SCENE_RACE;
    EXPECT_EQ(1, SceneManagerObserveTransition(&game.scenes));
    EXPECT_EQ(17, timer);
    EXPECT_EQ(1, GameEventQueuePop(&game.events, &event));
    EXPECT_EQ(SCENE_BOOT_LOGO, event.data.sceneChanged.previous);
    EXPECT_EQ(SCENE_RACE, event.data.sceneChanged.current);
}

static void test_invalid_scenes_are_rejected(void) {
    SceneHandler handlers[SCENE_COUNT] = {0};
    GameContext game;
    s32 scene = SCENE_BOOT_LOGO;
    s32 timer = 5;

    handlers[SCENE_BOOT_LOGO] = CountCall;
    GameContextInit(&game, &scene, &timer, handlers, SCENE_COUNT);
    EXPECT_EQ(0, SceneManagerSet(&game.scenes, SCENE_NONE));
    EXPECT_EQ(0, SceneManagerSet(&game.scenes, SCENE_COUNT));
    EXPECT_EQ(SCENE_BOOT_LOGO, scene);
    EXPECT_EQ(5, timer);
    scene = SCENE_NONE;
    EXPECT_EQ(0, SceneManagerDispatch(&game.scenes));
}

static void test_legacy_bridge_preserves_or_sets_timer(void) {
    SceneHandler handlers[SCENE_COUNT] = {0};
    GameContext game;
    s32 scene = SCENE_BOOT_LOGO;
    s32 timer = 41;

    handlers[SCENE_BOOT_LOGO] = CountCall;
    handlers[SCENE_RACE] = CountCall;
    handlers[SCENE_REPLAY] = CountCall;
    GameContextInit(&game, &scene, &timer, handlers, SCENE_COUNT);
    GameContextSetActive(&game);
    EXPECT_EQ(1, GameSceneSet(SCENE_RACE));
    EXPECT_EQ(41, timer);
    EXPECT_EQ(1, GameSceneEnter(SCENE_REPLAY, -1));
    EXPECT_EQ(-1, timer);
}

static void test_event_queue_overflow_is_explicit(void) {
    GameEventQueue queue;
    GameEvent input = {GAME_EVENT_NONE, {{0, 0}}};
    GameEvent output;
    int i;

    GameEventQueueInit(&queue);
    for (i = 0; i < GAME_EVENT_CAPACITY; i++)
        EXPECT_EQ(1, GameEventQueuePush(&queue, &input));
    EXPECT_EQ(0, GameEventQueuePush(&queue, &input));
    EXPECT_EQ(1, queue.dropped);
    for (i = 0; i < GAME_EVENT_CAPACITY; i++)
        EXPECT_EQ(1, GameEventQueuePop(&queue, &output));
    EXPECT_EQ(0, GameEventQueuePop(&queue, &output));
}

int main(void) {
    test_transition_and_dispatch();
    test_legacy_transition_is_observed();
    test_invalid_scenes_are_rejected();
    test_legacy_bridge_preserves_or_sets_timer();
    test_event_queue_overflow_is_explicit();
    if (failures != 0) return 1;
    return 0;
}
