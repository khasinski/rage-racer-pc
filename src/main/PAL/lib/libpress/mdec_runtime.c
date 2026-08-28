#include <sys/types.h>
#include <stdio.h>

#include "common.h"
#include "psyq/cd.h"
#include "psyq/press_internal.h"


void MDEC_reset(long mode) {
    register long option asm("$5") = mode;
    register long zero asm("$0");
    volatile u_long *inBuffer = (volatile u_long *)g_MdecQuantCmd;

    switch (option) {
    case 0:
    *g_MdecCtrlReg = 0x80000000;
    *g_MdecInDmaChcr = zero;
    *g_MdecOutDmaChcr = zero;
    *g_MdecCtrlReg = 0x60000000;
    MDEC_in(inBuffer, 0x20);
    MDEC_in((volatile u_long *)g_MdecIdctCmd, 0x20);
    return;

    case 1:
    *g_MdecCtrlReg = 0x80000000;
    *g_MdecInDmaChcr = 0;
    *g_MdecOutDmaChcr = 0;
    *g_MdecOutDmaChcr;
    *g_MdecCtrlReg = 0x60000000;
    return;

    }
    printf(g_FmtMdecBadOption);
}

void MDEC_in(volatile u_long *buf, long words) {
    MDEC_in_sync();
    *g_MdecDpcr |= 0x88;
    *g_MdecInDmaMadr = (u_long)(buf + 1);
    *g_MdecInDmaBcr = ((u_long)words >> 5 << 16) | 0x20;
    *g_MdecCmdReg = *buf;
    *g_MdecInDmaChcr = 0x01000201;
}

void MDEC_out(volatile u_long *buf, long words) {
    MDEC_out_sync();
    *g_MdecDpcr |= 0x88;
    *g_MdecOutDmaChcr = 0;
    *g_MdecOutDmaMadr = (u_long)buf;
    *g_MdecOutDmaBcr = ((u_long)words >> 5 << 16) | 0x20;
    *g_MdecOutDmaChcr = 0x01000200;
}

long MDEC_in_sync(void) {
    volatile long timeout;

    timeout = 0x100000;
    if (*g_MdecCtrlReg & 0x20000000) {
        do {
            if (--timeout == -1) {
                MDEC_timeout(g_MsgMdecInSync);
                return -1;
            }
        } while (*g_MdecCtrlReg & 0x20000000);
    }
    return 0;
}

long MDEC_out_sync(void) {
    volatile long timeout;

    timeout = 0x100000;
    if (*g_MdecOutDmaChcr & 0x01000000) {
        do {
            if (--timeout == -1) {
                MDEC_timeout(g_MsgMdecOutSync);
                return -1;
            }
        } while (*g_MdecOutDmaChcr & 0x01000000);
    }
    return 0;
}

long MDEC_timeout(u_char *name) {
    u_long status;
    register long ret;

    printf(g_FmtMdecTimeout, name);
    status = *g_MdecCtrlReg;
    printf(g_FmtMdecTimeoutDma, (*g_MdecInDmaChcr >> 24) & 1, (*g_MdecOutDmaChcr >> 24) & 1, *g_MdecInDmaMadr, *g_MdecOutDmaMadr);
    printf(g_FmtMdecTimeoutStatus,
                  (~status >> 31) & 1,
                  (status >> 30) & 1,
                  (status >> 29) & 1,
                  (status >> 28) & 1,
                  (status >> 27) & 1,
                  (status >> 25) & 1,
                  (status >> 23) & 1);

    *g_MdecCtrlReg = 0x80000000;
    *g_MdecInDmaChcr = 0;
    *g_MdecOutDmaChcr = 0;

    ret = 0;
    *g_MdecOutDmaChcr;
    *g_MdecCtrlReg = 0x60000000;

    return ret;
}
