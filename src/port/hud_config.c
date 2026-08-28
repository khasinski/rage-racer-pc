#include "rage/hud_config.h"

#include <string.h>

#include "modern/modern_renderer.h"
#include "port_config.h"
#include "runtime_config.h"

enum { RAGE_HUD_WIDESCREEN_MARGIN = 53 };

static int s_initialized;
static int s_anchorEdges;
static int s_showLapTimes;
static int s_showTimeLimit;

static void RageHudConfigInit(void) {
    const char *anchor;
    if (s_initialized) return;
    anchor = RageRuntimeConfigGet("hud.anchor");
    s_anchorEdges = anchor == NULL || strcmp(anchor, "center") != 0;
    s_showLapTimes = RageRuntimeConfigGet("hud.show_lap_times") == NULL ||
        RageRuntimeConfigEnabled("hud.show_lap_times", NULL);
    s_showTimeLimit = RageRuntimeConfigGet("hud.show_time_limit") == NULL ||
        RageRuntimeConfigEnabled("hud.show_time_limit", NULL);
    s_initialized = 1;
}

static int RageHudEdgeOffset(void) {
    RageHudConfigInit();
    if (!s_anchorEdges || !RageModernIsEnabled() ||
        RagePortActiveConfig()->modernAspect != RAGE_MODERN_ASPECT_16_9)
        return 0;
    return RAGE_HUD_WIDESCREEN_MARGIN;
}

int RageHudLeftX(int x) { return x - RageHudEdgeOffset(); }
int RageHudRightX(int x) { return x + RageHudEdgeOffset(); }

int RageHudShowLapTimes(void) {
    RageHudConfigInit();
    return s_showLapTimes;
}

int RageHudShowTimeLimit(void) {
    RageHudConfigInit();
    return s_showTimeLimit;
}
