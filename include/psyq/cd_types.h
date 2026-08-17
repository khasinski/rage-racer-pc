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
    long sector;
    CdlLOC pos;
} CdlLBA;

typedef struct CdlDIR {
    long number;
    long parent_number;
    CdlLBA lba;
    char name[32];
} CdlDIR;

typedef void (*CdCallback)(u_char status, u_char *result);
typedef void (*StCallback)(void);

typedef struct CdAlarm {
    long deadline;
    long count;
    char *name;
} CdAlarm;

typedef struct CdIntr {
    u_char sync;
    u_char ready;
    u_char command;
} CdIntr;

typedef struct CdlATV {
    u_char val0;
    u_char val1;
    u_char val2;
    u_char val3;
} CdlATV;

typedef struct CdRegisterMap {
    u_char pad0[0x180];
    u_short cd_left_volume;
    u_short cd_right_volume;
    u_char pad184[0x1AA - 0x184];
    u_short audio_control;
    u_char pad1AC[0x1B0 - 0x1AC];
    u_short output_left_volume;
    u_short output_right_volume;
    u_char pad1B4[0x1B8 - 0x1B4];
    u_short status_mode_a;
    u_short status_mode_b;
} CdRegisterMap;

typedef struct StRingEventRecord {
    volatile u_short state;
    u_char pad02[6];
    u_long frame;
    u_char pad0C[4];
    u_short width;
    u_short height;
    u_char pad14[0xC];
} StRingEventRecord;

typedef struct StRingClearRecord {
    long value;
    u_char pad4[0x1C];
} StRingClearRecord;

typedef struct StStrHeader {
    u_short state;
    u_short mode;
    u_short frame;
    u_short nSectors;
    u_short nFrames;
    u_char pad0A[0x12];
    CdlLOC loc;
} StStrHeader;

typedef struct CdSearchDirEntry {
    long type;
    u_char pad4[4];
    u_char name[0x24];
} CdSearchDirEntry;

typedef struct CdRawWord {
    u_char bytes[4];
} CdRawWord;

typedef struct CdPathCacheRecord {
    long lba;
    u_char rest[40];
} CdPathCacheRecord;

#endif
