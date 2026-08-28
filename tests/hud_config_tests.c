#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port_config.h"
#include "rage/hud_config.h"

static RagePortConfig config;
static int modernEnabled = 1;
static int failures;

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) {         \
    fprintf(stderr, "%s:%d: expected %d, got %d\n", __FILE__, __LINE__,    \
            (expected), (actual)); failures++;                                 \
} } while (0)

const RagePortConfig *PortActiveConfig(void) { return &config; }
int ModernIsEnabled(void) { return modernEnabled; }

const char *RuntimeConfigGet(const char *key) {
    if (!strcmp(key, "hud.anchor")) return "edges";
    if (!strcmp(key, "hud.show_lap_times")) return "false";
    if (!strcmp(key, "hud.show_time_limit")) return "true";
    return NULL;
}

int RuntimeConfigEnabled(const char *key, const char *legacyEnv) {
    const char *value = RuntimeConfigGet(key);
    (void)legacyEnv;
    return value != NULL && strcmp(value, "false") != 0;
}

int main(void) {
    config.modernAspect = RAGE_MODERN_ASPECT_16_9;
    EXPECT_EQ(-45, HudLeftX(8));
    EXPECT_EQ(303, HudRightX(250));
    EXPECT_EQ(0, HudShowLapTimes());
    EXPECT_EQ(1, HudShowTimeLimit());

    modernEnabled = 0;
    EXPECT_EQ(8, HudLeftX(8));
    EXPECT_EQ(250, HudRightX(250));
    modernEnabled = 1;
    config.modernAspect = RAGE_MODERN_ASPECT_4_3;
    EXPECT_EQ(8, HudLeftX(8));
    EXPECT_EQ(250, HudRightX(250));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
