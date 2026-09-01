#include "game/memcard.h"
#include <stdio.h>

static void RequestCardInfo(s32 handle) {
    _card_info(handle);
    g_McStatusState = MC_STATUS_WAIT_INFO;
    g_McPollTicks = 0;
    g_McStatusResult = MC_CARD_RESULT_PENDING;
}

static void HandleCardInfoEvent(s32 handle) {
    MemoryCardEvent event = PollMemoryCardHwEvent();

    if (event == MC_EVENT_NONE) return;

    switch (event) {
    case MC_EVENT_IO_COMPLETE:
        g_McPollStatus = MC_CARD_RESULT_READY;
        g_McStatusState = g_McLastCardStatus == MC_CARD_RESULT_READY
                              ? MC_STATUS_PUBLISH_RESULT
                              : MC_STATUS_REQUEST_LOAD;
        break;
    case MC_EVENT_TIMEOUT:
        g_McPollStatus = MC_CARD_RESULT_NO_CARD;
        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_NEW_CARD:
        g_McPollStatus = MC_CARD_RESULT_NEW_CARD;
        ClearMemoryCardSwEvents();
        _card_clear(handle);
        WaitMemoryCardSwEvent();
        g_McStatusState = MC_STATUS_REQUEST_LOAD;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_ERROR:
    default:
        g_McPollStatus = MC_CARD_RESULT_ERROR;
        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    }
}

static void RequestCardLoad(s32 handle) {
    ClearMemoryCardHwEvents();
    _card_load(handle);
    g_McStatusState = MC_STATUS_WAIT_LOAD;
    g_McPollTicks = 0;
}

static void HandleCardLoadEvent(void) {
    MemoryCardEvent event = PollMemoryCardHwEvent();

    if (event == MC_EVENT_NONE) return;

    g_McStatusState = MC_STATUS_PUBLISH_RESULT;
    switch (event) {
    case MC_EVENT_IO_COMPLETE:
        g_McLastCardStatus = MC_CARD_RESULT_READY;
        break;
    case MC_EVENT_TIMEOUT:
        g_McPollStatus = MC_CARD_RESULT_NO_CARD;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_NEW_CARD:
        g_McPollStatus = MC_CARD_RESULT_UNFORMATTED;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    case MC_EVENT_ERROR:
    default:
        g_McPollStatus = MC_CARD_RESULT_ERROR;
        g_McLastCardStatus = MC_CARD_RESULT_PENDING;
        break;
    }
}


s32 PollMemoryCardStatus(s32 port, s32 slot) {
    s32 handle;

    handle = (port * 16) + slot;

    switch (g_McStatusState) {
    case MC_STATUS_REQUEST_INFO:
        RequestCardInfo(handle);
        break;

    case MC_STATUS_WAIT_INFO:
        HandleCardInfoEvent(handle);
        break;

    case MC_STATUS_REQUEST_LOAD:
        RequestCardLoad(handle);
        break;

    case MC_STATUS_WAIT_LOAD:
        HandleCardLoadEvent();
        break;

    case MC_STATUS_PUBLISH_RESULT:
        g_McStatusState = MC_STATUS_REQUEST_INFO;
        g_McStatusResult = g_McPollStatus;
        break;

    default:
        g_McStatusState = MC_STATUS_REQUEST_INFO;
        g_McStatusResult = MC_CARD_RESULT_PENDING;
    }

    return g_McStatusResult;
}

s32 FormatMemoryCard(s32 port, s32 slot) {
    char device[8];
    s32 status;

    sprintf(device, g_FmtCardDevice, port, slot);
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

/* The eight libcard event descriptors: [0..3] are the hardware class
 * 0xF4000001 and [4..7] the software class 0xF0000011, each in the order IOE,
 * Error, Timeout, NewCard -- which is why every poller below returns
 * index + 1. They are eight scalars rather than one array because with an
 * array symbol gcc 2.6.3 keeps the base address live in a callee-saved
 * register across the TestEvent calls, which grows PollMemoryCardHwEvent's
 * frame from 24 to 32 bytes. */

void OpenMemoryCardEvents(void) {
    EnterCriticalSection();
    g_McHwEventIoe = OpenEvent(0xF4000001, 0x0004, 0x2000, 0);
    g_McHwEventError = OpenEvent(0xF4000001, 0x8000, 0x2000, 0);
    g_McHwEventTimeout = OpenEvent(0xF4000001, 0x0100, 0x2000, 0);
    g_McHwEventNew = OpenEvent(0xF4000001, 0x2000, 0x2000, 0);
    g_McSwEventIoe = OpenEvent(0xF0000011, 0x0004, 0x2000, 0);
    g_McSwEventError = OpenEvent(0xF0000011, 0x8000, 0x2000, 0);
    g_McSwEventTimeout = OpenEvent(0xF0000011, 0x0100, 0x2000, 0);
    g_McSwEventNew = OpenEvent(0xF0000011, 0x2000, 0x2000, 0);
    ExitCriticalSection();
}

void EnableMemoryCardEvents(void) {
    EnableEvent(g_McHwEventIoe);
    EnableEvent(g_McHwEventError);
    EnableEvent(g_McHwEventTimeout);
    EnableEvent(g_McHwEventNew);
    EnableEvent(g_McSwEventIoe);
    EnableEvent(g_McSwEventError);
    EnableEvent(g_McSwEventTimeout);
    EnableEvent(g_McSwEventNew);
}

void DisableMemoryCardEvents(void) {
    DisableEvent(g_McHwEventIoe);
    DisableEvent(g_McHwEventError);
    DisableEvent(g_McHwEventTimeout);
    DisableEvent(g_McHwEventNew);
    DisableEvent(g_McSwEventIoe);
    DisableEvent(g_McSwEventError);
    DisableEvent(g_McSwEventTimeout);
    DisableEvent(g_McSwEventNew);
}

void CloseMemoryCardEvents(void) {
    EnterCriticalSection();
    CloseEvent(g_McHwEventIoe);
    CloseEvent(g_McHwEventError);
    CloseEvent(g_McHwEventTimeout);
    CloseEvent(g_McHwEventNew);
    CloseEvent(g_McSwEventIoe);
    CloseEvent(g_McSwEventError);
    CloseEvent(g_McSwEventTimeout);
    CloseEvent(g_McSwEventNew);
    ExitCriticalSection();
}
