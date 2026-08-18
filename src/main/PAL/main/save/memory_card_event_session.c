#include "common.h"
#include "game/memcard.h"

void StartMemoryCardEvents(void) {
    OpenMemoryCardEvents();
    EnableMemoryCardEvents();
}

void StopMemoryCardEvents(void) {
    DisableMemoryCardEvents();
    CloseMemoryCardEvents();
}

void CardReadAndSetMode(s32 param) {
    ClearMemoryCardSwEvents();
    while (_card_clear((u8)param) == 0) {}
    WaitMemoryCardSwEvent();
    ClearMemoryCardHwEvents();
    _card_load((u8)param);
    WaitMemoryCardHwEvent();
}
