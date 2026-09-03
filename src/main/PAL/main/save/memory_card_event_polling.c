#include "game/memcard.h"
#include "game/memcard_internal.h"

enum { MEMORY_CARD_POLL_DEADLINE_TICKS = 91 };

enum { MEMORY_CARD_EVENT_COUNT = 4 };

static const MemoryCardEvent s_eventResults[MEMORY_CARD_EVENT_COUNT] = {
    MC_EVENT_IO_COMPLETE, MC_EVENT_ERROR, MC_EVENT_TIMEOUT, MC_EVENT_NEW_CARD,
};

static s32 *const s_hwEventHandles[MEMORY_CARD_EVENT_COUNT] = {
    &g_McHwEventIoe, &g_McHwEventError, &g_McHwEventTimeout, &g_McHwEventNew,
};

static s32 *const s_swEventHandles[MEMORY_CARD_EVENT_COUNT] = {
    &g_McSwEventIoe, &g_McSwEventError, &g_McSwEventTimeout, &g_McSwEventNew,
};

static void ClearEventHandles(s32 *const *handles) {
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        TestEvent(*handles[index]);
    }
}

void ClearMemoryCardHwEvents(void) {
    ClearEventHandles(s_hwEventHandles);
}

void ClearMemoryCardSwEvents(void) {
    ClearEventHandles(s_swEventHandles);
}

MemoryCardEvent PollMemoryCardHwEvent(void) {
    MemoryCardEvent result = MC_EVENT_NONE;
    s32 index;

    for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
        if (TestEvent(*s_hwEventHandles[index]) == 1) {
            result = s_eventResults[index];
        }
    }

    g_McPollTicks++;
    if (result == MC_EVENT_NONE &&
        g_McPollTicks >= MEMORY_CARD_POLL_DEADLINE_TICKS) {
        result = MC_EVENT_ERROR;
    }

    return result;
}

static MemoryCardEvent WaitForEvent(s32 *const *handles) {
    while (1) {
        s32 index;

        for (index = 0; index < MEMORY_CARD_EVENT_COUNT; index++) {
            if (TestEvent(*handles[index]) == 1) {
                return s_eventResults[index];
            }
        }
    }
}

MemoryCardEvent WaitMemoryCardHwEvent(void) {
    return WaitForEvent(s_hwEventHandles);
}

MemoryCardEvent WaitMemoryCardSwEvent(void) {
    return WaitForEvent(s_swEventHandles);
}
void RestartMemoryCard(void) {
    BiosBuInit();
    g_SaveElapsedTicks = 0;
}


void AdvanceSaveHeaderCounter(void) {
    if (g_FrameSyncThreshold == 0x80) {
        g_SaveElapsedTicks++;
    } else {
        g_SaveElapsedTicks += 2;
    }
}
