#ifndef PSYQ_CD_H
#define PSYQ_CD_H

#include <sys/types.h>

#include "common.h"
#include "psyq/cd_types.h"

/* libcd's polling deadline: a wall-clock limit, the retries left, and the name
 * the timeout message prints. Was duplicated in four CD_*.c files. */
/* Returns the human-readable name for CD command `cmd`, or a default string if
 * out of range. */
char *CdComstr(long cmd);
/* Returns the human-readable name for CD interrupt code `intr`. */
char *CdIntstr(long intr);
long CdSetDebug(long level);
void CdFlush(void);
long CdInit(void);
long CdStatus(void);
u_char CdMode(void);
u_char CdLastCom(void);
/* Returns the last reported disc position. */
CdlLOC *CdLastPos(void);
CdlLOC *CdIntToPos(long i, CdlLOC *p);
long CdPosToInt_Local(CdlLOC *loc);
long CdGetToc(CdlLOC *toc);
long CdGetToc2(long maxTracks, u_char *out);
void CD_initintr(void);
long CD_initvol(void);
void CD_flush(void);
long CD_getsector2(long madr, u_long size);
long CD_vol(CdlATV *vol);
CdlFILE *DsSearchFile(CdlFILE *file, char *name);
long DS_searchdir(long type, u_char *name);
void StClearRing(void);
long StGetBackloc(CdlLOC *loc);
/* LibRef47 spells these `u_long *ring_addr, u_long ring_size` and StSetStream's
 * last two arguments as function pointers; kept as-is to match the call sites. */
void StSetRing(void *base, long size);
void StSetStream(long mode, long start_frame, long end_frame, long callback, long user_data);
u_long StFreeRing(u_long *base);
/* The libds streaming state machine: advances g_StInterruptState through states
 * 1..0xA, DMAs the sector header then body and drives the StStrHeader ring. */
void StCdInterrupt(void);

/*
 * libcd command interface. All three share the same retry-3 body over CD_cw;
 * CdControlB additionally waits on CD_sync (blocking), CdControlF sends the
 * command without collecting a result.
 */
long CdControl(long com, void *param, u_char *result);
long CdControlF(long com, void *param);
long CdControlB(long com, void *param, void *result);
long CdSync(long mode, u_char *result);
long CdReady(long mode, u_char *result);
/* Install a completion / data-ready callback; returns the previous one. */
long CdSyncCallback(long callback);
long CdReadyCallback(long callback);

/*
 * libcd internals. CD_init resets the drive (CD_initvol + CD_initintr + the
 * register-level reset CdResetState) and is what CdInit retries up to 5 times.
 * CdDispatchInterrupts is the IRQ2 handler installed by that reset; it drains
 * the interrupt status via CdReadInterruptStatus and fans out to the sync/ready
 * callbacks.
 */
/*
 * libcd's cdread.c, linked into the game's own .text range instead of the
 * 0x80063200+ SDK block - identified by its three surviving messages
 * "CdRead: sector error" / "CdRead: Shell open..." / "CdRead: retry...".
 * CdRead arms a multi-sector transfer and returns immediately; CdReadSync
 * polls it (mode 0 blocks, non-zero returns the sectors still outstanding).
 * The two lower entries are cdread.c's own statics, named descriptively here.
 */
/*
 * Sector geometry. A Mode 2 Form 1 data sector carries CD_SECTOR_SIZE bytes of
 * user data, so a byte count becomes a sector count as
 * `(n + CD_SECTOR_MASK) >> 11`, which is what LoadAsset does to the size it
 * read out of the RAGE.BIN index.
 */
#define CD_SECTOR_SIZE 0x800
#define CD_SECTOR_MASK 0x7FF

/*
 * CdlSetmode bit 7, double speed - the `mode` every CdRead call site in this
 * tree passes. It is not a sector-length selector: CdRead itself decodes those
 * with `mode & 0x30` (0x00 -> 0x200 words, 0x10 -> 0x246, 0x20 -> 0x249), and
 * 0x80 leaves that field zero, i.e. plain 0x800-byte sectors at 2x.
 */
