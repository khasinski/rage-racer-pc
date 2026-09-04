#include "../../src/port/host_state_pad.c"

_Static_assert(sizeof(g_NegconSteerDeadZone) ==
                   NEGCON_CALIBRATION_COUNT * NEGCON_DEAD_ZONE_VALUE_COUNT *
                       sizeof(u16),
               "NeGcon dead-zone table shape changed");
_Static_assert(sizeof(g_PadLabelSlots) ==
                   CONTROLLER_CONFIG_LABEL_SLOT_COUNT * sizeof(DVec),
               "pad label slot table shape changed");
_Static_assert(sizeof(g_PadCalloutLabelPoints) ==
                   CONTROLLER_CONFIG_LABEL_SLOT_COUNT * sizeof(DVec),
               "pad callout label table shape changed");
_Static_assert(sizeof(g_PadCalloutButtonPoints) ==
                   CONTROLLER_CONFIG_BUTTON_POINT_COUNT * sizeof(DVec),
               "pad callout button table shape changed");
_Static_assert(sizeof(g_PadErrorState) == sizeof(s32),
               "pad error state must be a scalar");
