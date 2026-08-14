#include "common.h"
#include "psyq/cd.h"
#include "psyq/cd_internal.h"


long CD_getsector2(long madr, u_long size) {
    volatile u_char *status;
    volatile u_long *cdDpcr;
    volatile u_long *cdDmaChcr;

    *g_CdReg0 = 0;
    *g_CdReg3 = 0x80;
    *g_CdromDelayReg = 0x20943;
    *g_ComDelayReg = 0x1323;

    cdDpcr = g_CdDpcr;
    *cdDpcr |= 0x8000;

    *g_CdDmaMadr = madr;
    *g_CdDmaBcr = size | 0x10000;

    status = g_CdReg0;
    while ((*status & 0x40) == 0) {
    }

    *g_CdDmaChcr = 0x11000000;

    cdDmaChcr = g_CdDmaChcr;
    if ((*cdDmaChcr & 0x1000000) != 0) {
        cdDpcr = cdDmaChcr;
        do {
        } while ((*cdDpcr & 0x1000000) != 0);
    }

    *g_ComDelayReg = 0x1325;
    return 0;
}

void CdSetSectorParam(long words) {
    g_CdTestParamCount = words;
}

void CdDispatchInterrupts(void) {
    u_char *statusByte;
    long status;
    long saved;

    saved = *g_CdReg0 % 4;
    statusByte = &g_CdSyncStatus.ready;

    while ((status = CdReadInterruptStatus()) != 0) {
        if ((status & 4) != 0) {
            if (g_CdReadyCallback != 0) {
                g_CdReadyCallback(*statusByte, g_CdReadyResult);
            }
        }

        if ((status & 2) != 0) {
            CdCallback doneCallback;
            u_char *resultByte;

            doneCallback = g_CdSyncCallback;
            if (doneCallback != 0) {
                resultByte = &g_CdSyncStatus;
                asm volatile("" : "=r"(resultByte) : "0"(resultByte));
                doneCallback(*resultByte, g_CdSyncResult);
            }
        }
    }

    *g_CdReg0 = saved;
}
