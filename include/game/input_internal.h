#ifndef GAME_INPUT_INTERNAL_H
#define GAME_INPUT_INTERNAL_H

#include "common.h"

extern u16 g_PadButtonMapping[16];
typedef s16 ControllerMappingIndex;
extern ControllerMappingIndex g_PadMappingIndex;
extern ControllerMappingIndex g_NegconMappingIndex;
typedef s16 NegconCalibrationValue;
extern NegconCalibrationValue g_NegconMaxTwist;
extern NegconCalibrationValue g_NegconSteerPlay;
extern NegconCalibrationValue g_NegconSteerNeutral;
extern NegconCalibrationValue g_NegconNeutralI;
extern NegconCalibrationValue g_NegconNeutralII;
extern NegconCalibrationValue g_NegconNeutralL;
enum {
    NEGCON_STEER_RANGE_COUNT = 4,
};
extern s16 g_NegconSteerRange[NEGCON_STEER_RANGE_COUNT];

static inline s32 GetNegconSteerRange(void) {
    s32 index = g_NegconMaxTwist;
    s32 range;

    if ((u32)index >= NEGCON_STEER_RANGE_COUNT) {
        index = 0;
    }
    range = g_NegconSteerRange[index];
    return range > 0 ? range : 1;
}

#endif
