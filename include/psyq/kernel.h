#ifndef RAGE_PC_PSYQ_KERNEL_H
#define RAGE_PC_PSYQ_KERNEL_H

#include <sys/types.h>

#include "common.h"

void _card_info(s32 port);
s32 _card_load(s32 port);

typedef void (*KernelCallback)(void);

void copyKernelWords(u_long *dst, u_long *src, u_long count, long unused);
long VSync(long mode);
void waitVSync(long target, long timeoutFrames);
void ChangeClearRCnt(long clear);
void ChangeClearInterruptMask(long index, long clear);
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

typedef struct DirEntry {
    char name[20];
    s32 attributes;
    s32 size;
    struct DirEntry *next;
    char system[8];
} DirEntry;

#endif
