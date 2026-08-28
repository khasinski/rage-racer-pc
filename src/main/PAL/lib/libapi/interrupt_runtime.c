#include "psyq/kernel.h"
#include <stdio.h>

void *StartKernelInterrupts(void) {
    u_short *state;

    state = g_IntrState;
    if (state[0] != 0) {
        return 0;
    }

    HookEntryInt(&state[0x1C]);
    {
        volatile u_short *mask = g_IrqMask;
        u_short pending = g_IntrSavedIrqMask;

        state[0] = 1;
        *mask = pending;
    }
    *g_KernelDpcr = g_IntrSavedDpcr;
    ExitCriticalSection();

    return state;
}

void clearKernelInterruptState(u_long *dst, long count) {
    volatile long unused;
    long i = count - 1;

    if (count != 0) {
        do {
            *dst = 0;
            i--;
            dst++;
        } while (i != -1);
    }
}

void *startIntrVSync(void) {
    *g_Timer1ModeReg = 0x107;
    g_VSyncCount = 0;
    clearIntrVSyncCallbacks((u_long *)g_VSyncCallbacks, 8);
    KernelCallbackSlot2(0, intrVSyncDispatcher);

    return setIntrVSync;
}

void intrVSyncDispatcher(void) {
    long i;
    void (**callback)(void);
    void (*func)(void);
    long count;

    count = g_VSyncCount;
    i = 0;
    callback = (void (**)(void))g_VSyncCallbacks;
    g_VSyncCount = count + 1;
    count = g_VSyncCount;
    for (; i < 8; i++) {
        func = *callback++;
        if (func != 0) {
            func();
        }
    }
}

void setIntrVSync(long index, void *callback) {
    void **base;
    void **slot;

    base = g_VSyncCallbacks;
    slot = &base[index];
    if (callback != *slot) {
        *slot = callback;
    }
}

void clearIntrVSyncCallbacks(u_long *dst, long count) {
    volatile long unused;
    long i = count - 1;

    if (count != 0) {
        do {
            *dst = 0;
            i--;
            dst++;
        } while (i != -1);
    }
}

void *startIntrDMA(void) {
    clearIntrDMACallbacks(g_DmaCallbacks, 8);
    *g_DmaIrqControl = 0;
    KernelCallbackSlot2(3, intrDMADispatcher);

    return setIntrDMA;
}

void intrDMADispatcher(void) {
    u_long pending;
    u_long pendingTemp;
    long i;
    void (**handler)(void);
    u_long lowMask;
    u_long one;
    void (**handlerBase)(void);
    u_char *fmt;

    pendingTemp = *g_DmaIrqControl;
    pending = (pendingTemp >> 0x18) & 0x7F;
    if (pending != 0) {
        one = 1;
        lowMask = 0xFFFFFF;
        handlerBase = (void (**)(void))g_DmaCallbacks;
        do {
            i = 0;
            if (pending != 0) {
                handler = handlerBase;
                while ((pending != 0) && (i < 7)) {
                    if (pending & 1) {
                        volatile u_long *bits;
                        u_long value;
                        long shift;

                        bits = g_DmaIrqControl;
                        shift = i + 0x18;
                        value = one << shift;
                        value |= lowMask;
                        value &= *bits;
                        *bits = value;
                        if (*handler != 0) {
                            (*handler)();
                        }
                    }
                    handler++;
                    pending >>= 1;
                    i++;
                }
            }

            pendingTemp = *g_DmaIrqControl;
            pending = (pendingTemp >> 0x18) & 0x7F;
        } while (pending != 0);
    }

    if (((*g_DmaIrqControl & 0xFF000000) == 0x80000000) || ((*g_DmaIrqControl & 0x8000) != 0)) {
        fmt = g_MsgDmaBusError;
        printf(fmt, *g_DmaIrqControl);
        for (i = 0; i < 7; i++) {
            printf(g_FmtDmaMadr, i, g_DmaChannelRegs[i * 4]);
        }
    }
}

u_long setIntrDMA(long channel, u_long handler) {
    long index;
    u_long callback;
    u_long *base;
    long offset;
    u_long *slot;
    u_long oldCallback;

    index = channel;
    base = g_DmaCallbacks;
    offset = index << 2;
    slot = (u_long *)((long)base + offset);
    oldCallback = *slot;
    callback = handler;

    if (callback != oldCallback) {
        if (callback != 0) {
            volatile u_long *bits = g_DmaIrqControl;
            u_long value;
            register long shift asm("$3");
            register u_long mask asm("$2") = 0xFFFFFF;

            *slot = callback;
            value = *bits;
            shift = index + 0x10;
            value &= mask;
            mask = 1;
            mask <<= shift;
            shift = 0x800000;
            mask |= shift;
            value |= mask;
            *bits = value;
        } else {
            register volatile u_long *bits asm("$5") = g_DmaIrqControl;
            u_long value;
            long shift;
            register u_long mask asm("$2") = 0xFFFFFF;

            *slot = callback;
            value = *bits;
            shift = index + 0x10;
            value &= mask;
            mask = 0x800000;
            value |= mask;
            mask = 1;
            mask <<= shift;
            mask = ~mask;
            value &= mask;
            *bits = value;
        }
    }

    return oldCallback;
}

void clearIntrDMACallbacks(u_long *dst, long count) {
    volatile long unused;
    long i = count - 1;

    if (count != 0) {
        do {
            *dst = 0;
            i--;
            dst++;
        } while (i != -1);
    }
}

long SetDMAInterruptState(long state) {
    long value;

    value = g_DmaInterruptState;
    g_DmaInterruptState = state;
    return value;
}

long GetDMAInterruptState(void) {
    return g_DmaInterruptState;
}
