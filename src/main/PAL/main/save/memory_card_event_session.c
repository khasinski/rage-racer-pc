#include "game/memcard.h"

enum {
    MEMORY_CARD_EVENT_COUNT = 4,
    MEMORY_CARD_EVENT_MODE = 0x2000,
};

static const unsigned long MEMORY_CARD_HW_EVENT_CLASS = 0xF4000001UL;
static const unsigned long MEMORY_CARD_SW_EVENT_CLASS = 0xF0000011UL;

static const s32 s_eventSpecs[MEMORY_CARD_EVENT_COUNT] = {
    0x0004, 0x8000, 0x0100, 0x2000,
};

static s32 *const s_hwEventHandles[MEMORY_CARD_EVENT_COUNT] = {
    &g_McHwEventIoe, &g_McHwEventError, &g_McHwEventTimeout, &g_McHwEventNew,
};

static s32 *const s_swEventHandles[MEMORY_CARD_EVENT_COUNT] = {
    &g_McSwEventIoe, &g_McSwEventError, &g_McSwEventTimeout, &g_McSwEventNew,
};

static void OpenEventClass(unsigned long eventClass, s32 *const *handles) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        *handles[index] = OpenEvent(eventClass, s_eventSpecs[index],
                                    MEMORY_CARD_EVENT_MODE, 0);
    }
}

static void ApplyToEventHandles(long (*operation)(long)) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        operation(*s_hwEventHandles[index]);
    }
    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        operation(*s_swEventHandles[index]);
    }
}

static void OpenMemoryCardEvents(void) {
    EnterCriticalSection();
    OpenEventClass(MEMORY_CARD_HW_EVENT_CLASS, s_hwEventHandles);
    OpenEventClass(MEMORY_CARD_SW_EVENT_CLASS, s_swEventHandles);
    ExitCriticalSection();
}

static void EnableMemoryCardEvents(void) {
    ApplyToEventHandles(EnableEvent);
}

static void DisableMemoryCardEvents(void) {
    ApplyToEventHandles(DisableEvent);
}

static void CloseMemoryCardEvents(void) {
    EnterCriticalSection();
    ApplyToEventHandles(CloseEvent);
    ExitCriticalSection();
}

void StartMemoryCardEvents(void) {
    OpenMemoryCardEvents();
    EnableMemoryCardEvents();
}

void StopMemoryCardEvents(void) {
    DisableMemoryCardEvents();
    CloseMemoryCardEvents();
}
