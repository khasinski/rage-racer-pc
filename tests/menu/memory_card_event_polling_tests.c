#include "game/memcard.h"

#include <stdio.h>
#include <string.h>

s32 g_McHwEventIoe = 1;
s32 g_McHwEventError = 2;
s32 g_McHwEventTimeout = 3;
s32 g_McHwEventNew = 4;
s32 g_McSwEventIoe = 5;
s32 g_McSwEventError = 6;
s32 g_McSwEventTimeout = 7;
s32 g_McSwEventNew = 8;
s32 g_McPollTicks;
s32 g_FrameSyncThreshold;
s32 g_SaveElapsedTicks;

static int s_active[9];
static int s_calls[9];
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
    g_McPollTicks = 0;
}

static int TestNoEventAndTimeout(void) {
    ResetMock();
    CHECK(PollMemoryCardHwEvent() == MC_EVENT_NONE);
    CHECK(g_McPollTicks == 1);
    g_McPollTicks = 90;
    CHECK(PollMemoryCardHwEvent() == MC_EVENT_ERROR);
    CHECK(g_McPollTicks == 91);
    return 0;
}

static int TestPollPriority(void) {
    ResetMock();
    s_active[g_McHwEventIoe] = 1;
    s_active[g_McHwEventError] = 1;
    s_active[g_McHwEventTimeout] = 1;
    s_active[g_McHwEventNew] = 1;
    CHECK(PollMemoryCardHwEvent() == MC_EVENT_NEW_CARD);

    ResetMock();
    g_McPollTicks = 90;
    s_active[g_McHwEventIoe] = 1;
    CHECK(PollMemoryCardHwEvent() == MC_EVENT_IO_COMPLETE);
    CHECK(g_McPollTicks == 91);
    return 0;
}

static int TestWaitAndClear(void) {
    ResetMock();
    s_active[g_McHwEventTimeout] = 1;
    CHECK(WaitMemoryCardHwEvent() == MC_EVENT_TIMEOUT);
    s_active[g_McSwEventError] = 1;
    CHECK(WaitMemoryCardSwEvent() == MC_EVENT_ERROR);

    ClearMemoryCardHwEvents();
    ClearMemoryCardSwEvents();
    CHECK(s_calls[g_McHwEventIoe] > 0 && s_calls[g_McHwEventNew] > 0);
    CHECK(s_calls[g_McSwEventIoe] > 0 && s_calls[g_McSwEventNew] > 0);
    return 0;
}

static int TestSaveCounter(void) {
    s_buInitCalls = 0;
    g_SaveElapsedTicks = 99;
    RestartMemoryCard();
    CHECK(s_buInitCalls == 1 && g_SaveElapsedTicks == 0);

    g_FrameSyncThreshold = 0x80;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == 1);
    g_FrameSyncThreshold = 0x180;
    AdvanceSaveHeaderCounter();
    CHECK(g_SaveElapsedTicks == 3);
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
    CHECK(g_McHwEventIoe == 100 && g_McHwEventNew == 103);
    CHECK(g_McSwEventIoe == 104 && g_McSwEventNew == 107);
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
    if (TestNoEventAndTimeout() || TestPollPriority() ||
        TestWaitAndClear() || TestSaveCounter() ||
        TestEventSessionLifecycle()) return 1;
    puts("memory card event polling: ok");
    return 0;
}
