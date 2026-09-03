#include "game/state.h"
#include "game/input_internal.h"

/* The two live 0..7 selections: standard pad and NeGcon. */
/* Entry hook for the controller-configuration screen: clears the four screen
 * animation counters and snapshots both button-mapping selections so a cancel
 * can put them back. */
void BeginControllerConfig(void) {
    g_PadMappingIndex = ClampControllerMappingIndex(g_PadMappingIndex);
    g_NegconMappingIndex = ClampControllerMappingIndex(g_NegconMappingIndex);
    g_ControllerSceneAngleY = 0;
    g_ControllerSceneAngleX = 0;
    g_PadMappingIndexSaved = (u16)g_PadMappingIndex;
    g_NegconMappingIndexSaved = (u16)g_NegconMappingIndex;
}
