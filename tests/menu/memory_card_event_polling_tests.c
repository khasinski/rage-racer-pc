#include "game/memcard.h"
#include "game/memcard_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

enum {
    HW_IO_EVENT = 100,
    HW_ERROR_EVENT,
    HW_TIMEOUT_EVENT,
    HW_NEW_EVENT,
    SW_IO_EVENT,
    SW_ERROR_EVENT,
    SW_TIMEOUT_EVENT,
    SW_NEW_EVENT,
};
static MemoryCardPoll s_poll;
s32 g_FrameSyncThreshold;
s32 g_SaveElapsedTicks;

static int s_active[108];
static int s_calls[108];
static int s_buInitCalls;
static int s_criticalDepth;
static int s_openCalls;
static int s_enableCalls;
static int s_disableCalls;
static int s_closeCalls;
static long s_enabled[8];
static long s_disabled[8];
static long s_closed[8];

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

long TestEvent(long event) {
    s_calls[event]++;
    return s_active[event];
}

void _bu_init(void) { s_buInitCalls++; }
void EnterCriticalSection(void) { s_criticalDepth++; }
void ExitCriticalSection(void) { s_criticalDepth--; }
long OpenEvent(unsigned long descriptor, long spec, long mode,
               long (*callback)()) {
    (void)descriptor;
    (void)spec;
    (void)mode;
    (void)callback;
    return 100 + s_openCalls++;
}
long EnableEvent(long event) {
    s_enabled[s_enableCalls++] = event;
    return 1;
}
long DisableEvent(long event) {
    s_disabled[s_disableCalls++] = event;
    return 1;
}
long CloseEvent(long event) {
    s_closed[s_closeCalls++] = event;
    return 1;
}

static void ResetMock(void) {
    memset(s_active, 0, sizeof(s_active));
    memset(s_calls, 0, sizeof(s_calls));
    s_poll.ticks = 0;
}

static int TestNoEventAndTimeout(void) {
    ResetMock();
    CHECK(PollMemoryCardHwEvent(&s_poll) == MC_EVENT_NONE);
    CHECK(s_poll.ticks == 1);
    s_poll.ticks = 90;
    CHECK(PollMemoryCardHwEvent(&s_poll) == MC_EVENT_ERROR);
    CHECK(s_poll.ticks == 91);
    CHECK(PollMemoryCardHwEvent(&s_poll) == MC_EVENT_ERROR);
    CHECK(s_poll.ticks == 91);
    return 0;
}

static int TestPollPriority(void) {
    ResetMock();
    s_active[HW_IO_EVENT] = 1;
    s_active[HW_ERROR_EVENT] = 1;
    s_active[HW_TIMEOUT_EVENT] = 1;
    s_active[HW_NEW_EVENT] = 1;
    CHECK(PollMemoryCardHwEvent(&s_poll) == MC_EVENT_NEW_CARD);

    ResetMock();
    s_poll.ticks = 90;
    s_active[HW_IO_EVENT] = 1;
    CHECK(PollMemoryCardHwEvent(&s_poll) == MC_EVENT_IO_COMPLETE);
    CHECK(s_poll.ticks == 91);
    return 0;
}

static int TestWaitAndClear(void) {
    ResetMock();
    s_active[SW_ERROR_EVENT] = 1;
    CHECK(WaitMemoryCardSwEvent() == MC_EVENT_ERROR);

    ClearMemoryCardHwEvents();
    ClearMemoryCardSwEvents();
    CHECK(s_calls[HW_IO_EVENT] > 0 && s_calls[HW_NEW_EVENT] > 0);
    CHECK(s_calls[SW_IO_EVENT] > 0 && s_calls[SW_NEW_EVENT] > 0);
    return 0;
}

static int TestSaveCounter(void) {
    s_buInitCalls = 0;
    g_SaveElapsedTicks = 99;
    RestartMemoryCard();
    /* The host storage root owns the card directories; the BIOS helper must
     * not create bu00/bu10 in the working directory. */
    CHECK(s_buInitCalls == 0 && g_SaveElapsedTicks == 0);

    g_FrameSyncThreshold = 0x80;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == 1);
    g_FrameSyncThreshold = 0x180;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == 3);

    g_SaveElapsedTicks = INT_MAX;
    g_FrameSyncThreshold = 0x80;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == INT_MIN);
    g_SaveElapsedTicks = INT_MAX;
    g_FrameSyncThreshold = 0x180;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == INT_MIN + 1);
    return 0;
}

static int TestEventSessionLifecycle(void) {
    int index;

    s_criticalDepth = 0;
    s_openCalls = 0;
    s_enableCalls = 0;
    s_disableCalls = 0;
    s_closeCalls = 0;
    StartMemoryCardEvents();
    CHECK(s_criticalDepth == 0 && s_openCalls == 8 && s_enableCalls == 8);
    for (index = 0; index < 8; index++) {
        CHECK(s_enabled[index] == 100 + index);
    }

    StopMemoryCardEvents();
    CHECK(s_criticalDepth == 0 && s_disableCalls == 8 && s_closeCalls == 8);
    for (index = 0; index < 8; index++) {
        CHECK(s_disabled[index] == 100 + index);
        CHECK(s_closed[index] == 100 + index);
    }
    return 0;
}

int main(void) {
    StartMemoryCardEvents();
    if (TestNoEventAndTimeout() || TestPollPriority() ||
        TestWaitAndClear() || TestSaveCounter() ||
        TestEventSessionLifecycle()) return 1;
    puts("memory card event polling: ok");
    return 0;
}