#define CdlModeSpeed 0x80

long CdRead(long sectors, void *buf, long mode);
long CdReadSync(long mode, u_char *result);
void CdReadBreak(void);
/* cdread.c's `data_ready_callback`: drains one sector per CdReady interrupt. */
void CdReadDataReadyCallback(u_char intr, long result);
/* cdread.c's `read_retry`: re-issues CdlSetmode + CdlReadN after a shell open,
 * a seek error, or the 0x4B0-vblank watchdog in CdReadSync. */
long CdReadRetry(long mode);

/* Install the DMA3 (CD-ROM) data callback; returns the previous one. */
void CdDataCallback(long callback);
/* Fetch the next ready ring frame: *addr = its data, *header = its ring entry;
 * returns 0 when one was handed out. */
long StGetNext(StRingEventRecord **addr, StRingEventRecord **header);
/* Tear the stream down: clears the CD data / ready callbacks and both kernel
 * callback slots inside a critical section. */
void StUnSetRing(void);

long CD_init(long mode);
long CD_sync(long mode, u_char *result);
long CD_ready(long mode, u_char *result);
long CD_cw(u_char command, u_char *params, u_char *result, long async);

/* Declared identically by 65 translation units before this
 * header carried them. */

extern long g_CdCachedDir;
extern long g_CdDebugLevel;
extern volatile u_long *g_CdDmaChcr;
extern u_char g_CdLastCommand;
extern u_char g_CdModeByte;
extern volatile long g_CdReadBuffer;
extern volatile long g_CdReadMode;
extern volatile long g_CdReadSavedReadyCallback;
extern volatile long g_CdReadSavedSyncCallback;
extern volatile long g_CdReadSectorCount;
extern volatile long g_CdReadSectorWords;
extern volatile long g_CdReadStartVSync;
extern volatile u_char *g_CdReg0;
extern volatile u_char *g_CdReg1;
extern volatile u_char *g_CdReg2;
extern volatile u_char *g_CdReg3;
extern long g_CdTimeoutCounter;

long CdReadInterruptStatus(void);
void MDEC_in(volatile u_long* buf, long words);

/* Declared identically by 54 translation units before this
 * header carried them. */

extern long g_CdCachedShellOpenCount;
extern u_long g_CdCommandAckHasStatus[];
extern long g_CdCommandClearsReady[];
extern u_long g_CdCommandHasComplete[];
extern long g_CdCommandNeedsSetloc[];
extern long g_CdCommandParamCount[];
extern volatile u_char g_CdDataEndStatus;
extern void *g_CdDebugInfo[];
extern CdSearchDirEntry g_CdDirEntryName[];
extern volatile u_long *g_CdDmaBcr;
extern volatile u32 *g_CdDmaControl;
extern volatile u_long *g_CdDmaMadr;
extern volatile u_long *g_CdDpcr;
extern volatile long g_CdReadExpectedSector;
extern volatile long g_CdReadLastVSync;
extern volatile long g_CdReadPtr;
extern volatile long g_CdReadRemaining;
extern CdRegisterMap *volatile g_CdSpuRegs;
extern volatile u_long *g_CdromDelayReg;

void CD_dmastart(
    s32 channel,
    u32 address,
    u32 count,
    u32 size,
    u32 control,
    u8 mode,
    u32 unused);
extern long CD_namecmp(char *a, char *b);
extern long CD_newmedia(void);
void CdDefaultReadCallback(void);
void CdDefaultReadyCallback(void);
void CdDefaultSyncCallback(void);
void CdDispatchInterrupts(void);
void CdRead2Callback(void);
void MDEC_reset(long mode);
long MDEC_timeout(u_char* name);
long cd_read(long sectors, long address, void *mode);
long CD_cachefile(long dir);
long CD_datasync();
long CdResetState(void);
long CdGetSector2(long address, u_long words);
s32 StClearRingRange(long first, u_long count);
void data_ready_callback(void);
void StSetRingParams();
void MDEC_out();
long MDEC_in_sync(void);
long MDEC_out_sync(void);

#endif
