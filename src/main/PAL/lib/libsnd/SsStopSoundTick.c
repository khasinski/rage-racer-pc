#include "common.h"
#include "psyq/kernel.h"
#include "psyq/snd.h"

#include "psyq/snd_internal.h"


void SsStopSoundTick(void) {
    if (g_SndNoTickFlag == 0) {
        g_SndTickHalfRate = 0;
        EnterCriticalSection();

        if (g_SndTickUsesVSync != 0) {
            VSyncCallback(0);
            g_SndTickUsesVSync = 0;
        } else if (g_SndTickIrq == 0) {
            KernelCallbackSlot2(0, g_SndPrevVSyncCallback);
            g_SndPrevVSyncCallback = 0;
        } else {
            KernelCallbackSlot2(6, 0);
        }

        ExitCriticalSection();
        g_SndTickIrq = 0xFF;
    }
}
