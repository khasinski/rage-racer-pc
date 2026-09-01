#include "game/memcard.h"
#include "game/memcard_internal.h"

enum { MEMORY_CARD_POLL_TIMEOUT_TICKS = 91 };

void ClearMemoryCardHwEvents(void) {
    TestEvent(g_McHwEventIoe);
    TestEvent(g_McHwEventError);
    TestEvent(g_McHwEventTimeout);
    TestEvent(g_McHwEventNew);
}

void ClearMemoryCardSwEvents(void) {
    TestEvent(g_McSwEventIoe);
    TestEvent(g_McSwEventError);
    TestEvent(g_McSwEventTimeout);
    TestEvent(g_McSwEventNew);
}

MemoryCardEvent PollMemoryCardHwEvent(void) {
    MemoryCardEvent result = MC_EVENT_NONE;

    if (TestEvent(g_McHwEventIoe) == 1) {
        result = MC_EVENT_IO_COMPLETE;
    }
    if (TestEvent(g_McHwEventError) == 1) {
        result = MC_EVENT_ERROR;
    }
    if (TestEvent(g_McHwEventTimeout) == 1) {
        result = MC_EVENT_TIMEOUT;
    }
    if (TestEvent(g_McHwEventNew) == 1) {
        result = MC_EVENT_NEW_CARD;
    }

    if (++g_McPollTicks >= MEMORY_CARD_POLL_TIMEOUT_TICKS) {
        result = MC_EVENT_ERROR;
    }

    return result;
}

MemoryCardEvent WaitMemoryCardHwEvent(void) {
    while (1) {
        if (TestEvent(g_McHwEventIoe) == 1) {
            return MC_EVENT_IO_COMPLETE;
        }
        if (TestEvent(g_McHwEventError) == 1) {
            return MC_EVENT_ERROR;
        }
        if (TestEvent(g_McHwEventTimeout) == 1) {
            return MC_EVENT_TIMEOUT;
        }
        if (TestEvent(g_McHwEventNew) == 1) {
            return MC_EVENT_NEW_CARD;
        }
    }
}

MemoryCardEvent WaitMemoryCardSwEvent(void) {
    while (1) {
        if (TestEvent(g_McSwEventIoe) == 1) {
            return MC_EVENT_IO_COMPLETE;
        }
        if (TestEvent(g_McSwEventError) == 1) {
            return MC_EVENT_ERROR;
        }
        if (TestEvent(g_McSwEventTimeout) == 1) {
            return MC_EVENT_TIMEOUT;
        }
        if (TestEvent(g_McSwEventNew) == 1) {
            return MC_EVENT_NEW_CARD;
        }
    }
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
