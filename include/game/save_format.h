#ifndef GAME_SAVE_FORMAT_H
#define GAME_SAVE_FORMAT_H

#include "common.h"
#include "game/menu_types.h"

#define MC_GP_CARS_OFS    0x58
#define MC_EXTRA_CARS_OFS 0xC0
#define MC_TIME_CARS_OFS  0x128
#define MC_BLOCK_SIZE         0x1000
#define MC_BLOCK_CHECKSUM_OFS 0xFFC

typedef union GameSaveHeaderRow {
    struct {
        u8 nameLength;
        u8 name[7];
        s32 saveCounter;
        u8 reserved[0x70];
        u32 checksum;
    } fields;
    u8 bytes[0x80];
    u16 halfwords[0x40];
} GameSaveHeaderRow;

typedef union GameSaveHeaderRowAddress {
    uintptr_t value;
    GameSaveHeaderRow *pointer;
} GameSaveHeaderRowAddress;

typedef union GameSaveHeaderWordAddress {
    u8 *bytes;
    volatile u32 *word;
} GameSaveHeaderWordAddress;

typedef struct GameSaveHeaderClearCursor {
    u8 prefix[0xC];
    u16 reservedHalfword;
    u8 suffix[0x74];
} GameSaveHeaderClearCursor;

typedef union GameSaveHeaderRowsAddress {
    u8 *bytes;
    GameSaveHeaderRow *rows;
    GameSaveHeaderClearCursor *clearCursors;
} GameSaveHeaderRowsAddress;

typedef struct SavedCarSetup {
    u8 modelVariant;
    u8 tireCompound;
    u8 transmission;
    u8 paintColor1;
    u8 paintColor2;
    u8 enabled;
    u8 reserved[2];
} SavedCarSetup;

typedef struct SavedClassRecord {
    u16 grade;
    u16 clears;
} SavedClassRecord;

typedef struct SavedRaceProgress {
    s32 course;
    s32 carIndex;
    s32 classIndex;
    s32 maxClassReached;
    s32 money;
} SavedRaceProgress;

/* Retail file: 0x200 icon, 0x80 header, this payload, repeated header. */
typedef struct GameSaveBlock {
    u16 padMappingIndex;
    u16 negconMappingIndex;
    u16 negconSteerNeutral;
    u16 negconSteerPlay;
    u16 negconNeutralI;
    u16 negconNeutralII;
    u16 negconNeutralL;
    u16 negconMaxTwist;
    SavedRaceProgress grandPrixProgress;
    SavedRaceProgress extraGrandPrixProgress;
    SavedRaceProgress timeAttackProgress;
    s16 bgmSelection;
    u16 extraGrandPrixUnlocked;
    s32 maxClassReached[2];
    SavedCarSetup carSetup[3][13];
    SavedClassRecord classRecords[11];
    u16 teamLogoClut[16];
    u16 teamLogoCanvas[0x400];
    s32 bestLapTimes[2][4][2];
    s32 bestTotalTimes[2][4][2];
    RaceRecord rankingRecords[2][4][5];
    RaceRecord timeRecords[2][4][5];
    s32 bestSectorTimes[2][4][3];
    s32 bgmVolume;
    s32 sfxVolume;
    s32 monoOutput;
    u8 grandPrixCourseProgress[8];
    u8 extraGrandPrixCourseProgress[8];
    u8 reserved[0x24];
    u32 checksum;
} GameSaveBlock;

typedef union GameSaveBlockAddress {
    s32 offset;
    u8 *bytePointer;
    u16 *halfwordPointer;
    s32 *wordPointer;
    GameSaveBlock *pointer;
} GameSaveBlockAddress;

#endif
