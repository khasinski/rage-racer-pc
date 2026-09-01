#include "game/memcard.h"

#include <stdio.h>

MemoryCardStatusState g_McStatusState;
s32 g_McPollTicks;
s32 g_McStatusResult;
s32 g_McPollStatus;
s32 g_McLastCardStatus;
s32 g_McHwEventIoe;
s32 g_McHwEventError;
s32 g_McHwEventTimeout;
s32 g_McHwEventNew;
s32 g_McSwEventIoe;
s32 g_McSwEventError;
s32 g_McSwEventTimeout;
s32 g_McSwEventNew;
char g_FmtCardDevice[] = "bu%d%d:";

static MemoryCardEvent s_hwEvent;
static MemoryCardEvent s_swEvent;
static s32 s_infoHandle;
static s32 s_loadHandle;
static s32 s_clearHandle;
static s32 s_clearSwCalls;
static s32 s_waitSwCalls;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

long _card_info(long handle) {
    s_infoHandle = (s32)handle;
    return 0;
}
long _card_load(long handle) {
    s_loadHandle = (s32)handle;
    return 0;
}
long _card_clear(long handle) {
    s_clearHandle = (s32)handle;
    return 0;
}
MemoryCardEvent PollMemoryCardHwEvent(void) {
    MemoryCardEvent event = s_hwEvent;
    s_hwEvent = MC_EVENT_NONE;
    return event;
}
void ClearMemoryCardHwEvents(void) {}
void ClearMemoryCardSwEvents(void) { s_clearSwCalls++; }
MemoryCardEvent WaitMemoryCardSwEvent(void) {
    s_waitSwCalls++;
    return s_swEvent;
}
long BiosFormatDevice(void *device) {
    (void)device;
    return 0;
}

void EnterCriticalSection(void) {}
void ExitCriticalSection(void) {}
long OpenEvent(unsigned long desc, long spec, long mode, long (*func)()) {
    (void)desc; (void)spec; (void)mode; (void)func;
    return 1;
}
long CloseEvent(long event) { (void)event; return 1; }
long EnableEvent(long event) { (void)event; return 1; }
long DisableEvent(long event) { (void)event; return 1; }

static void ResetPoller(void) {
    g_McStatusState = MC_STATUS_REQUEST_INFO;
    g_McPollTicks = 99;
    g_McStatusResult = 0;
    g_McPollStatus = 0;
    g_McLastCardStatus = 0;
    s_hwEvent = MC_EVENT_NONE;
    s_swEvent = MC_EVENT_IO_COMPLETE;
    s_infoHandle = -1;
    s_loadHandle = -1;
    s_clearHandle = -1;
    s_clearSwCalls = 0;
    s_waitSwCalls = 0;
}

static int TestSuccessfulPoll(void) {
    ResetPoller();
    CHECK(PollMemoryCardStatus(2, 3) == 0);
    CHECK(s_infoHandle == 35);
    CHECK(g_McStatusState == MC_STATUS_WAIT_INFO && g_McPollTicks == 0);

    s_hwEvent = MC_EVENT_IO_COMPLETE;
    CHECK(PollMemoryCardStatus(2, 3) == 0);
    CHECK(g_McStatusState == MC_STATUS_REQUEST_LOAD);
    CHECK(PollMemoryCardStatus(2, 3) == 0);
    CHECK(s_loadHandle == 35 && g_McStatusState == MC_STATUS_WAIT_LOAD);

    s_hwEvent = MC_EVENT_IO_COMPLETE;
    CHECK(PollMemoryCardStatus(2, 3) == 0);
    CHECK(g_McStatusState == MC_STATUS_PUBLISH_RESULT);
    CHECK(PollMemoryCardStatus(2, 3) == MC_EVENT_IO_COMPLETE);
    CHECK(g_McStatusState == MC_STATUS_REQUEST_INFO);
    return 0;
}

static int TestInfoTimeout(void) {
    ResetPoller();
    PollMemoryCardStatus(0, 0);
    s_hwEvent = MC_EVENT_TIMEOUT;
    PollMemoryCardStatus(0, 0);
    CHECK(g_McStatusState == MC_STATUS_PUBLISH_RESULT);
    CHECK(PollMemoryCardStatus(0, 0) == -1);
    return 0;
}

static int TestNewCardLoadFailure(void) {
    ResetPoller();
    PollMemoryCardStatus(0, 1);
    s_hwEvent = MC_EVENT_NEW_CARD;
    PollMemoryCardStatus(0, 1);
    CHECK(g_McPollStatus == 2 && s_clearHandle == 1);
    CHECK(s_clearSwCalls == 1 && s_waitSwCalls == 1);

    PollMemoryCardStatus(0, 1);
    s_hwEvent = MC_EVENT_NEW_CARD;
    PollMemoryCardStatus(0, 1);
    CHECK(PollMemoryCardStatus(0, 1) == -2);
    return 0;
}

static int TestFormatResults(void) {
    ResetPoller();
    s_swEvent = MC_EVENT_IO_COMPLETE;
    CHECK(FormatMemoryCard(0, 0) == 1);
    s_swEvent = MC_EVENT_TIMEOUT;
    CHECK(FormatMemoryCard(0, 0) == -1);
    s_swEvent = MC_EVENT_ERROR;
    CHECK(FormatMemoryCard(0, 0) == -3);
    return 0;
}

int main(void) {
    if (TestSuccessfulPoll() || TestInfoTimeout() ||
        TestNewCardLoadFailure() || TestFormatResults()) return 1;
    puts("memory card runtime state machine: ok");
    return 0;
}
