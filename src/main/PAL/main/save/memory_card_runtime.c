#include "game/memcard.h"
#include "game/memcard_internal.h"
#include <stdio.h>

static void RequestCardInfo(MemoryCardPoll *poll, s32 handle) {
    _card_info(handle);
    poll->state = MC_STATUS_WAIT_INFO;
    poll->ticks = 0;
    poll->result = MC_CARD_RESULT_PENDING;
}

static void HandleCardInfoEvent(MemoryCardPoll *poll, s32 handle) {
    MemoryCardEvent event = PollMemoryCardHwEvent(poll);

    if (event == MC_EVENT_NONE) return;

    switch (event) {
    case MC_EVENT_IO_COMPLETE:
        poll->pendingResult = MC_CARD_RESULT_READY;
        poll->state = poll->lastStatus == MC_CARD_RESULT_READY
                              ? MC_STATUS_PUBLISH_RESULT
                              : MC_STATUS_REQUEST_LOAD;
        break;
    case MC_EVENT_TIMEOUT:
        poll->pendingResult = MC_CARD_RESULT_NO_CARD;
        poll->state = MC_STATUS_PUBLISH_RESULT;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_NEW_CARD:
        poll->pendingResult = MC_CARD_RESULT_NEW_CARD;
        ClearMemoryCardSwEvents();
        _card_clear(handle);
        WaitMemoryCardSwEvent();
        poll->state = MC_STATUS_REQUEST_LOAD;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_ERROR:
    default:
        poll->pendingResult = MC_CARD_RESULT_ERROR;
        poll->state = MC_STATUS_PUBLISH_RESULT;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    }
}

static void RequestCardLoad(MemoryCardPoll *poll, s32 handle) {
    ClearMemoryCardHwEvents();
    _card_load(handle);
    poll->state = MC_STATUS_WAIT_LOAD;
    poll->ticks = 0;
}

static void HandleCardLoadEvent(MemoryCardPoll *poll) {
    MemoryCardEvent event = PollMemoryCardHwEvent(poll);

    if (event == MC_EVENT_NONE) return;

    poll->state = MC_STATUS_PUBLISH_RESULT;
    switch (event) {
    case MC_EVENT_IO_COMPLETE:
        poll->lastStatus = MC_CARD_RESULT_READY;
        break;
    case MC_EVENT_TIMEOUT:
        poll->pendingResult = MC_CARD_RESULT_NO_CARD;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_NEW_CARD:
        poll->pendingResult = MC_CARD_RESULT_UNFORMATTED;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_ERROR:
    default:
        poll->pendingResult = MC_CARD_RESULT_ERROR;
        poll->lastStatus = MC_CARD_RESULT_PENDING;
        break;
    }
}


s32 PollMemoryCardStatus(MemoryCardPoll *poll, s32 port, s32 slot) {
    s32 handle;

    handle = (port * 16) + slot;

    switch (poll->state) {
    case MC_STATUS_REQUEST_INFO:
        RequestCardInfo(poll, handle);
        break;

    case MC_STATUS_WAIT_INFO:
        HandleCardInfoEvent(poll, handle);
        break;

    case MC_STATUS_REQUEST_LOAD:
        RequestCardLoad(poll, handle);
        break;

    case MC_STATUS_WAIT_LOAD:
        HandleCardLoadEvent(poll);
        break;

    case MC_STATUS_PUBLISH_RESULT:
        poll->state = MC_STATUS_REQUEST_INFO;
        poll->result = poll->pendingResult;
        break;

    default:
        poll->state = MC_STATUS_REQUEST_INFO;
        poll->result = MC_CARD_RESULT_PENDING;
    }

    return poll->result;
}

s32 FormatMemoryCard(s32 port, s32 slot) {
    char device[8];
    s32 status;

    snprintf(device, sizeof(device), "bu%1d%1d:", port, slot);
    ClearMemoryCardSwEvents();
    BiosFormatDevice(device);
    status = WaitMemoryCardSwEvent();

    if (status != MC_EVENT_IO_COMPLETE) {
        if (status == MC_EVENT_TIMEOUT) {
            status = MC_CARD_RESULT_NO_CARD;
        } else {
            status = MC_CARD_RESULT_ERROR;
        }
    }

    return status;
}
