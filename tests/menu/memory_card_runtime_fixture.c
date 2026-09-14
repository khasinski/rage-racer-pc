/* Use the actual asynchronous status machine alongside the menu's existing
 * synthetic status sweep. Only the hardware events are immediate fixtures. */
#include "game/memcard.h"
#include "game/memcard_internal.h"

#define PollMemoryCardStatus FixturePollMemoryCardStatus
#define FormatMemoryCard FixtureFormatMemoryCard
#include "../../src/main/PAL/main/save/memory_card_runtime.c"

extern MemoryCardPoll s_poll;

MemoryCardEvent PollMemoryCardHwEvent(MemoryCardPoll *poll) {
    (void)poll;
    return MC_EVENT_IO_COMPLETE;
}
MemoryCardEvent WaitMemoryCardSwEvent(void) { return MC_EVENT_IO_COMPLETE; }
void ClearMemoryCardHwEvents(void) {}
void ClearMemoryCardSwEvents(void) {}
long _card_clear(long channel) { (void)channel; return 1; }
long BiosFormatDevice(void *device) { (void)device; return 1; }

void FixtureResetMemoryCardStatus(void) {
    s_poll.state = MC_STATUS_REQUEST_INFO;
    s_poll.ticks = 0;
    s_poll.result = MC_CARD_RESULT_PENDING;
    s_poll.pendingResult = MC_CARD_RESULT_PENDING;
    s_poll.lastStatus = MC_CARD_RESULT_PENDING;
}
