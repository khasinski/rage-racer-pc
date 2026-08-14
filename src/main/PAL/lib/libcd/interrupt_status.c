#include <sys/types.h>
#include <stdio.h>

#include "common.h"
#include "psyq/cd.h"
#include "psyq/cd_internal.h"

#define CD_STATUS_WORD (*(u_long *)(void *)&g_CdStatusByte)
#define CD_ERROR_WORD (*(u_long *)(void *)&g_CdErrorByte)

/*
 * LEFT RAW ON PURPOSE. This is libcd's interrupt decoder:
 * it reads the 8-byte response FIFO, decodes intr codes 1..5 into g_CdSyncStatus
 * and the two result buffers, and is drained in a while loop by CD_sync,
 * CD_ready, CD_cw and the IRQ2 handler. Every other libcd internal in this
 * image was pinned because it stores its own name into a trace slot; this one
 * owns only "DiskError: " and "CDROM: unknown intr", neither of which names a
 * function. CD_intr / CD_getintr / CD_status / CD_readIntr are all
 * unfalsifiable here, so no name is asserted.
 *
 * Confirmed under emulation 2026-08-03: across a boot-to-race trace it is the
 * only writer of g_CdSyncStatus, g_CdSyncResult, g_CdSyncStatus.ready and
 * g_CdReadyResult, and reads no global at all. That matches the decoder
 * reading above exactly -- but it still does not pin a Sony name, so the
 * decision to leave it raw stands.
 */


static __inline__ void copy8(u_char *d, u_char *s) {
    long n;
    if (d == 0) {
        return;
    }
    n = 7;
    do {
        *d++ = *s++;
    } while (--n != -1);
}

long CdReadInterruptStatus(void) {
    volatile u_char mode;
    volatile u_char buf[8];
    long i;
    long flag;
    long v;
    volatile u_char *p;
    volatile u_char *q;

    *g_CdReg0 = 1;
    p = g_CdReg3;
    mode = *p % 8;
    if (mode == 0) {
        return 0;
    }
    flag = 0;
    while (mode != (*p % 8)) {
        mode = *p % 8;
    }
    i = 0;
    q = buf;
    for (; i < 8; i++) {
        if (!(*g_CdReg0 & 0x20)) {
            break;
        }
        *q++ = *g_CdReg1;
    }
    if (i < 8) {
        volatile u_char *r;
        r = (volatile u_char *)(i + (long)buf);
        do {
            *r++ = 0;
        } while ((long)r < (long)&buf[8]);
    }

    *g_CdReg0 = 1;
    *g_CdReg3 = 7;
    *g_CdReg2 = 7;

    if (!(mode == 3 && g_CdCommandAckHasStatus[g_CdLastCommand] == 0)) {
        if (!(CD_STATUS_WORD & 0x10) && (buf[0] & 0x10)) {
            g_CdShellOpenCount++;
        }
        v = buf[0];
        flag = v & 0x1d;
        CD_STATUS_WORD = v;
        CD_ERROR_WORD = buf[1];
    }

    if (mode == 5) {
        puts(&g_MsgCdDiskError);
        if (g_CdDebugLevel > 0) {
            printf(&g_MsgCdErrorCommandCode, g_CdCommandNames[g_CdLastCommand], CD_STATUS_WORD, CD_ERROR_WORD);
        }
    }

    switch (mode) {
    case 3:
        if (flag) {
            volatile u_char *sp = &g_CdSyncStatus.sync;
            *sp = 5;
            copy8(g_CdSyncResult, (u_char *)buf);
            return 2;
        }
        if (g_CdCommandHasComplete[g_CdLastCommand] != 0) {
            volatile u_char *sp = &g_CdSyncStatus.sync;
            *sp = 3;
            copy8(g_CdSyncResult, (u_char *)buf);
            return 1;
        }
        {
            volatile u_char *sp = &g_CdSyncStatus.sync;
            *sp = 2;
        }
        copy8(g_CdSyncResult, (u_char *)buf);
        return 2;
    case 2:
        g_CdSyncStatus.sync = flag ? 5 : 2;
        copy8(g_CdSyncResult, (u_char *)buf);
        return 2;
    case 1:
        if (flag) {
            if (i == 1) {
                flag = 0;
            }
        }
        g_CdSyncStatus.ready = flag ? 5 : 1;
        copy8(g_CdReadyResult, (u_char *)buf);
        *g_CdReg0 = 0;
        *g_CdReg3 = 0;
        return 4;
    case 4: {
        volatile u_char *sp = &g_CdSyncStatus.ready;
        g_CdDataEndStatus = 4;
        *sp = g_CdDataEndStatus;
        copy8(g_CdDataEndResult, (u_char *)buf);
        copy8(g_CdReadyResult, (u_char *)buf);
        return 4;
    }
    case 5: {
        volatile u_char *sp = &g_CdSyncStatus.sync;
        g_CdSyncStatus.ready = 5;
        *sp = g_CdSyncStatus.ready;
        copy8(g_CdSyncResult, (u_char *)buf);
        copy8(g_CdReadyResult, (u_char *)buf);
        return 6;
    }
    default:
        puts(&g_MsgCdUnknownIntr);
        printf(&g_MsgCdUnknownIntrCode, mode);
        break;
    }
    return 0;
}
