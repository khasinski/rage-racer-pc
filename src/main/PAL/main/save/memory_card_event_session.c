#include "game/memcard.h"
#include "game/memcard_internal.h"

enum {
    MEMORY_CARD_EVENT_COUNT = 4,
    MEMORY_CARD_EVENT_MODE = 0x2000,
    MEMORY_CARD_POLL_DEADLINE_TICKS = 91,
};

static const unsigned long MEMORY_CARD_HW_EVENT_CLASS = 0xF4000001UL;
static const unsigned long MEMORY_CARD_SW_EVENT_CLASS = 0xF0000011UL;

static const s32 s_eventSpecs[MEMORY_CARD_EVENT_COUNT] = {
    0x0004, 0x8000, 0x0100, 0x2000,
};

static const MemoryCardEvent s_eventResults[MEMORY_CARD_EVENT_COUNT] = {
    MC_EVENT_IO_COMPLETE, MC_EVENT_ERROR, MC_EVENT_TIMEOUT, MC_EVENT_NEW_CARD,
};

static s32 s_hwEventHandles[MEMORY_CARD_EVENT_COUNT];
static s32 s_swEventHandles[MEMORY_CARD_EVENT_COUNT];

static void OpenEventClass(unsigned long eventClass, s32 *handles) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        handles[index] = OpenEvent(eventClass, s_eventSpecs[index],
                                   MEMORY_CARD_EVENT_MODE, 0);
    }
}

static void ApplyToEventHandles(long (*operation)(long)) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        operation(s_hwEventHandles[index]);
    }
    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        operation(s_swEventHandles[index]);
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

static void ClearEventHandles(const s32 *handles) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        TestEvent(handles[index]);
    }
}

void ClearMemoryCardHwEvents(void) {
    ClearEventHandles(s_hwEventHandles);
}

void ClearMemoryCardSwEvents(void) {
    ClearEventHandles(s_swEventHandles);
}

MemoryCardEvent PollMemoryCardHwEvent(MemoryCardPoll *poll) {
    MemoryCardEvent result = MC_EVENT_NONE;
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        if (TestEvent(s_hwEventHandles[index]) == 1) {
            result = s_eventResults[index];
        }
    }

    if (poll->ticks < MEMORY_CARD_POLL_DEADLINE_TICKS) poll->ticks++;
    if (result == MC_EVENT_NONE &&
        poll->ticks >= MEMORY_CARD_POLL_DEADLINE_TICKS) {
        return MC_EVENT_ERROR;
    }
    return result;
}

static MemoryCardEvent WaitForEvent(const s32 *handles) {
    for (;;) {
        s32 index;

        for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
            if (TestEvent(handles[index]) == 1) return s_eventResults[index];
        }
    }
}

MemoryCardEvent WaitMemoryCardSwEvent(void) {
    return WaitForEvent(s_swEventHandles);
}

void RestartMemoryCard(void) {
    _bu_init();
    g_SaveElapsedTicks = 0;
}

void AdvanceSaveHeaderCounter(void) {
    u32 increment = g_FrameSyncThreshold == 0x80 ? 1u : 2u;

    /* The on-disc counter is a 32-bit bit pattern. Define its wrap explicitly
     * instead of relying on signed overflow after a sufficiently long save. */
    g_SaveElapsedTicks = (s32)((u32)g_SaveElapsedTicks + increment);
}
