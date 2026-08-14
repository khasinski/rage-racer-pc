#ifndef RAGE_PC_PSYQ_KERNEL_H
#define RAGE_PC_PSYQ_KERNEL_H

#include <sys/types.h>

#include "common.h"

void _card_info(s32 port);
s32 _card_load(s32 port);

typedef void (*KernelCallback)(void);

typedef struct RootCounter {
    u_short count;
    short pad2;
    short mode;
    short pad6;
    short target;
    long padC;
} RootCounter;

void copyKernelWords(u_long *dst, u_long *src, u_long count, long unused);
long VSync(long mode);
void waitVSync(long target, long timeoutFrames);
void ChangeClearRCnt(long clear);
void ChangeClearInterruptMask(long index, long clear);
void KernelCallbackSlot3(void);
long KernelCallbackSlot2();
/* BIOS DMA callback installer: spec 0=MDECin 1=MDECout 2=GPU 3=CD-ROM 4=SPU.
 * Was declared ResetCallback here; that was wrong. */
void DMACallback(long spec, long callback);
/* Callers hand it a Callback, the body takes a long; an empty parameter
 * list lets both spellings stand. */
void VSyncCallback();
void KernelCallbackSlot5(void);
void KernelCallbackSlot4(void);
void KernelCallbackSlot6(void);
long GetKernelStatus(void);
long GetIntrMask(void);
long SetIntrMask(long mask);

void *InitKernelInterrupts(void);
void intrDispatch(void);
KernelCallback SetKernelInterruptCallback(long index, KernelCallback callback);
void *StartKernelInterrupts(void);
void clearKernelInterruptState(u_long *dst, long count);
void SysEnqIntRP(void *rp);
void ReturnFromException(void);
void ResetEntryInt(void);
void HookEntryInt(void *entry);
long SaveKernelRegisters(void *state);
void RestoreKernelRegisters(void *state, long ret);

void *startIntrVSync(void);
void intrVSyncDispatcher(void);
void setIntrVSync(long index, void *callback);
void clearIntrVSyncCallbacks(u_long *dst, long count);
void *startIntrDMA(void);
void intrDMADispatcher(void);
u_long setIntrDMA(long index, u_long callback);
void clearIntrDMACallbacks(u_long *dst, long count);
long SetDMAInterruptState(long value);
long GetDMAInterruptState(void);

/* LibRef47 narrows `target` to unsigned short; kept long here because narrowing
 * it changes the truncation gcc emits at the call sites. */
long SetRCnt(long spec, long target, long mode);
long GetRCnt(long spec);
long StartRCnt(long spec);
long StopRCnt(long spec);
long ResetRCnt(long spec);
void EnterCriticalSection(void);
void ExitCriticalSection(void);
long OpenEvent(long desc, long spec, long mode, long func);
void CloseEvent(long event);
long TestEvent(long event);
void EnableEvent(long event);
void DisableEvent(long event);
void WaitEvent(long event);
void DeliverEvent(long event, long spec);

long BiosFileOpen(void *path, long mode);
long BiosFileSeek(long fd, long offset, long whence);
long BiosFileRead(long fd, void *buf, long len);
long BiosFileWrite(long fd, void *buf, long len);
long BiosFileClose(long fd);
long BiosFormatDevice(void *device);
void *BiosFirstFile(char *path, void *entry);
void *BiosNextFile(void *entry);

/* Declared identically by 26 translation units before this
 * header carried them. */

extern u_short g_IntrCallbackMask;
extern u_short g_IntrInDispatch;
extern u_long g_IntrSavedDpcr;
extern u_short g_IntrSavedIrqMask;
extern u_short g_IntrState[];
extern volatile u_short *g_IrqMask;
extern volatile u_short *g_IrqStatus;
extern volatile u_long *g_KernelDpcr;
extern volatile long g_VSyncCount;

/* Declared identically by 20 translation units before this
 * header carried them. */

extern char g_MsgVSyncTimeout[];
extern u_char g_MsgUnexpectedInterrupt[];
extern u_char g_MsgIntrTimeout[];
extern u_char g_MsgDmaBusError[];
extern u_char g_FmtDmaMadr[];
extern u_short g_IntrJmpBufSp[];
extern KernelCallback g_IntrCallbacks[];
extern KernelCallback *g_IntrRpNode;
extern long g_DmaInterruptState;
extern u_long g_DmaCallbacks[];
extern u_long *g_DmaChannelRegs;
extern volatile u_long *g_DmaIrqControl;
extern long g_IntrStuckCount;
extern volatile u_long *g_IrqRegs;
extern u_long g_RootCounterIrqBits[];
extern volatile RootCounter *g_RootCounterRegs;
extern volatile long *g_Timer1CountReg;
extern u_long *g_Timer1ModeReg;
extern void *g_VSyncCallbacks[];
extern long g_VSyncCountBase;
extern volatile long *g_VSyncGpuStat;
typedef struct DirEntry {
    char name[20];
    s32 attributes;
    s32 size;
    struct DirEntry *next;
    char system[8];
} DirEntry;

extern volatile long g_VSyncTimerBase;

#endif
