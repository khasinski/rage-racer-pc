#include "common.h"
#include <stdio.h>
#include "psyq/cd.h"
#include "psyq/cd_internal.h"

#define CD_STATUS_WORD (*(long *)(void *)&g_CdStatusByte)
#define CD_ERROR_WORD (*(long *)(void *)&g_CdErrorByte)


long CD_vol(CdlATV *vol) {
    *g_CdReg0 = 2;
    *g_CdReg2 = vol->val0;
    *g_CdReg3 = vol->val1;
    *g_CdReg0 = 3;
    *g_CdReg1 = vol->val2;
    *g_CdReg2 = vol->val3;
    *g_CdReg3 = 0x20;
    return 0;
}

void CD_flush(void) {
    volatile u_char *state;
    volatile u_char *reg;

    *g_CdReg0 = 1;

    if ((*g_CdReg3 % 8) != 0) {
        do {
            *g_CdReg0 = 1;
            *g_CdReg3 = 7;
            *g_CdReg2 = 7;
        } while ((*g_CdReg3 % 8) != 0);
    }

    state = &g_CdSyncStatus.ready;
    g_CdSyncStatus.command = 0;
    *state = g_CdSyncStatus.command;
    reg = g_CdReg0;
    g_CdSyncStatus.sync = 2;
    *reg = 0;
    *g_CdReg3 = 0;
    *g_ComDelayReg = 0x1325;
}

long CD_initvol(void) {
    CdRegisterMap *cdSpuRegs;
    u_char sp0[4];

    cdSpuRegs = g_CdSpuRegs;
    if (cdSpuRegs->status_mode_a == 0 && cdSpuRegs->status_mode_b == 0) {
        cdSpuRegs->cd_left_volume = 0x3FFF;
        cdSpuRegs->cd_right_volume = 0x3FFF;
        cdSpuRegs = g_CdSpuRegs;
    }

    cdSpuRegs->output_left_volume = 0x3FFF;
    cdSpuRegs->output_right_volume = 0x3FFF;
    cdSpuRegs->audio_control = 0xC001;

    sp0[2] = 0x80;
    sp0[0] = 0x80;
    sp0[3] = 0;
    sp0[1] = 0;

    *g_CdReg0 = 2;
    *g_CdReg2 = sp0[0];
    *g_CdReg3 = sp0[1];
    *g_CdReg0 = 3;
    *g_CdReg1 = sp0[2];
    *g_CdReg2 = sp0[3];
    *g_CdReg3 = 0x20;

    return 0;
}

void CD_initintr(void) {
    g_CdReadyCallback = 0;
    g_CdSyncCallback = 0;
    CD_ERROR_WORD = 0;
    CD_STATUS_WORD = 0;
    KernelCallbackSlot3();
    KernelCallbackSlot2(2, (void *)CdDispatchInterrupts);
}

long CdResetState(void) {
    puts(g_MsgCdInit);
    printf(g_MsgCdInitAddr, g_CdDebugInfo);

    g_CdLastCommand = 0;
    g_CdModeByte = 0;
    g_CdReadyCallback = 0;
    g_CdSyncCallback = 0;
    CD_ERROR_WORD = 0;
    CD_STATUS_WORD = 0;
    KernelCallbackSlot3();
    KernelCallbackSlot2(2, CdDispatchInterrupts);

    *g_CdReg0 = 1;
    while ((*g_CdReg3 % 8) != 0) {
        *g_CdReg0 = 1;
        *g_CdReg3 = 7;
        *g_CdReg2 = 7;
    }

    g_CdSyncStatus.ready = g_CdSyncStatus.command = 0;
    g_CdSyncStatus.sync = 2;
    *g_CdReg0 = 0;
    *g_CdReg3 = 0;
    *g_ComDelayReg = 0x1325;

    CD_cw(1, 0, 0, 0);
    if ((CD_STATUS_WORD & 0x10) != 0) {
        CD_cw(1, 0, 0, 0);
    }

    if (CD_cw(0xA, 0, 0, 0) != 0) {
        return -1;
    }
    if (CD_cw(0xC, 0, 0, 0) != 0) {
        return -1;
    }
    if (CD_sync(0, 0) != 2) {
        return -1;
    }
    return 0;
}
