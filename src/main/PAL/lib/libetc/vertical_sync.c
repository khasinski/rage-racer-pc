#include "psyq/kernel.h"
#include <stdio.h>

typedef void (*Callback2)(long, long);

typedef struct CallbackTable {
    u_char pad0[0xC];
    void (*callback)(void);
} CallbackTable;

long VSync(long mode) {
    long oldTimer;
    long delta;
    long waitTarget;
    long waitCount;
    long one;
    volatile long *timer;

    oldTimer = *g_VSyncGpuStat;
    delta = (u_short)(*g_Timer1CountReg - g_VSyncTimerBase);

    if (mode < 0) {
        return g_VSyncCount;
    }

    if (mode == 1) {
        return delta;
    }

    one = 1;
    waitTarget = mode > 0 ? g_VSyncCountBase - one + mode : g_VSyncCountBase;
    waitCount = mode > 0 ? mode - one : 0;
    waitVSync(waitTarget, waitCount);

    {
        volatile long *timer2;
        long waitBase;

        timer2 = g_VSyncGpuStat;
        oldTimer = *timer2;
        waitBase = g_VSyncCount;
        waitVSync(waitBase + 1, 1);
    }

    if (oldTimer & 0x80000) {
        timer = g_VSyncGpuStat;
        if (!((oldTimer ^ *timer) < 0)) {
            do {
            } while (((oldTimer ^ *timer) & 0x80000000) == 0);
        }
    }

    g_VSyncCountBase = g_VSyncCount;
    g_VSyncTimerBase = *g_Timer1CountReg;
    return delta;
}

void waitVSync(long target, long timeoutFrames) {
    volatile long timeout;

    timeout = timeoutFrames << 15;
    if (g_VSyncCount < target) {
        do {
            if (--timeout == -1) {
                puts(g_MsgVSyncTimeout);
                ChangeClearRCnt(0);
                ChangeClearInterruptMask(3, 0);
                break;
            }
        } while (g_VSyncCount < target);
    }
}

void KernelCallbackSlot3(void) {
    ((CallbackTable *)g_IntrRpNode)->callback();
}

long KernelCallbackSlot2(void) {
    return ((long (*)())g_IntrRpNode[2])();
}

void DMACallback(long spec, long callback) {
    g_IntrRpNode[1]();
}

void VSyncCallback(long count) {
    ((Callback2)g_IntrRpNode[5])(0, count);
}

void KernelCallbackSlot5(void) {
    g_IntrRpNode[5]();
}

void KernelCallbackSlot4(void) {
    g_IntrRpNode[4]();
}

void KernelCallbackSlot6(void) {
    g_IntrRpNode[6]();
}

long GetKernelStatus(void) {
    return g_IntrInDispatch;
}

long GetIntrMask(void) {
    return *g_IrqMask;
}

long SetIntrMask(long mask) {
    u_short value;
    volatile u_short *ptr;

    ptr = g_IrqMask;
    value = *ptr;
    *ptr = mask;
    return value;
}
