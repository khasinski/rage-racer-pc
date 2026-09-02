#include "native_geometry_diagnostics.h"

#include <stdio.h>
#include <string.h>

typedef struct ConfigEntry {
    const char *key;
    const char *value;
} ConfigEntry;

static ConfigEntry s_config[4];
static int s_configCount;

const char *RuntimeConfigGet(const char *key) {
    int i;

    for (i = 0; i < s_configCount; i++) {
        if (strcmp(s_config[i].key, key) == 0) return s_config[i].value;
    }
    return NULL;
}

#define CHECK(expression) do { \
    if (!(expression)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #expression); \
        return 1; \
    } \
} while (0)

static int TestDefaults(void) {
    RageGeometryDiagnostics diagnostics = {0};

    s_configCount = 0;
    GeometryDiagnosticsInit(&diagnostics);

    CHECK(diagnostics.initialized);
    CHECK(!diagnostics.modelTraceEnabled);
    CHECK(!diagnostics.terrainTraceEnabled);
    CHECK(!diagnostics.terrainDecisionTraceEnabled);
    CHECK(!diagnostics.courseTraceEnabled);
    CHECK(diagnostics.modelTraceTimer == -1);
    CHECK(diagnostics.terrainDecisionTraceLimit == 10000);
    return 0;
}

static int TestConfiguredValues(void) {
    RageGeometryDiagnostics diagnostics = {0};

    s_config[0] = (ConfigEntry){"diagnostics.model_trace_timer", "42"};
    s_config[1] = (ConfigEntry){"diagnostics.terrain_trace_clut", "7e00"};
    s_config[2] = (ConfigEntry){"diagnostics.terrain_decision_limit", "250"};
    s_config[3] = (ConfigEntry){"diagnostics.course_trace_tpage", "19f"};
    s_configCount = 4;
    GeometryDiagnosticsInit(&diagnostics);

    CHECK(diagnostics.modelTraceEnabled &&
          diagnostics.modelTraceTimer == 42);
    CHECK(diagnostics.terrainTraceEnabled &&
          diagnostics.terrainTraceClut == 0x7e00);
    CHECK(diagnostics.terrainDecisionTraceLimit == 250);
    CHECK(diagnostics.courseTraceEnabled &&
          diagnostics.courseTraceTpage == 0x19f);
    return 0;
}

static int TestInvalidValuesUseDefaults(void) {
    RageGeometryDiagnostics diagnostics = {0};

    s_config[0] = (ConfigEntry){"diagnostics.model_trace_timer", "12x"};
    s_config[1] = (ConfigEntry){
        "diagnostics.terrain_decision_limit", "999999999999999999999"};
    s_configCount = 2;
    GeometryDiagnosticsInit(&diagnostics);

    CHECK(diagnostics.modelTraceEnabled &&
          diagnostics.modelTraceTimer == -1);
    CHECK(diagnostics.terrainDecisionTraceLimit == 10000);
    return 0;
}

int main(void) {
    if (TestDefaults() != 0) return 1;
    if (TestConfiguredValues() != 0) return 1;
    if (TestInvalidValuesUseDefaults() != 0) return 1;
    puts("classic geometry diagnostics parse bounded configuration values");
    return 0;
}
