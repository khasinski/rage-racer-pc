#ifndef GAME_CAR_RUNTIME_STATE_H
#define GAME_CAR_RUNTIME_STATE_H

#include "common.h"

enum {
    CAR_LAUNCH_THRESHOLD_COUNT = 5,
    CAR_TORQUE_BAND_COUNT = 10,
    /* Eleven live cars followed by retail's -1 sentinel entry. */
    RACE_GRID_STORAGE_COUNT = 12,
};

typedef union RaceGridSlot {
    s32 value;
    struct {
        u16 modelId;
        u16 reserved;
    } halves;
} RaceGridSlot;

typedef union RaceIntroCameraCoordinate {
    s32 word;
    struct {
        u16 value;
        u16 reserved;
    } half;
} RaceIntroCameraCoordinate;

typedef struct RaceIntroCameraKey {
    RaceIntroCameraCoordinate x;
    RaceIntroCameraCoordinate y;
    RaceIntroCameraCoordinate z;
    s32 mode;
    s16 startFrame;
    s16 duration;
} RaceIntroCameraKey;

typedef struct RaceIntroCameraScript {
    s16 firstKeyIndex[2];
    RaceIntroCameraKey keys[1];
} RaceIntroCameraScript;

typedef struct LaunchSpeedThreshold {
    s16 initial;
    s16 sustain;
} LaunchSpeedThreshold;

#endif
