#ifndef PSYQ_CD_TYPES_H
#define PSYQ_CD_TYPES_H

#include "common.h"
#include "psyq/cd_location.h"

typedef struct CdlFILE {
    CdlLOC pos;
    /* PsyQ's long is 32-bit even on LP64 hosts. Keep the recovered 24-byte
     * ABI: game state reserves exactly that much storage for this record. */
    s32 size;
    char name[16];
} CdlFILE;

_Static_assert(sizeof(CdlFILE) == 24, "CdlFILE must retain the PsyQ ABI");

typedef union CdlLBA {
    s32 sector;
    CdlLOC pos;
} CdlLBA;

typedef struct CdlDIR {
    s32 number;
    s32 parent_number;
    CdlLBA lba;
    char name[32];
} CdlDIR;

typedef void (*CdCallback)(u8 status, u8 *result);
typedef void (*StCallback)(void);

typedef struct CdAlarm {
    s32 deadline;
    s32 count;
    char *name;
} CdAlarm;

typedef struct CdIntr {
    u8 sync;
    u8 ready;
    u8 command;
} CdIntr;

typedef struct CdlATV {
    u8 val0;
    u8 val1;
    u8 val2;
    u8 val3;
} CdlATV;

typedef struct CdRegisterMap {
    u8 pad0[0x180];
    u16 cd_left_volume;
    u16 cd_right_volume;
    u8 pad184[0x1AA - 0x184];
    u16 audio_control;
    u8 pad1AC[0x1B0 - 0x1AC];
    u16 output_left_volume;
    u16 output_right_volume;
    u8 pad1B4[0x1B8 - 0x1B4];
    u16 status_mode_a;
    u16 status_mode_b;
} CdRegisterMap;

typedef struct StRingEventRecord {
    u16 state;
    u8 pad02[6];
    u32 frame;
    u8 pad0C[4];
    u16 width;
    u16 height;
    u8 pad14[0xC];
} StRingEventRecord;

typedef struct StRingClearRecord {
    s32 value;
    u8 pad4[0x1C];
} StRingClearRecord;

typedef struct StStrHeader {
    u16 state;
    u16 mode;
    u16 frame;
    u16 nSectors;
    u16 nFrames;
    u8 pad0A[0x12];
    CdlLOC loc;
} StStrHeader;

typedef struct CdSearchDirEntry {
    s32 type;
    u8 pad4[4];
    u8 name[0x24];
} CdSearchDirEntry;

typedef struct CdRawWord {
    u8 bytes[4];
} CdRawWord;

typedef struct CdPathCacheRecord {
    s32 lba;
    u8 rest[40];
} CdPathCacheRecord;

_Static_assert(sizeof(CdlLBA) == 4, "CdlLBA must retain the PsyQ ABI");
_Static_assert(sizeof(CdlDIR) == 44, "CdlDIR must retain the PsyQ ABI");
_Static_assert(sizeof(CdlATV) == 4, "CdlATV must retain the PsyQ ABI");
_Static_assert(sizeof(StRingEventRecord) == 32,
               "StRingEventRecord must retain the PsyQ ABI");
_Static_assert(sizeof(StRingClearRecord) == 32,
               "StRingClearRecord must retain the PsyQ ABI");
_Static_assert(sizeof(StStrHeader) == 32,
               "StStrHeader must retain the PsyQ ABI");
_Static_assert(sizeof(CdSearchDirEntry) == 44,
               "CdSearchDirEntry must retain the PsyQ ABI");
_Static_assert(sizeof(CdPathCacheRecord) == 44,
               "CdPathCacheRecord must retain the PsyQ ABI");

#endif
