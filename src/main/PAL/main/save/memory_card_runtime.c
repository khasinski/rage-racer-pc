#include "game/memcard.h"
#include <stdio.h>

/* The poller's own working status word. Distinct from menu.h's
 * g_McPollStatus (g_McCardStatus), which is the code the menu reads. */


s32 PollMemoryCardStatus(s32 port, s32 slot) {
    s32 handle;
    s32 two;
    s32 status;
    s32 state;

    handle = (port * 16) + slot;

    switch (g_McStatusState) {
    case MC_STATUS_REQUEST_INFO:
        _card_info(handle);
        g_McStatusState = MC_STATUS_WAIT_INFO;
        g_McPollTicks = 0;
        g_McStatusResult = 0;
        break;

    case MC_STATUS_WAIT_INFO:
        status = PollMemoryCardHwEvent();
        if (status == 0) {
            break;
        }

        two = 2;
        if (status != two) {
        if (status < 3) {
            if (status == 1) {
                goto case1_ready;
            }
            state = -3;
        g_McPollStatus = state;
        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        g_McLastCardStatus = 0;
        break;
        }
        if (status != 3) {
        if (status != 4) {
        state = -3;
        goto fail_case1;

case1_ready:
        g_McPollStatus = status;
        if (g_McLastCardStatus == status) {
            g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        } else {
            g_McStatusState = MC_STATUS_REQUEST_LOAD;
        }
        break;

        }
        } else {
        state = -1;
        g_McPollStatus = state;
        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        g_McLastCardStatus = 0;
        break;

        }
        g_McPollStatus = two;
        ClearMemoryCardSwEvents();
        _card_clear(handle);
        WaitMemoryCardSwEvent();
        g_McStatusState = MC_STATUS_REQUEST_LOAD;
        g_McLastCardStatus = 0;
        break;

        }
        state = -3;

fail_case1:
        g_McPollStatus = state;
        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        g_McLastCardStatus = 0;
        break;

    case MC_STATUS_REQUEST_LOAD:
        ClearMemoryCardHwEvents();
        _card_load(handle);
        g_McStatusState = MC_STATUS_WAIT_LOAD;
        g_McPollTicks = 0;
        break;

    case MC_STATUS_WAIT_LOAD:
        status = PollMemoryCardHwEvent();
        if (status == 0) {
            break;
        }

        g_McStatusState = MC_STATUS_PUBLISH_RESULT;
        if (status != 2) {
        if (status < 3) {
            if (status == 1) {
                goto case3_ready;
            }
            state = -3;
        g_McPollStatus = state;
        g_McLastCardStatus = 0;
        break;
        }
        if (status != 3) {
        if (status != 4) {
        state = -3;
        g_McPollStatus = state;
        g_McLastCardStatus = 0;
        break;

case3_ready:
        g_McLastCardStatus = status;
        break;

        }
        } else {
        state = -1;
        g_McPollStatus = state;
        g_McLastCardStatus = 0;
        break;

        }
        state = -2;

        } else {
        state = -3;

        }
        g_McPollStatus = state;
        g_McLastCardStatus = 0;
        break;

    case MC_STATUS_PUBLISH_RESULT:
        g_McStatusState = MC_STATUS_REQUEST_INFO;
        g_McStatusResult = g_McPollStatus;
        break;

    default:
        g_McStatusState = MC_STATUS_REQUEST_INFO;
        g_McStatusResult = 0;
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

    if (status != 1) {
        if (status == 3) {
            status = -1;
        } else {
            status = -3;
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
