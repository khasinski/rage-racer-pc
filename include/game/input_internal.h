#ifndef GAME_INPUT_INTERNAL_H
#define GAME_INPUT_INTERNAL_H

#include "common.h"
#include "game/vector.h"

extern u16 g_PadButtonMapping[16];
typedef s16 ControllerMappingIndex;
enum {
    CONTROLLER_MAPPING_COUNT = 8,
    CONTROLLER_MAPPING_BUTTON_COUNT = 8,
    CONTROLLER_CONFIG_ACTION_COUNT = 5,
    CONTROLLER_CONFIG_LABEL_SLOT_COUNT = 6,
    CONTROLLER_CONFIG_BUTTON_POINT_COUNT = 16,
    CONTROLLER_CONFIG_ROW_COUNT =
        CONTROLLER_MAPPING_COUNT * CONTROLLER_CONFIG_ACTION_COUNT,
    CONTROLLER_MAPPING_FIRST = 0,
    CONTROLLER_MAPPING_LAST = CONTROLLER_MAPPING_COUNT - 1,
};

extern u16 g_PadButtonPresets[CONTROLLER_MAPPING_COUNT]
                             [CONTROLLER_MAPPING_BUTTON_COUNT];
extern u16 g_NegconButtonPresets[CONTROLLER_MAPPING_COUNT]
                                [CONTROLLER_MAPPING_BUTTON_COUNT];
extern u8 g_PadConfigLabelRows[CONTROLLER_CONFIG_ROW_COUNT];
extern u8 g_PadConfigButtonRows[CONTROLLER_CONFIG_ROW_COUNT];
extern u8 g_NegconConfigLabelRows[CONTROLLER_CONFIG_ROW_COUNT];
extern u8 g_NegconConfigButtonRows[CONTROLLER_CONFIG_ROW_COUNT];
extern DVec g_PadLabelSlots[CONTROLLER_CONFIG_LABEL_SLOT_COUNT];
extern DVec g_PadCalloutLabelPoints[CONTROLLER_CONFIG_LABEL_SLOT_COUNT];
extern DVec
    g_PadCalloutButtonPoints[CONTROLLER_CONFIG_BUTTON_POINT_COUNT];

static inline ControllerMappingIndex ClampControllerMappingIndex(s32 index) {
    if (index < CONTROLLER_MAPPING_FIRST) {
        return CONTROLLER_MAPPING_FIRST;
    }
    if (index > CONTROLLER_MAPPING_LAST) {
        return CONTROLLER_MAPPING_LAST;
    }
    return (ControllerMappingIndex)index;
}

extern ControllerMappingIndex g_PadMappingIndex;
extern ControllerMappingIndex g_NegconMappingIndex;
extern u16 g_PadMappingIndexSaved;
extern u16 g_NegconMappingIndexSaved;
extern s32 g_ControllerSceneAngleX;
extern s32 g_ControllerSceneAngleY;
typedef s16 NegconCalibrationValue;
extern NegconCalibrationValue g_NegconMaxTwist;
extern NegconCalibrationValue g_NegconSteerPlay;
extern NegconCalibrationValue g_NegconSteerNeutral;
extern NegconCalibrationValue g_NegconNeutralI;
extern NegconCalibrationValue g_NegconNeutralII;
extern NegconCalibrationValue g_NegconNeutralL;
enum {
    NEGCON_CALIBRATION_FIRST = 0,
    NEGCON_CALIBRATION_COUNT = 4,
    NEGCON_CALIBRATION_LAST = NEGCON_CALIBRATION_COUNT - 1,
    NEGCON_STEER_RANGE_COUNT = NEGCON_CALIBRATION_COUNT,
    NEGCON_DEAD_ZONE_VALUE_COUNT = 2,
};

static inline NegconCalibrationValue ClampNegconCalibrationValue(s32 value) {
    if (value < NEGCON_CALIBRATION_FIRST) return NEGCON_CALIBRATION_FIRST;
    if (value > NEGCON_CALIBRATION_LAST) return NEGCON_CALIBRATION_LAST;
    return (NegconCalibrationValue)value;
}
extern s16 g_NegconSteerRange[NEGCON_STEER_RANGE_COUNT];
extern u16 g_NegconSteerDeadZone[NEGCON_CALIBRATION_COUNT]
                                [NEGCON_DEAD_ZONE_VALUE_COUNT];
extern s32 g_NegconPlayScale[NEGCON_CALIBRATION_COUNT];

/* Runtime calibration state normally comes from a validated save or from the
 * option screen. Treat any damaged value as the first retail preset before it
 * is used as a table index. */
static inline s32 NegconCalibrationIndex(NegconCalibrationValue value) {
    return (u32)value < NEGCON_CALIBRATION_COUNT ? value
                                                 : NEGCON_CALIBRATION_FIRST;
}

/* Reset controller-screen animation and retain both mapping selections so
 * cancelling the screen can restore them. */
void BeginControllerConfig(void);

static inline s32 GetNegconSteerRange(void) {
    s32 index = NegconCalibrationIndex(g_NegconMaxTwist);
    s32 range;

    range = g_NegconSteerRange[index];
    return range > 0 ? range : 1;
}

#endif
