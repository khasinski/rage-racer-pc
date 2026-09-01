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

void BiosBuInit(void) { s_buInitCalls++; }

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

int main(void) {
    if (TestNoEventAndTimeout() || TestPollPriority() ||
        TestWaitAndClear() || TestSaveCounter()) return 1;
    puts("memory card event polling: ok");
    return 0;
}
