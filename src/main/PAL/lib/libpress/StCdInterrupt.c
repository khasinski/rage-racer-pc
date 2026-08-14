#include "common.h"
#include "psyq/cd.h"
#include "psyq/kernel.h"
#include "psyq/press_internal.h"

typedef struct StReadyStatus {
    u16 reserved;
    u16 status;
    u16 cause;
} StReadyStatus;


void StCdInterrupt(void) {
    volatile StReadyStatus readyStatus;
    CdlLOC location;
    u8 readyResult[8];
    u8 *resultCursor;
    u32 i;
    u16 frameCount;
    u32 dmaControl;
    u16 headerState;
    u16 expectedState;
    u32 endFrame;

    if (g_StDmaBusy == 1) {
        return;
    }

    if (g_StColorMode != 0 && (*g_MdecOutDmaControl & 0x01000000)) {
        g_StInterruptPending = 1;
        if (g_StCopySource != 0) {
            g_StCopySector++;
        }
        g_StInterruptState = 1;
        return;
    }

    if (CdReady(1, readyResult) == 5) {
        return;
    }

    readyStatus.status = readyResult[0];
    readyStatus.cause = readyResult[1];
    if (readyStatus.status & 4) {
        g_StInterruptState = 3;
        return;
    }

    g_StActiveHeader =
        (StStrHeader *)&g_StRingBase[g_StWriteCursor];
    if (*(volatile u16 *)g_StActiveHeader != 0) {
        if (g_StCopySource != 0) {
            g_StCopySector++;
        }
        g_StInterruptState = 4;
        return;
    }

    *g_StCdReg0 = 0;
    *g_StCdReg3 = 0;
    *g_StCdReg0 = 0;
    *g_StCdReg3 = 0x80;
    *g_InterruptStatus = 0x20943;
    *g_InterruptMask = 0x1323;

    if (g_StNotStream2Mode == 0) {
        resultCursor = (u8 *)&location;
        do {
            *resultCursor++ = *g_StCdReg2;
        } while (resultCursor < (u8 *)(&location + 1));

        i = 0;
        do {
            *g_StCdReg2;
            i++;
        } while (i < 8);
    }

    dmaControl = 0x11000000;
    if (g_StCopySource != 0) {
        copyKernelWords(
            (u_long *)g_StActiveHeader,
            (u_long *)(g_StCopySource + (g_StCopySector << 11)),
            8,
            0);
    } else {
        CD_dmastart(
            3,
            (u32)g_StActiveHeader,
            0,
            8,
            dmaControl,
            0,
            0);
    }

    while (*g_CdDmaControl & 0x01000000) {
    }

    ((StStrHeader *)g_StActiveHeader)->loc = location;
    *g_InterruptStatus = 0x20843;
    *g_InterruptMask = 0x1325;

    if (g_StStreamFlag == 1 && g_StStartFrame != 0) {
        if (g_StStartFrame != g_StActiveHeader->nFrames) {
            *(u16 *)g_StActiveHeader = 0;
            if (g_StCopySource != 0) {
                g_StCopySector++;
            }
            return;
        }
        g_StStreamFlag = 0;
    }

    headerState = *(volatile u16 *)g_StActiveHeader;
    expectedState = 0x160;
    if (headerState != expectedState ||
        ((g_StActiveHeader->mode >> 10) & 0x1F) != g_StCurrentChannel) {
        if (g_StCopySource != 0) {
            g_StCopySector = 0;
        } else {
            /* Preserve the SDK's second observation of shared ring state. */
            *(volatile u16 *)g_StActiveHeader;
        }
        g_StInterruptState = 5;
        *(volatile u16 *)g_StActiveHeader = 0;
        return;
    }

    if (g_StCurrentSector != g_StActiveHeader->frame ||
        (g_StCurrentFrameCount != 0 &&
         g_StCurrentFrameCount != g_StActiveHeader->nFrames)) {
        g_StCurrentFrameCount = 0;
        g_StCurrentSector = 0;
        StClearRingRange(
            g_StReadCursor, g_StWriteCursor - g_StReadCursor);
        g_StWriteCursor = g_StReadCursor;
        *(u16 *)g_StActiveHeader = 0;
        if (g_StCopySource != 0) {
            g_StCopySector++;
        }
        g_StInterruptState = 6;
        return;
    }

    if (g_StActiveHeader->frame == 0) {
        frameCount = g_StActiveHeader->nFrames;
        endFrame = g_StEndFrame;
        g_StCurrentSector = 0;
        g_StCurrentFrameCount = frameCount;

        if (endFrame != 0 && frameCount >= endFrame) {
            g_StCurrentFrameCount = 0;
            g_StCurrentSector = 0;
            StClearRingRange(
                g_StReadCursor, g_StWriteCursor - g_StReadCursor);
            g_StWriteCursor = g_StReadCursor;
            *(u16 *)g_StActiveHeader = 0;
            g_StStreamFlag = 1;
            if (g_StEndCallback != 0) {
                g_StEndCallback();
            }
            if (g_StCopySource != 0) {
                g_StCopySector++;
            }
            g_StInterruptState = 7;
            return;
        }

        if ((u32)(g_StRingSize - g_StWriteCursor - 1) <
            g_StActiveHeader->nSectors) {
            if (g_StEndFrame == 0) {
                *(u16 *)g_StActiveHeader = 1;
                g_StStreamFlag = 1;
                if (g_StEndCallback != 0) {
                    g_StEndCallback();
                }
                if (g_StCopySource != 0) {
                    g_StCopySector++;
                }
                g_StInterruptState = 8;
                return;
            }

            if (*(s16 *)g_StRingBase != 0) {
                *(u16 *)g_StActiveHeader = 0;
                if (g_StCopySource != 0) {
                    g_StCopySector++;
                }
                g_StInterruptState = 9;
                return;
            }

            *(u16 *)g_StActiveHeader = 1;
            g_StWriteCursor = 0;
            i = 0;
            do {
                ((u32 *)g_StRingBase)[i] =
                    ((u32 *)g_StActiveHeader)[i];
                i++;
            } while (i < 8);
            g_StActiveHeader = (StStrHeader *)g_StRingBase;
        }

        g_StReadCursor = g_StWriteCursor;
    }

    g_StInterruptState = 10;
    g_StCurrentSector++;
    g_StSectorData =
        (u8 *)g_StRingBase +
        (g_StRingSize * 32) +
        g_StWriteCursor * 0x7E0;

    dmaControl = 0x11000000;
    if (g_StColorMode != 0) {
        *g_InterruptStatus = 0x20943;
        *g_InterruptMask = 0x1323;
    } else {
        *g_InterruptStatus = 0x21020843;
        dmaControl = 0x11400100;
    }

    if (g_StActiveHeader->nSectors - 1 ==
        g_StActiveHeader->frame) {
        g_StDmaBusy = 1;
        if (g_StCopySource != 0) {
            copyKernelWords(
                (u_long *)g_StSectorData,
                (u_long *)(g_StCopySource +
                           (g_StCopySector << 11) + 0x20),
                0x1F8,
                1);
            g_StCopySector++;
        } else {
            CD_dmastart(
                3,
                (u32)g_StSectorData,
                0,
                0x1F8,
                dmaControl,
                1,
                0);
        }
        g_StCurrentSector = 0;
        g_StCurrentFrameCount = 0;
        g_StCurrentChannel = g_StNextChannel;
    } else if (g_StCopySource != 0) {
        copyKernelWords(
            (u_long *)g_StSectorData,
            (u_long *)(g_StCopySource +
                       (g_StCopySector << 11) + 0x20),
            0x1F8,
            0);
        g_StCopySector++;
    } else {
        CD_dmastart(
            3,
            (u32)g_StSectorData,
            0,
            0x1F8,
            dmaControl,
            0,
            0);
    }

    *g_InterruptMask = 0x1325;
    *(u16 *)g_StActiveHeader = 3;
    g_StWriteCursor++;

    if (g_StCopySource != 0 && g_StDmaBusy != 0) {
        data_ready_callback();
    }
}
