/* Use the actual asynchronous status machine alongside the menu's existing
 * synthetic status sweep. Only the hardware events are immediate fixtures. */
#include "game/memcard.h"
#include "game/memcard_internal.h"

#define PollMemoryCardStatus FixturePollMemoryCardStatus
#define FormatMemoryCard FixtureFormatMemoryCard
#include "../../src/main/PAL/main/save/memory_card_runtime.c"

MemoryCardStatusState g_McStatusState;
s32 g_McPollTicks;
s32 g_McStatusResult;
s32 g_McPollStatus;
s32 g_McLastCardStatus;
char g_FmtCardDevice[] = "bu%d%d:";

MemoryCardEvent PollMemoryCardHwEvent(void) { return MC_EVENT_IO_COMPLETE; }
MemoryCardEvent WaitMemoryCardSwEvent(void) { return MC_EVENT_IO_COMPLETE; }
void ClearMemoryCardHwEvents(void) {}
void ClearMemoryCardSwEvents(void) {}
long _card_clear(long channel) { (void)channel; return 1; }
long BiosFormatDevice(void *device) { (void)device; return 1; }

void FixtureResetMemoryCardStatus(void) {
    g_McStatusState = MC_STATUS_REQUEST_INFO;
    g_McPollTicks = 0;
    g_McStatusResult = MC_CARD_RESULT_PENDING;
    g_McPollStatus = MC_CARD_RESULT_PENDING;
    g_McLastCardStatus = MC_CARD_RESULT_PENDING;
}
