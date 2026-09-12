#include "game/state.h"
#include "game/input_internal.h"

/* Entry hook for the controller-configuration screen: clamps and snapshots
 * both button-mapping selections so a cancel can put them back, then resets
 * the controller model orientation. */
void BeginControllerConfig(void) {
    g_PadMappingIndex = ClampControllerMappingIndex(g_PadMappingIndex);
    g_NegconMappingIndex = ClampControllerMappingIndex(g_NegconMappingIndex);
    g_ControllerSetup.angleY = 0;
    g_ControllerSetup.angleX = 0;
    g_ControllerSetup.savedPadMapping = (u16)g_PadMappingIndex;
    g_ControllerSetup.savedNegconMapping = (u16)g_NegconMappingIndex;
}
