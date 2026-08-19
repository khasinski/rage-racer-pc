#ifndef GAME_CAR_CONTROL_COMMAND_H
#define GAME_CAR_CONTROL_COMMAND_H

#include "common.h"

struct GameInputFrame;

typedef enum CarControlSource {
    CAR_CONTROL_PLAYER,
    CAR_CONTROL_RIVAL
} CarControlSource;

/* Immutable intent consumed by a car update. Player and rival producers fill
 * different fields, but both cross the simulation boundary through this one
 * explicit command type instead of reading input/AI globals mid-update. */
typedef struct CarControlCommand {
    CarControlSource source;
    s32 accelerator;
    s32 brake;
    s32 steering;
    s32 targetYaw;
    u8 shiftUp;
    u8 shiftDown;
    u8 steerLeft;
    u8 steerRight;
    u8 digitalController;
    u8 analogController;
    u8 boostEnabled;
} CarControlCommand;

CarControlCommand CarControlCommandBuildPlayer(
    const struct GameInputFrame *input, s32 negconMappingIndex,
    s32 drivingEnabled);
CarControlCommand CarControlCommandBuildRival(
    s32 targetYaw, s32 boostEnabled);

#endif
