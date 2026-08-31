
#include <string.h>

#include "modern/modern_renderer.h"
#include "runtime_config.h"

enum { RAGE_HUD_WIDESCREEN_MARGIN = 53 };

static int s_initialized;
static int s_anchorEdges;
static int s_showLapTimes;
static int s_showTimeLimit;

static void HudConfigInit(void) {
    const char *anchor;
    if (s_initialized) return;
    anchor = RuntimeConfigGet("hud.anchor");
    s_anchorEdges = anchor == NULL || strcmp(anchor, "center") != 0;
    s_showLapTimes = RuntimeConfigGet("hud.show_lap_times") == NULL ||
        RuntimeConfigEnabled("hud.show_lap_times");
    s_showTimeLimit = RuntimeConfigGet("hud.show_time_limit") == NULL ||
        RuntimeConfigEnabled("hud.show_time_limit");
    s_initialized = 1;
}

static int HudEdgeOffset(void) {
    HudConfigInit();
    if (!s_anchorEdges || !ModernIsEnabled() ||
        PortActiveConfig()->modernAspect != RAGE_MODERN_ASPECT_16_9)
        return 0;
    return RAGE_HUD_WIDESCREEN_MARGIN;
}

int HudLeftX(int x) { return x - HudEdgeOffset(); }
int HudRightX(int x) { return x + HudEdgeOffset(); }

/* A quarter of the canvas at either end counts as an edge. Everything the
 * retail layout puts there is furniture along a side; everything it puts
 * between belongs to the middle. */
enum { RAGE_HUD_EDGE_BAND = 80, RAGE_HUD_CANVAS_WIDTH = 320 };

int HudAnchorX(int x) {
    if (x < RAGE_HUD_EDGE_BAND) return HudLeftX(x);
    if (x >= RAGE_HUD_CANVAS_WIDTH - RAGE_HUD_EDGE_BAND) return HudRightX(x);
    return x;
}

int HudShowLapTimes(void) {
    HudConfigInit();
    return s_showLapTimes;
}

int HudShowTimeLimit(void) {
    HudConfigInit();
    return s_showTimeLimit;
}
