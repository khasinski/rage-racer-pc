#include "native_geometry_diagnostics.h"

#include <limits.h>
#include <string.h>

#include "../runtime_config.h"

static int ParseValue(const char *value, int fallback, int base) {
    int parsed;

    return RuntimeParseInt(value, base, INT_MIN, INT_MAX, &parsed)
               ? parsed
               : fallback;
}

void GeometryDiagnosticsInit(RageGeometryDiagnostics *diagnostics) {
    const char *model;
    const char *modelTimer;
    const char *terrainTimer;
    const char *terrainClut;
    const char *terrainTpage;
    const char *decision;
    const char *decisionTimer;
    const char *decisionLimit;
    const char *courseTimer;
    const char *courseClut;
    const char *courseTpage;

    if (diagnostics == NULL || diagnostics->initialized) return;
    memset(diagnostics, 0, sizeof(*diagnostics));
    diagnostics->initialized = 1;
    diagnostics->modelTraceTimer = -1;
    diagnostics->terrainTraceTimer = -1;
    diagnostics->terrainTraceClut = -1;
    diagnostics->terrainTraceTpage = -1;
    diagnostics->terrainDecisionTraceTimer = -1;
    diagnostics->terrainDecisionTraceLimit = 10000;
    diagnostics->courseTraceTimer = -1;
    diagnostics->courseTraceClut = -1;
    diagnostics->courseTraceTpage = -1;

    model = RuntimeConfigGet("diagnostics.model_trace");
    modelTimer = RuntimeConfigGet("diagnostics.model_trace_timer");
    diagnostics->modelTraceEnabled = model != NULL || modelTimer != NULL;
    diagnostics->modelTraceTimer = ParseValue(modelTimer, -1, 0);

    terrainTimer = RuntimeConfigGet("diagnostics.terrain_trace_timer");
    terrainClut = RuntimeConfigGet("diagnostics.terrain_trace_clut");
    terrainTpage = RuntimeConfigGet("diagnostics.terrain_trace_tpage");
    diagnostics->terrainTraceEnabled = terrainTimer != NULL || terrainClut != NULL || terrainTpage != NULL;
    diagnostics->terrainTraceTimer = ParseValue(terrainTimer, -1, 0);
    diagnostics->terrainTraceClut = ParseValue(terrainClut, -1, 16);
    diagnostics->terrainTraceTpage = ParseValue(terrainTpage, -1, 16);

    decision = RuntimeConfigGet("diagnostics.terrain_decision_trace");
    decisionTimer = RuntimeConfigGet("diagnostics.terrain_decision_timer");
    decisionLimit = RuntimeConfigGet("diagnostics.terrain_decision_limit");
    diagnostics->terrainDecisionTraceEnabled = decision != NULL || decisionTimer != NULL;
    diagnostics->terrainDecisionTraceTimer = ParseValue(decisionTimer, -1, 0);
    diagnostics->terrainDecisionTraceLimit = ParseValue(decisionLimit, 10000, 0);

    courseTimer = RuntimeConfigGet("diagnostics.course_trace_timer");
    courseClut = RuntimeConfigGet("diagnostics.course_trace_clut");
    courseTpage = RuntimeConfigGet("diagnostics.course_trace_tpage");
    diagnostics->courseTraceEnabled = courseTimer != NULL || courseClut != NULL || courseTpage != NULL;
    diagnostics->courseTraceTimer = ParseValue(courseTimer, -1, 0);
    diagnostics->courseTraceClut = ParseValue(courseClut, -1, 16);
    diagnostics->courseTraceTpage = ParseValue(courseTpage, -1, 16);
}
