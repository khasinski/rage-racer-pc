#ifndef RAGE_PC_PSYQ_SPU_H
#define RAGE_PC_PSYQ_SPU_H

#include <sys/types.h>

#include "common.h"
#include "psyq/spu_internal_types.h"

/*
 * One block of the SPU heap. The two top bits of `addr` are flags, not
 * address: FREE marks a released block that _spu_gcSPU may coalesce, END
 * marks the block that closes the list. EMPTY is the whole word written over
 * a block that has been merged away.
 */
typedef struct SpuMallocEntry {
    u_long addr;
    u_long size;
} SpuMallocEntry;

#define SPU_BLOCK_FREE  0x80000000
#define SPU_BLOCK_END   0x40000000
#define SPU_BLOCK_ADDR  0x0FFFFFFF
#define SPU_BLOCK_EMPTY 0x2FFFFFFF

typedef struct SpuCommonAttr {
    u_long mask;
    struct {
        SpuVolume volume;
        SpuVolume volmode;
        SpuVolume volumex;
    } mvol;
    struct {
        SpuVolume volume;
        long reverb;
        long mix;
    } cd;
    struct {
        SpuVolume volume;
        long reverb;
        long mix;
    } ext;
} SpuCommonAttr;

typedef struct SpuReverbRegAttr {
    u_long flags;
    short dAPF1;
    short dAPF2;
    short vIIR;
    short vCOMB1;
    short vCOMB2;
    short vCOMB3;
    short vCOMB4;
    short vWALL;
    short vAPF1;
    short vAPF2;
    short mLSAME;
    short mRSAME;
    short mLCOMB1;
    short mRCOMB1;
    short mLCOMB2;
    short mRCOMB2;
    short dLSAME;
    short dRSAME;
    short mLDIFF;
    short mRDIFF;
    short mLCOMB3;
    short mRCOMB3;
    short mLCOMB4;
    short mRCOMB4;
    short dLDIFF;
    short dRDIFF;
    short mLAPF1;
    short mRAPF1;
    short mLAPF2;
    short mRAPF2;
    short vLIN;
    short vRIN;
} SpuReverbRegAttr;

void SpuInit(void);
void _SpuInit(long reset_voice_center_note);
void SpuStart(void);
long _spu_resetTransferControl(void);
long _spu_init(long reset_mode);
long _spu_writeByIO(long addr, long size);
void _spu_transferCallback(void);
void _spu_startDmaTransfer(u_long addr, u_short mode, u_long size);
long _spu_t(long op, ...);
long _spu_Fw(long addr, long size);
long _spu_Fr(long addr, long size);
void _spu_FsetRXX(long reg, u_long value, long use_mem_shift);
long _spu_FsetRXXa(long reg, u_long value);
long _spu_FgetRXXa(long reg, long raw);
void _SpuDataCallback(void *callback);
void SpuQuit(void);
void _spu_gcSPU(void);
long _SpuIsInAllocateArea(u_long addr);
long _SpuIsInAllocateArea_(u_long addr);
long SpuInitMalloc(long num, u_long *memlist);
long SpuMalloc(long size);
void SpuFree(u_long addr);
u_long SpuSetNoiseVoice(long on_off, u_long voice_bit);
u_long _SpuSetAnyVoice(long on_off, u_long voice_bit, long reg_lo, long reg_hi);
long SpuSetReverb(long on_off);
long SpuSetReverbModeParam(SpuReverbAttr *attr);
void _spu_setReverbAttr(SpuReverbRegAttr *attr);
long SpuClearReverbWorkArea(u_long mode);
long SpuTransferStatus(long addr, long mode);
long SpuGetKeyStatus(u_long voice_bit);
u_long Spu_ReadFromSpu(long addr, u_long size);
long SpuSetTransferStartAddr(long addr);
long SpuSetTransferMode(long mode);
long SpuIsTransferCompleted(long wait);
void _spu_setTransferCompletionFlag(long completed);
u_long _spu_isTransferIdle(void);
void SpuSetCommonAttr(SpuCommonAttr *attr);

/* The SPU hardware register file at g_SpuRegBase, as a struct and as the raw
 * half-word window the transfer paths use. */
/* Declared identically by 50 translation units before this
 * header carried them. */

extern long _spu_mem_mode_unitM;
extern volatile u_long *g_SpuDelayReg;
extern volatile u_long *g_SpuDmaBcr;
extern volatile u_long *g_SpuDmaChcr;
extern volatile u_long *g_SpuDmaMadr;
extern long g_SpuIsStarted;
extern long g_SpuMemMode;
extern long g_SpuRevReserveWa;
extern long g_SpuRevWorkAreaAddr;
extern long g_SpuTransferByIo;
extern long g_SpuTransferCompleted;
extern long g_SpuTransferEvent;
extern long g_SpuTransferIsRead;
extern long g_SpuTransferMode;
extern u_short g_SpuTransferStartAddr;
extern long g_SpuWaitCount;

/* Declared identically by 19 translation units before this
 * header carried them. */

extern long D_8009A710;
extern u_short g_SpuVoiceCenterNoteLast;
extern long g_SpuInTransfer;
extern long g_SpuDmaBlockCount;
extern long g_SpuDmaTransferAddr;
extern volatile long *g_SpuDpcr;
extern long g_SpuDummyAdpcmBlock;
extern u_char g_SpuMallocArea[];
extern long g_SpuRevAttrDelay;
extern short g_SpuRevAttrDepthLeft;
extern short g_SpuRevAttrDepthRight;
extern long g_SpuRevAttrFeedback;
extern SpuReverbRegAttr g_SpuRevAttrTable[];
extern u_char g_SpuTimeoutMsgDmaf[];
extern char g_SpuTimeoutMsgReset[];
extern u_char g_SpuTimeoutMsgWrdy[];
extern long g_SpuZeroBuf[];
extern void (*volatile g_SpuTransferCallback)(void);
extern void (*volatile g_SpuIrqCallback)(void);

#endif
