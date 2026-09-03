#ifndef GAME_SAVE_FORMAT_H
#define GAME_SAVE_FORMAT_H

#include "common.h"
#include "game/car.h"
#include "game/menu_types.h"

#include <stddef.h>

#define MC_GP_CARS_OFS    0x58
#define MC_EXTRA_CARS_OFS 0xC0
#define MC_TIME_CARS_OFS  0x128
#define MC_ICON_BLOCK_SIZE    0x200
#define MC_ICON_TITLE_OFS     0x04
#define MC_ICON_CLUT_OFS      0x60
#define MC_ICON_PIXELS_OFS    0x80
#define MC_HEADER_SIZE        0x80
#define MC_HEADER_CHECKSUM_OFS 0x7C
#define MC_BLOCK_SIZE         0x1000
#define MC_SAVE_PATH_SIZE     0x1A
#define MC_SAVE_TITLE_SIZE    0x46
#define MC_SAVE_BLOCK_OFS     (MC_ICON_BLOCK_SIZE + MC_HEADER_SIZE)
#define MC_BACKUP_HEADER_OFS  (MC_SAVE_BLOCK_OFS + MC_BLOCK_SIZE)
#define MC_BLOCK_CHECKSUM_OFS 0xFFC

enum { SAVE_TEAM_NAME_CAPACITY = 7 };

typedef struct GameSaveIconBlock {
    u8 magic[2];
    u8 format;
    u8 frameCount;
    char title[MC_ICON_CLUT_OFS - MC_ICON_TITLE_OFS];
    u16 clut[(MC_ICON_PIXELS_OFS - MC_ICON_CLUT_OFS) / sizeof(u16)];
    u8 pixels[MC_ICON_BLOCK_SIZE - MC_ICON_PIXELS_OFS];
} GameSaveIconBlock;

_Static_assert(sizeof(GameSaveIconBlock) == MC_ICON_BLOCK_SIZE,
               "memory-card icon block size changed");
_Static_assert(__builtin_offsetof(GameSaveIconBlock, title) ==
                   MC_ICON_TITLE_OFS,
               "memory-card icon title offset changed");
_Static_assert(__builtin_offsetof(GameSaveIconBlock, clut) == MC_ICON_CLUT_OFS,
               "memory-card icon CLUT offset changed");
_Static_assert(__builtin_offsetof(GameSaveIconBlock, pixels) ==
                   MC_ICON_PIXELS_OFS,
               "memory-card icon pixels offset changed");

typedef union GameSaveHeaderRow {
    struct {
        u8 nameLength;
        u8 name[SAVE_TEAM_NAME_CAPACITY];
        s32 saveCounter;
        u8 reserved[0x70];
        u32 checksum;
    } fields;
    u8 bytes[0x80];
    u16 halfwords[0x40];
} GameSaveHeaderRow;

_Static_assert(sizeof(GameSaveHeaderRow) == MC_HEADER_SIZE,
               "memory-card save header size changed");
_Static_assert(offsetof(GameSaveHeaderRow, fields.checksum) ==
                   MC_HEADER_CHECKSUM_OFS,
               "memory-card save header checksum offset changed");

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
    union {
        s32 money;
        s32 timeAttackSeries;
    };
} SavedRaceProgress;

typedef enum SavedCarTableIndex {
    SAVED_CARS_GRAND_PRIX,
    SAVED_CARS_EXTRA_GRAND_PRIX,
    SAVED_CARS_TIME_ATTACK,
    SAVED_CAR_TABLE_COUNT,
} SavedCarTableIndex;

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
    SavedCarSetup carSetup[SAVED_CAR_TABLE_COUNT][GAME_CAR_COUNT];
    SavedClassRecord classRecords[CLASS_RECORD_COUNT];
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

_Static_assert(sizeof(GameSaveBlock) == MC_BLOCK_SIZE,
               "memory-card save block size changed");
_Static_assert(offsetof(GameSaveBlock, checksum) == MC_BLOCK_CHECKSUM_OFS,
               "memory-card save block checksum offset changed");
_Static_assert(offsetof(GameSaveBlock, carSetup[SAVED_CARS_GRAND_PRIX]) ==
                   MC_GP_CARS_OFS,
               "Grand Prix car table offset changed");
_Static_assert(offsetof(GameSaveBlock, carSetup[SAVED_CARS_EXTRA_GRAND_PRIX]) ==
                   MC_EXTRA_CARS_OFS,
               "Extra Grand Prix car table offset changed");
_Static_assert(offsetof(GameSaveBlock, carSetup[SAVED_CARS_TIME_ATTACK]) ==
                   MC_TIME_CARS_OFS,
               "Time Attack car table offset changed");

#endif
