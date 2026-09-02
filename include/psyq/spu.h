#ifndef RAGE_PC_PSYQ_SPU_H
#define RAGE_PC_PSYQ_SPU_H

#include <sys/types.h>

#include "common.h"
#include "psyq/spu_internal_types.h"

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

#endif
