#ifndef GAME_CAR_INTERNAL_H
#define GAME_CAR_INTERNAL_H

#include "common.h"
#include "game/vector.h"

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

struct RaceIntroCameraScript {
    s16 firstKeyIndex[2];
    RaceIntroCameraKey keys[1];
};

typedef struct LaunchSpeedThreshold {
    s16 initial;
    s16 sustain;
} LaunchSpeedThreshold;

enum {
    CAR_LAUNCH_THRESHOLD_COUNT = 5,
};

static inline s32 NormalizeCarLaunchThresholdIndex(s32 index) {
    index %= CAR_LAUNCH_THRESHOLD_COUNT;
    return index < 0 ? index + CAR_LAUNCH_THRESHOLD_COUNT : index;
}

extern u32 g_CarModelSlot;
extern RaceIntroCameraKey *g_RaceIntroCameraCursor;
extern LaunchSpeedThreshold
    g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];
enum {
    CAR_TORQUE_BAND_COUNT = 10,
};
extern s16 g_TorqueBandEnd[CAR_TORQUE_BAND_COUNT];
extern s16 g_TorqueLossBandEnd[CAR_TORQUE_BAND_COUNT];

#endif
