#include <sys/types.h>
#include <stdio.h>

#include "common.h"
#include "psyq/cd_internal.h"


/*
 * Low-level DMA-channel transfer helper. Spins on the channel's CHCR busy bit
 * (register window at 0x1F801088 + ch*0x10, timing out after 0x10000 polls),
 * then programs MADR/BCR/CHCR to start a transfer of `count` blocks of `size`
 * words. `mode` selects between the block-mode / linked-list setups.
 */
void CD_dmastart(
    s32 ch,
    u32 madr,
    u32 count,
    u32 size,
    u32 chcrVal,
    u8 mode,
    u32 unused) {
    volatile long dummy;
    long i;
    volatile u_long *p;
    u_char *dptr;
    volatile u_long *dp;
    register long bv asm("$4");

    i = 0;
    while (*(volatile u_long *)(0x1F801088 + (ch << 4)) & 0x01000000) {
        if (i == 0x10000) {
            printf(g_MsgDmaStatusError, *(volatile u_long *)(0x1F801088 + (ch << 4)));
            break;
        }
        i++;
    }

    if (mode == 1) {
        dptr = g_DmaDicr;
        bv = dptr[2];
        dptr[2] = bv | (1 << ch);
    } else {
        dptr = g_DmaDicr;
        bv = dptr[2];
        dptr[2] = bv & ~(1 << ch);
    }

    dummy = *(volatile u_long *)g_DmaDicr;
    asm volatile("");
    {
        register long dv asm("$6");
        long bit;

        dv = ch * 4;
        bit = 1 << (dv + 3);
        asm volatile("");
        p = (volatile u_long *)(0x1F801080 + (ch << 4));
        dp = g_DmaDpcr;
        dv = *dp;
        *dp = dv | bit;
        *p++ = madr;
        *p++ = (count << 16) | size;
        while (!(*D_800993D0 & 0x40)) {
        }
        *p = chcrVal;
        dummy = *p;
    }
}
