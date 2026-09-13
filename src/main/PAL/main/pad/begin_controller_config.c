#include "game/state.h"
#include "game/input_internal.h"

/* Entry hook for the controller-configuration screen: clamps and snapshots
 * both button-mapping selections so a cancel can put them back, then resets
 * the controller model orientation. */
void BeginControllerConfig(ControllerSetup *setup) {
    g_PadMappingIndex = ClampControllerMappingIndex(g_PadMappingIndex);
    g_NegconMappingIndex = ClampControllerMappingIndex(g_NegconMappingIndex);
    setup->angleY = 0;
    setup->angleX = 0;
    setup->savedPadMapping = (u16)g_PadMappingIndex;
    setup->savedNegconMapping = (u16)g_NegconMappingIndex;
}
